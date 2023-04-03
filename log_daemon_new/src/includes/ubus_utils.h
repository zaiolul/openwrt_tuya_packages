#ifndef DAEMON_UBUS_UTILS_H
#define DAEMON_UBUS_UTILS_H
void uptime_cb(struct ubus_request *req, int type, struct blob_attr *msg);
/*device callback function*/
void devices_cb(struct ubus_request *req, int type, struct blob_attr *msg);
/*gpio state callback function*/
void state_cb(struct ubus_request *req, int type, struct blob_attr *msg);
/*start ubus context*/
int ubus_start();
int ubus_end();
/*check invoke method return value*/
void check_ubus_return_value(int value);
/*sends data about connected ESP devices to cloud*/
int report_device_data();
/*sends device uptime data to cloud*/
int report_uptime_data();
/*changes the state of gpio pin depending on the input from cloud, sends back response*/
int change_gpio_state(cJSON *input);
/*invokes ubus method*/
int invoke_method(char *path, char *method, struct blob_attr *msg,
    ubus_data_handler_t cb, void *data);
/*used in main loop*/

#endif