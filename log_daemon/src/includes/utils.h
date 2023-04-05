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

/*make program a daemon*/
int daemonize();
/*gets value field from json MESSAGE and writes to FILENAME upon receiving PARAMETER set*/
int write_to_file(char* parameter, char* message, char* filename);
/*main program loop*/
int main_func(struct arguments arguments);
/*program signal handler*/
void sig_handler(int signum);
/*uptime callback function*/

#endif