#ifndef UTILS_H
#define UTILS_H
#include <argp.h>
#include <tuyalink_core.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>

/*program usage documentation*/
static char doc[] = "Log daemon program. Connects to Tuya cloud service, sends and receives data.";

/*argp options struct*/
static struct argp_option options[] = {
    {"product_id", 'p', "[PRODUCT ID]", 0, 0 },
    {"device_id", 'd', "[DEVICE ID]", 0, 0 },
    {"device_secret", 's', "[DEVICE SECRET]", 0},
    {"daemon", 'a', 0, 0},
    {0}
};
/*option parsing function*/
error_t parse_opt (int key, char *arg, struct argp_state *state);

static struct argp argp = { options, parse_opt, 0, doc };

/*arguments struct*/
struct arguments{
    int daemon;
    char* secret;
    char* product_id;
    char* device_id;         
};

/*device data*/
#define DEVICE_CAP 16
struct device{
    char name[100];
    char port[30];
    char vid[30];
    char pid[30];
};

struct device_list{
    struct device devices[DEVICE_CAP];
    int count;
};

struct esp_response{
    int code;
    char message[1024];
};

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


/*make program a daemon*/
int daemonize();
/*gets value field from json MESSAGE and writes to FILENAME upon receiving PARAMETER set*/
int write_to_file(char* parameter, char* message, char* filename);
/*uptime callback function*/
void uptime_cb(struct ubus_request *req, int type, struct blob_attr *msg);
/*device callback function*/
void devices_cb(struct ubus_request *req, int type, struct blob_attr *msg);
/*gpio state callback function*/
void state_cb(struct ubus_request *req, int type, struct blob_attr *msg);
/*start ubus context*/
int ubus_start(struct ubus_context **ctx, char *path, uint32_t *id);
/*check invoke method return value*/
void check_ubus_return_value(int value);
/*main program loop*/
int main_func(struct arguments arguments);
/*program signal handler*/
void sig_handler(int signum);
/*sends data about connected ESP devices to cloud*/
int report_device_data();
/*sends device uptime data to cloud*/
int report_uptime_data();
/*changes the state of gpio pin depending on the input from cloud, sends back response*/
int change_gpio_state(cJSON *input);
/*invokes ubus method*/
int invoke_method(struct ubus_context *ctx, uint32_t id, char *method, struct blob_attr *msg,
    ubus_data_handler_t cb, void *data);

#endif