#ifndef TUYA_UTILS_H
#define TUYA_UTILS_H

#include "tuyalink_core.h"

/*calls on client connection*/
void on_connected(tuya_mqtt_context_t* context, void* user_data);
/*calls on client connection*/
void on_disconnect(tuya_mqtt_context_t* context, void* user_data);
/*calls when messages are sent and received*/
void on_messages(tuya_mqtt_context_t* context, void* user_data, const tuyalink_message_t* msg);
/*initialises mqtt client and connects to cloud*/
int client_init( tuya_mqtt_context_t* client, char *device_id, char *secret);
/*disconnect the device, clear up memory*/
int client_deinit(tuya_mqtt_context_t* client);

/*sends REPORT to cloud*/
int send_report(tuya_mqtt_context_t* client, char *device_id, char* report);
#endif