#ifndef TUYA_UTILS_H
#define TUYA_UTILS_H

#include "tuyalink_core.h"

/*calls when messages are sent and received*/
void on_messages(tuya_mqtt_context_t* context, void* user_data, const tuyalink_message_t* msg);
/*initialises mqtt client and connects to cloud*/
int client_init(char *device_id, char *secret);
/*disconnect the device, clear up memory*/
int client_deinit();

/*sends REPORT to cloud*/
int send_report(char* report);
int get_field_value(const tuyalink_message_t* msg, char* str, char* value, int n);
int report_device_data(struct device_list dev_list);
int get_device_data(struct device_list *dev_list);
int tuya_loop();
#endif