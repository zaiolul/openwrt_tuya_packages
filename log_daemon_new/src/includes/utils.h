#ifndef UTILS_H
#define UTILS_H
#include <argp.h>
#include <tuyalink_core.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>

extern struct ubus_context ubus_ctx;
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
/*arguments struct*/
struct arguments{
    int daemon;
    char* secret;
    char* product_id;
    char* device_id;         
};

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

/*option parsing function*/
error_t parse_opt (int key, char *arg, struct argp_state *state);
/*make program a daemon*/
int daemonize();
/*gets value field from json MESSAGE and writes to FILENAME upon receiving PARAMETER set*/
int write_to_file(char* parameter, char* message, char* filename);
/*callback function for ubus*/
void callback(struct ubus_request *req, int type, struct blob_attr *msg);
void devices_cb(struct ubus_request *req, int type, struct blob_attr *msg);
int ubus_start(struct ubus_context **ctx, char *path, uint32_t *id);

#endif