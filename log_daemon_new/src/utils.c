#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "cJSON.h"
#include <argp.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include "utils.h"
#include "tuya-utils.h"


//struct ubus_context esp_ctx; maybe later
uint32_t esp_ctx_id, system_ctx_id;
struct ubus_context *esp_ctx, *system_ctx;

int run = 1;

error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key){
    case 'a':
        arguments->daemon = 1;

    case 'p':
        arguments->product_id = arg;
        break;

    case 'd':
        arguments->device_id = arg;
        break;

    case 's':
        arguments->secret = arg;
        break;
    case ARGP_KEY_END:
    {
        printf("%s %s %s\n", arguments->device_id, arguments->product_id, arguments->secret);
        if(arguments->device_id == NULL || arguments->product_id == NULL || arguments->secret == NULL){
            argp_failure(state, 1, 0, "Product id, device id and device secret are all required\nCheck with --usage option");
        }
        break;
    }
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

int daemonize()
{
    int pid;

    if((pid = fork()) > 0){
        exit(0);
    } else if(pid == -1){
        return -1;
    }
    
    if(setsid() == -1)               
        return -1;

    if((pid = fork()) > 0){
        exit(0);
    } else if(pid == -1){
        return -1;
    }

    umask(0);
    chdir("/");
    
    close(STDIN_FILENO);

    int fd = open("/dev/null", O_RDWR);
    if(fd != STDIN_FILENO)
      return -1;
    if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
      return -2;
    if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
      return -3;

    return 0;
}

int write_to_file(char* parameter, char* message, char* filename)
{
    cJSON *json = cJSON_Parse(message);
    char* value = json->child->valuestring;
    char* param = json->child->string;

    if(strcmp(param, parameter) == 0){
        FILE* fptr = fopen(filename, "a");
        if(fptr == NULL){
            syslog(LOG_ERR, "Cannot open file for writing");
            return -1;
        }
        else{
            time_t seconds;
            time(&seconds);

            fprintf(fptr, "Timestamp: %ld, value: %s\n",seconds, value);
            fclose(fptr);
        }
    }
    cJSON_Delete(json);
    return 0;
}

void uptime_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
	struct blob_attr *tb[UPTIME_MAX];
    int *uptime_value = (int*)req->priv;
	blobmsg_parse(info_policy, UPTIME_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[UPTIME_VALUE]) {
		return;
	}
    *uptime_value = blobmsg_get_u32(tb[UPTIME_VALUE]);
}

void devices_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
    struct blob_attr *tb[DEVICES_INFO_MAX];
    struct blob_attr *device_data[DEVICE_MAX];
    struct device_list *dev_list = (struct device_list*)req->priv;
    
    blobmsg_parse_array(devices_policy, DEVICES_INFO_MAX, tb, blob_data(msg), blob_len(msg));
    
    if (!tb[DEVICES_INFO_ARR]) {
        return;
	}
    
    struct blob_attr *table;
    size_t rem;

    blobmsg_for_each_attr(table, tb[DEVICES_INFO_ARR], rem){
        blobmsg_parse(device_data_policy, DEVICE_MAX, device_data, blobmsg_data(table), blobmsg_data_len(table));

        struct device dev;
        memcpy(&dev.port, blobmsg_get_string(device_data[DEVICE_PORT]), 30);
        memcpy(&dev.pid, blobmsg_get_string(device_data[DEVICE_PID]), 30);
        memcpy(&dev.vid, blobmsg_get_string(device_data[DEVICE_VID]), 30);
        memcpy(&((*dev_list).devices[(*dev_list).count]), &dev, sizeof(struct device));
        (*dev_list).count++;
        if((*dev_list).count == DEVICE_CAP){
            syslog(LOG_ERR, "Device cap reached");
            return;
        }
    }
}

void state_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
    struct blob_attr *tb[CONTROL_MAX];
    struct esp_response *response = (struct esp_response*)req->priv;

    blobmsg_parse(control_policy, CONTROL_MAX, tb, blob_data(msg), blob_len(msg));
    
    (*response).code = blobmsg_get_u32(tb[CONTROL_RESPONSE_VALUE]);
    memcpy(&((*response).message), blobmsg_get_string(tb[CONTROL_MESSAGE]), sizeof((*response).message));
}

int ubus_start(struct ubus_context **ctx, char *path, uint32_t *id)
{
    (*ctx) = ubus_connect(NULL);
	if (!(*ctx)) {
		syslog(LOG_ERR, "Failed to connect to ubus");
		return -1;
	}
	int ret = ubus_lookup_id(*ctx, path, id);
    return 0;
}

int change_gpio_state(cJSON *input)
{
    if(!cJSON_HasObjectItem(input, "port") || !cJSON_HasObjectItem(input, "gpio") ||
        !cJSON_HasObjectItem(input, "state")){
        
        send_report("{\"error\":\"invalid action input\"}");
        syslog(LOG_ERR, "invalid action input");
        return -1;
    }
    char* port = cJSON_GetObjectItemCaseSensitive(input, "port")->valuestring;
    int gpio = cJSON_GetObjectItemCaseSensitive(input, "gpio")->valueint;
    char* state = cJSON_GetObjectItemCaseSensitive(input, "state")->valuestring;
    
    char full_message[1024];

    struct blob_buf buf = {};
    blob_buf_init(&buf, 0);
    blobmsg_add_string(&buf, "port", port);
    blobmsg_add_u32(&buf, "pin", (uint32_t) gpio);
    
    struct esp_response response;

    int ret;
    if ((ret = invoke_method(esp_ctx, esp_ctx_id, state, buf.head, state_cb, &response)) != 0){
        return ret;
    }
    
    sprintf(full_message, "{\"esp_response\":\"{\\\"response\\\":%d,\\\"msg\\\":\\\"%s\\\"}\"}",
        response.code, response.message);
    ret = send_report(full_message);

    return ret;
}

int invoke_method(struct ubus_context *ctx, uint32_t id, char *method, struct blob_attr *msg,
    ubus_data_handler_t cb, void *data)
{
    if(!ctx) return -1;
    int ret = ubus_invoke(ctx, id, method, msg, cb, data, 4000);
    if(ret != 0){
        check_ubus_return_value(ret);
    }
    return ret;
}

int report_uptime_data()
{
    int uptime = 0;
    int ret;
    if((ret = invoke_method(system_ctx, system_ctx_id, "info", NULL, uptime_cb, &uptime)) != 0){
        return ret;
    }

    char full_message[1024];
    sprintf(full_message, "{\"uptime\":\"{\\\"value\\\":%d}\"}", uptime);
    ret = send_report(full_message);
    return ret;
}

int report_device_data()
{
    struct device_list dev_list = {
        .count = 0,
    };
    
    int ret;
    if ((ret = invoke_method(esp_ctx, esp_ctx_id, "devices", NULL, devices_cb, &dev_list)) != 0){
        return ret;
    }

    char array_data[DEVICE_CAP * 1024] = "";
    for(int i = 0; i < dev_list.count; i ++)
    {
        char part[1024];
        sprintf(part, "\"{\\\"port\\\":\\\"%s\\\",\\\"vid\\\":\\\"%s\\\",\\\"pid\\\":\\\"%s\\\"}\",",
        dev_list.devices[i].port, dev_list.devices[i].vid, dev_list.devices[i].pid); // creates report string
        printf("%s\n", part);
        strncat(array_data, part, 1024);
    }

    array_data[strlen(array_data) - 1] = '\0'; // remove last comma;
    char full_message[1024];
    sprintf(full_message, "{\"device_list\":[%s]}", array_data);

    ret = send_report(full_message);
    return ret;
}

void check_ubus_return_value(int value){
    //expand as needed
    char error_message[20];
    switch(value){
        case UBUS_STATUS_INVALID_ARGUMENT:
            strncpy(error_message, "Bad arguments", sizeof(error_message));
            break;
        case UBUS_STATUS_METHOD_NOT_FOUND:
            strncpy(error_message, "Bad method",sizeof(error_message));
            break;
        case UBUS_STATUS_NO_DATA:
            strncpy(error_message, "No data", sizeof(error_message));
            break;

        default:
            return;
    }

    char report_message[1024];
    sprintf(report_message, "{\"error\":\"%s\"}", error_message);
    syslog(LOG_ERR, error_message);
    send_report(report_message);

}

void sig_handler(int signum)
{
    if(signum == SIGTERM || signum == SIGINT){
        syslog(LOG_WARNING, "Received interrupt signal");
        run = 0;
    }
}

int main_func(struct arguments arguments)
{
    /*register interrupt*/
    struct sigaction act;
    act.sa_handler = sig_handler;
    sigaction(SIGINT,  &act, 0);
    sigaction(SIGTERM, &act, 0);
    
    /*start log*/
    openlog("log_daemon", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
    setlogmask (LOG_UPTO (LOG_INFO));

    /*make program a daemon*/
    if(arguments.daemon){
        int status = daemonize();
        if(status < 0){
            syslog(LOG_ERR, "Cannot create daemon");
            return -1;
        }
    }
    int ret = 0;
    if((ret = ubus_start(&esp_ctx, "esp_control", &esp_ctx_id)) != 0 ||
        (ret = ubus_start(&system_ctx, "system", &system_ctx_id)) != 0){
        
        syslog(LOG_ERR, "Failed to get ubus objects");
        return ret;
    }
    
    if((ret = client_init( arguments.device_id, arguments.secret)) != 0){
        client_deinit();
        return ret;
    }

    int uptime = 0; 

    while(run){
        if((ret = tuya_loop()) != 0){
            syslog(LOG_ERR, "Cannot maintain connection");
            return ret;
        }
        //report_uptime_data();
    }
    
    /*disconnect device*/
    client_deinit();
    ubus_free(esp_ctx);
    ubus_free(system_ctx);
    return 0;
}

