#include <syslog.h>
#include "tuyalink_core.h"
#include "tuya_cacert.h"
#include "tuya_error_code.h"
#include "utils.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>

tuya_mqtt_context_t client_instance;

int get_field_value(const tuyalink_message_t* msg, char* str, char* value)
{
    cJSON *json = cJSON_Parse(msg->data_string);
    json = json->child;
    while(json != NULL){
        if(strcmp(str, json->string) == 0){
            strncpy(value, json->valuestring, 30);
            break;
        }
        json = json->next;
    }
    cJSON_free(json);
    return 0;
}

int send_report(char* report)
{
    tuya_mqtt_context_t* client = &client_instance;
    /*send data to cloud*/
    int ret;
    if((ret = tuyalink_thing_property_report_with_ack(client, NULL, report)) == OPRT_INVALID_PARM){
        syslog(LOG_ERR, "Cannot send report");
        return ret;
    }
    syslog(LOG_INFO, "Report message was sent succesfully");
    return OPRT_OK;
}

int get_device_data(struct device_list *dev_list)
{
    (*dev_list).count = 0;
    uint32_t id;
    struct ubus_context *ctx;
    ubus_start(&ctx, "esp_control", &id);
    ubus_invoke(ctx, id, "devices", NULL, devices_cb, dev_list, 1000);
    ubus_free(ctx);
}
int report_device_data(struct device_list dev_list)
{
    char array_data[DEVICE_CAP*1024] = "";
    for(int i = 0; i < dev_list.count; i ++)
    {
        char part[1024];
        sprintf(part, "\"{\\\"port\\\":\\\"%s\\\",\\\"vid\\\":\\\"%s\\\",\\\"pid\\\":\\\"%s\\\"}\",",
        dev_list.devices[i].port, dev_list.devices[i].vid, dev_list.devices[i].pid); // creates report string
        printf("%s\n", part);
        strncat(array_data, part, 1024);
    }

    array_data[strlen(array_data) - 1] = '\0'; // remove last comma;
    char full_message[sizeof(array_data)];
    sprintf(full_message, "{\"device_list\":[%s]}", array_data);

    int ret = send_report(full_message);
    return ret;
}

void on_messages(tuya_mqtt_context_t* context, void* user_data, const tuyalink_message_t* msg)
{
    char value[30];
    switch (msg->type) {
        
        case THING_TYPE_PROPERTY_REPORT_RSP:
            syslog(LOG_INFO, "Cloud received and replied: id:%s, type:%d", msg->msgid, msg->type);
            break;

        case THING_TYPE_PROPERTY_SET:
            syslog(LOG_INFO, "Device received id:%s, type:%d", msg->msgid, msg->type);
            write_to_file("write", msg->data_string, "/var/tmp/from-cloud");
            break;
        case THING_TYPE_ACTION_EXECUTE:
            get_field_value(msg, "actionCode", value);
            syslog(LOG_INFO, "%s action received", value);

            if(strcmp(value, "get_devices") == 0){
                struct device_list dev_list;
                get_device_data(&dev_list);
                report_device_data(dev_list);
            }
            break;

        default:
            break;
    }
}

int client_init(char *device_id, char *secret)
{
    tuya_mqtt_context_t* client = &client_instance;
    /* initialize the client */
    int ret = tuya_mqtt_init(client, &(const tuya_mqtt_config_t) {
        .host = "m1.tuyacn.com",
        .port = 8883,
        .cacert = tuya_cacert_pem,
        .cacert_len = sizeof(tuya_cacert_pem),
        .device_id = device_id,
        .device_secret = secret,
        .keepalive = 100,
        .timeout_ms = 3000,
        .on_connected = NULL,
        .on_disconnect = NULL,
        .on_messages = on_messages
    });

    if(ret != OPRT_OK){
        syslog(LOG_ERR, "Failed to initialize");
        return ret;
    }
    int retry_count = 0;
    int max_retries = 5;
    while(retry_count < max_retries)
    {
        /*try to connect*/
        if((ret = tuya_mqtt_connect(client)) != OPRT_OK){
            syslog(LOG_WARNING, "Failed to connect... retrying");
        }
        else break;
        retry_count++;
    }
    if(retry_count == max_retries){
        syslog(LOG_ERR, "Can't establish connection. Terminating.");
        return ret;
    }
   
    syslog(LOG_WARNING, "Log Daemon Start");
    return OPRT_OK;
}

int client_deinit()
{
    tuya_mqtt_context_t* client = &client_instance;
    tuya_mqtt_disconnect(client); 
    tuya_mqtt_deinit(client); //free memory
    syslog(LOG_WARNING, "Daemon terminated");
}

int tuya_loop()
{
    return tuya_mqtt_loop(&client_instance);
}

