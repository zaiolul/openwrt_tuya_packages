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
#include "includes/utils.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include "utils.h"

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

void callback(struct ubus_request *req, int type, struct blob_attr *msg)
{
	struct blob_attr *tb[UPTIME_MAX];
    int *uptime_value = (int*)req->priv;
	blobmsg_parse(info_policy, UPTIME_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[UPTIME_VALUE]) {
		puts("No uptime data received\n");
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
        puts("No device data received\n");
        return;
	}
    
    struct blob_attr *table;
    size_t rem;

    blobmsg_for_each_attr(table, tb[DEVICES_INFO_ARR], rem){
        blobmsg_parse(device_data_policy, DEVICE_MAX, device_data, blobmsg_data(table), blobmsg_data_len(table));
        struct device dev;

        //memcpy(&dev.name, blobmsg_get_string(device_data[DEVICE_NAME]), 30);
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

int ubus_start(struct ubus_context **ctx, char *path, uint32_t *id)
{
    (*ctx) = ubus_connect(NULL);
	if (!(*ctx)) {
		syslog(LOG_ERR, "Failed to connect to ubus");
		return -1;
	}
	int ret = ubus_lookup_id(*ctx, path, id);
    printf("%d", ret);
    return 0;
}

