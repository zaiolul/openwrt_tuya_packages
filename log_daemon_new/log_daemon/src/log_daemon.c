#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include "utils.h"
#include "tuya-utils.h"
#include "tuya_error_code.h"
#include "tuyalink_core.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>

static struct argp argp = { options, parse_opt, 0, doc };

void sig_handler(int signum);

int run = 1;

int main(int argc, char *argv[])
{ 
    /*get program arguments*/
    struct arguments arguments;
    arguments.daemon = 0;
    arguments.device_id = NULL;
    arguments.product_id = NULL;
    arguments.secret = NULL;

    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    /*register interrupt*/
    struct sigaction act;
    act.sa_handler = sig_handler;
    sigaction(SIGINT,  &act, 0);
    sigaction(SIGTERM, &act, 0);
    
    /*start log*/
    openlog("log_daemon", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
    setlogmask (LOG_UPTO (LOG_INFO));

    /*make program a daemon*/
    if(arguments.daemon){
        int status = daemonize();
        if(status < 0){
            syslog(LOG_ERR, "Cannot create daemon");
            return -1;
        }
    }
    
    int ret = client_init( arguments.device_id, arguments.secret);
    if(ret != OPRT_OK){
        client_deinit();
        return ret;
    }

    char report[1024];
    //struct ubus_context *ctx;
	uint32_t id;
    struct ubus_context *ctx;
    int uptime = 0; //stores data from ubus
    
	ubus_start(&ctx, "system", &id);

    while(run){
        ubus_invoke(ctx, id, "info", NULL, uptime_cb, &uptime, 1000);
        sprintf(report, "{\"upload\":{\"value\":\"%d\"}}", uptime); // creates report string
      //  printf("uptime: %s\n", report);
        /*maintain connection*/
        if((ret = tuya_loop()) != OPRT_OK){
            syslog(LOG_ERR, "Cannot maintain connection");
            return ret;
        }
        fflush(stdout);
        //report_uptime_data(report);
    }
    
    /*disconnect device*/
    client_deinit();
    ubus_free(ctx);

    return 0;
}

void sig_handler(int signum)
{
    if(signum == SIGTERM || signum == SIGINT){
        syslog(LOG_WARNING, "Received interrupt signal");
        run = 0;
    }
    
}

