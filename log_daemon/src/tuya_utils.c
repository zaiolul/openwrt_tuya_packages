#include <syslog.h>
#include "tuyalink_core.h"
#include "tuya_cacert.h"
#include "tuya_error_code.h"
#include "utils.h"
#include "ubus_utils.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>

tuya_mqtt_context_t client_instance;

int send_report(char* report)
{
    /*send data to cloud*/
    int ret;
    if((ret = tuyalink_thing_property_report_with_ack(&client_instance, NULL, report)) == OPRT_INVALID_PARM){
        syslog(LOG_ERR, "Cannot send report");
        return ret;
    }
    syslog(LOG_INFO, "Report message was sent succesfully");
    return OPRT_OK;
}
void action_handler(const tuyalink_message_t *msg)
{
    cJSON *json = cJSON_Parse(msg->data_string);
    cJSON *action = cJSON_GetObjectItemCaseSensitive(json, "actionCode");
    syslog(LOG_INFO, "%s action received", action->valuestring); 

    if(strcmp( action->valuestring, "get_devices") == 0){
        report_device_data();
    } else if(strcmp(action->valuestring, "change_state") == 0){
        cJSON *params = cJSON_GetObjectItemCaseSensitive(json, "inputParams");
        change_gpio_state(params);
    }
    cJSON_free(json);
}
void on_messages(tuya_mqtt_context_t* context, void* user_data, const tuyalink_message_t* msg)
{
    switch (msg->type) {
        case THING_TYPE_PROPERTY_REPORT_RSP:
            syslog(LOG_INFO, "Cloud received and replied: id:%s, type:%d", msg->msgid, msg->type);
            break;

        case THING_TYPE_PROPERTY_SET:
            syslog(LOG_INFO, "Device received id:%s, type:%d", msg->msgid, msg->type);
            write_to_file("write", msg->data_string, "/var/tmp/from-cloud");
            break;
        case THING_TYPE_ACTION_EXECUTE: 
            action_handler(msg);
            break;

        default:
            break;
    }
}
int tuya_init(char *device_id, char *secret)
{
    /* initialize the client */
    int ret = tuya_mqtt_init(&client_instance, &(const tuya_mqtt_config_t) {
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
    return OPRT_OK;
}
int tuya_connect(int retries)
{
    int retry_count = 0;
    int ret;
    while(retry_count < retries)
    {
        /*try to connect*/
        if((ret = tuya_mqtt_connect(&client_instance)) != OPRT_OK){
            syslog(LOG_WARNING, "Failed to connect... retrying");
        }
        else break;
        retry_count++;
    }
    if(retry_count == retries){
        syslog(LOG_ERR, "Can't establish connection. Terminating.");
        return ret;
    }
    return OPRT_OK;
}
int tuya_start(char *device_id, char *secret)
{   
    int ret;
    if((ret = tuya_init(device_id, secret)) != OPRT_OK)
        return ret;
    if((ret = tuya_connect(5)) != OPRT_OK)
        return ret;
   
    syslog(LOG_WARNING, "Log Daemon Start");
    return OPRT_OK;
}

int tuya_deinit()
{
    tuya_mqtt_disconnect(&client_instance); 
    tuya_mqtt_deinit(&client_instance); //free memory
    syslog(LOG_WARNING, "Daemon terminated");
}

int tuya_loop()
{
    return tuya_mqtt_loop(&client_instance);
}

