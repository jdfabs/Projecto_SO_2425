/*********************************************************************************
 * client.c
 * Skipper
 * 11/10/2024
 * Client Entry-point
 *********************************************************************************/


/************************************
 * INCLUDES
 ************************************/
 #include "client.h"
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

/************************************
 * STATIC FUNCTIONS
 ************************************/
int main(int argc, char *argv[]) {
    ClientConfig config;


    log_event(config.log_file, "Client Started");
    if(argc > 1){
        if(load_client_config(argv[1], &config)>=0){
             printf("Client ID: %s\n", config.id);
             printf("Server IP: %s\n", config.server_ip);
             printf("Server Port: %d\n", config.server_port);
             printf("Log File: %s\n", config.log_file);
        } else {
             fprintf(stderr, "Failed to load client configuration.\n");
             exit(-1);
        }
    }
    else{
        printf("NO SPECIFIC CONFIG FILE PROVIVED, GOING FOR DEFAULT FILE\n");
        if(load_client_config("client_1", &config)>=0){
             printf("Client ID: %s\n", config.id);
             printf("Server IP: %s\n", config.server_ip);
             printf("Server Port: %d\n", config.server_port);
             printf("Log File: %s\n", config.log_file);
        } else {
             fprintf(stderr, "Failed to load client configuration.\n");
             exit(-1);
        }
    }


    log_event(config.log_file, "Client Config Loaded");

    
    exit(0);
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
