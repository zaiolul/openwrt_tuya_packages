#include <syslog.h>
#include "tuyalink_core.h"
#include "tuya_cacert.h"
#include "tuya_error_code.h"
#include "utils.h"

void on_connected(tuya_mqtt_context_t* context, void* user_data)
{
    syslog(LOG_INFO,"Client connected");
}

void on_disconnect(tuya_mqtt_context_t* context, void* user_data)
{
    syslog(LOG_INFO,"Client disconnected");
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

        default:
            break;
    }
}

int client_init( tuya_mqtt_context_t* client, char *device_id, char *secret)
{
     /* initialize the client */
    int ret = tuya_mqtt_init(client, &(const tuya_mqtt_config_t) {
        .host = "m1.tuyacn.com",
        .port = 8883,
        .cacert = tuya_cacert_pem,
        .cacert_len = sizeof(tuya_cacert_pem),
        .device_id = device_id,
        .device_secret = secret,
        .keepalive = 100,
        .timeout_ms = 1000,
        .on_connected = on_connected,
        .on_disconnect = on_disconnect,
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

int client_deinit(tuya_mqtt_context_t* client)
{
    tuya_mqtt_disconnect(client); 
    tuya_mqtt_deinit(client); //free memory
    syslog(LOG_WARNING, "Daemon terminated");
}

int send_report(tuya_mqtt_context_t* client, char *device_id, char* report)
{
    /*send data to cloud*/
    int ret;
    if((ret = tuyalink_thing_property_report_with_ack(client, device_id, report)) == OPRT_INVALID_PARM){
        syslog(LOG_ERR, "Cannot send report");
        return ret;
    }
    syslog(LOG_INFO, "Report message was sent succesfully");
    return OPRT_OK;
}