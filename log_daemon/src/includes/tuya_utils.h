#ifndef TUYA_UTILS_H
#define TUYA_UTILS_H

/*calls when messages are sent and received*/
void on_messages(tuya_mqtt_context_t* context, void* user_data, const tuyalink_message_t* msg);
/*initialises mqtt client and connects to cloud*/
int tuya_start(char *device_id, char *secret);
/*disconnect the device, clear up memory*/
int tuya_deinit();

void action_handler(const tuyalink_message_t *msg);
/*sends message to cloud*/
int send_report(char* report);
/*tuya loop wrapper*/
int tuya_loop();
int tuya_connect(int retries);
int tuya_init(char *device_id, char *secret);
#endif