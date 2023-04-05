#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "cJSON.h"
#include <argp.h>
#include "utils.h"
#include "tuya_utils.h"
#include "ubus_utils.h"

int run = 1;

error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key){
    case 'a':
        arguments->daemon = 1;

    case 'p':
        arguments->product_id = arg;
        break;

    case 'd':
        arguments->device_id = arg;
        break;

    case 's':
        arguments->secret = arg;
        break;
    case ARGP_KEY_END:
    {
        printf("%s %s %s\n", arguments->device_id, arguments->product_id, arguments->secret);
        if(arguments->device_id == NULL || arguments->product_id == NULL || arguments->secret == NULL){
            argp_failure(state, 1, 0, "Product id, device id and device secret are all required\nCheck with --usage option");
        }
        break;
    }
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

int daemonize()
{
    int pid;

    if((pid = fork()) > 0){
        exit(0);
    } else if(pid == -1){
        return -1;
    }
    
    if(setsid() == -1)               
        return -1;

    if((pid = fork()) > 0){
        exit(0);
    } else if(pid == -1){
        return -1;
    }

    umask(0);
    chdir("/");
    
    close(STDIN_FILENO);

    int fd = open("/dev/null", O_RDWR);
    if(fd != STDIN_FILENO)
      return -1;
    if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
      return -2;
    if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
      return -3;

    return 0;
}

int write_to_file(char* parameter, char* message, char* filename)
{
    cJSON *json = cJSON_Parse(message);
    char* value = json->child->valuestring;
    char* param = json->child->string;

    if(strcmp(param, parameter) == 0){
        FILE* fptr = fopen(filename, "a");
        if(fptr == NULL){
            syslog(LOG_ERR, "Cannot open file for writing");
            return -1;
        }
        else{
            time_t seconds;
            time(&seconds);

            fprintf(fptr, "Timestamp: %ld, value: %s\n",seconds, value);
            fclose(fptr);
        }
    }
    cJSON_Delete(json);
    return 0;
}



void sig_handler(int signum)
{
    if(signum == SIGTERM || signum == SIGINT){
        syslog(LOG_WARNING, "Received interrupt signal");
        run = 0;
    }
}

int main_func(struct arguments arguments)
{
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
    int ret;
    if((ret = ubus_start())){
        return ret;
    }
    
    if((ret = tuya_start( arguments.device_id, arguments.secret)) != 0){
        tuya_deinit();
        return ret;
    }

    int uptime = 0; 

    while(run){
        if((ret = tuya_loop()) != 0){
            syslog(LOG_ERR, "Cannot maintain connection");
            return ret;
        }
        //report_uptime_data();
    }
    
    /*disconnect device*/
    tuya_deinit();
    ubus_end();

    return 0;
}

