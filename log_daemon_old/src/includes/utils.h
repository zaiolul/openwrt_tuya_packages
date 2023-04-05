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
/*arguments struct*/
struct arguments{
    int daemon;
    char* secret;
    char* product_id;
    char* device_id;         
};

enum {
    UPTIME_VALUE,
    UPTIME_MAX
};

static const struct blobmsg_policy info_policy[UPTIME_MAX] = {
	[UPTIME_VALUE] = { .name = "uptime", .type = BLOBMSG_TYPE_INT32},
};

/*option parsing function*/
error_t parse_opt (int key, char *arg, struct argp_state *state);
/*make program a daemon*/
int daemonize();
/*gets value field from json MESSAGE and writes to FILENAME upon receiving PARAMETER set*/
int write_to_file(char* parameter, char* message, char* filename);
/*callback function for ubus*/
void callback(struct ubus_request *req, int type, struct blob_attr *msg);

#endif