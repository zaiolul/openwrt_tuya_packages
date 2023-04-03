#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include "utils.h"

int main(int argc, char *argv[])
{ 
    /*get program arguments*/
    struct arguments arguments;
    arguments.daemon = 0;
    arguments.device_id = NULL;
    arguments.product_id = NULL;
    arguments.secret = NULL;

    argp_parse (&argp, argc, argv, 0, 0, &arguments);
    
    return main_func(arguments);
}

