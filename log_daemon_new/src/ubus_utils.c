
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include "utils.h"
#include "tuya_utils.h"

uint32_t esp_ctx_id, system_ctx_id;
struct ubus_context *esp_ctx, *system_ctx;

enum {
    UPTIME_VALUE,
    UPTIME_MAX
};
enum{
    DEVICE_NAME,
    DEVICE_PORT,
    DEVICE_VID,
    DEVICE_PID,
    DEVICE_MAX
};
enum {
    DEVICES_INFO_ARR,
    DEVICES_INFO_MAX
};

enum {
    CONTROL_RESPONSE_VALUE,
    CONTROL_MESSAGE,
    CONTROL_MAX
};


static const struct blobmsg_policy info_policy[UPTIME_MAX] = {
	[UPTIME_VALUE] = { .name = "uptime", .type = BLOBMSG_TYPE_INT32},
};
static const struct blobmsg_policy device_data_policy[DEVICE_MAX] = {
    [DEVICE_NAME] = { .name = "esp_name", .type = BLOBMSG_TYPE_STRING},
	[DEVICE_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING},
    [DEVICE_PID] = { .name = "pid", .type = BLOBMSG_TYPE_STRING},
    [DEVICE_VID] = { .name = "vid", .type = BLOBMSG_TYPE_STRING},
};

static const struct blobmsg_policy devices_policy[DEVICES_INFO_MAX] = {
	[DEVICES_INFO_ARR] = { .name = "devices", .type = BLOBMSG_TYPE_ARRAY},
};

static const struct blobmsg_policy control_policy[CONTROL_MAX] = {
	[CONTROL_RESPONSE_VALUE] = { .name = "response", .type = BLOBMSG_TYPE_INT32},
    [CONTROL_MESSAGE] = {.name = "msg", .type = BLOBMSG_TYPE_STRING}
};

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

int ubus_start_all()
{
    int ret = 0;
    if((ret = ubus_start(&esp_ctx, "esp_control", &esp_ctx_id)) != 0 ||
        (ret = ubus_start(&system_ctx, "system", &system_ctx_id)) != 0){
        
        syslog(LOG_ERR, "Failed to get ubus objects");
    }
    return ret;
}

int ubus_free_all()
{
    ubus_free(system_ctx);
    ubus_free(esp_ctx);
}