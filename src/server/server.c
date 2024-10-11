/*********************************************************************************
 * server.c
 * Skipper
 * 11/10/2024
 * Server entry-point
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <file_handler.h>
#include "server.h"
#include "../common/common.h"
/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
ServerConfig load_default_config();

/************************************
 * STATIC FUNCTIONS
 ************************************/

int main(int argc, char *argv[]) {
    
    ServerConfig config;

    if(load_server_config(&config)>=0){
        printf("Client IP: %s\n", config.ip);
        printf("LogFile: %s\n", config.log_file);
        printf("LogLevel: %d\n", config.log_level);
        printf("Logging: %d\n", config.logging);
        printf("Max_Clients: %d\n", config.max_clients);
        printf("Port: %d\n", config.port);

    } else {
        fprintf(stderr, "Failed to load server configuration.\n");
        exit(-1);
    }

    log_event(config.log_file,"Server Started");
    log_event(config.log_file,"Server Config Loaded");

    
    exit(0);
     
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
