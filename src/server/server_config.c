/*********************************************************************************
 * server_config.c
 * Skipper
 * 11/10/2024
 * Server config parser
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <cJSON.h>
#include "server.h"

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

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
int load_server_config(ServerConfig *config,  char *path){

    char configLoc[255];
    snprintf(configLoc, sizeof(configLoc),"./config/%s.json", path );


    FILE *file = fopen(configLoc, "r");
    if (!file) {
        //load default
        printf("No config file found - Loading Default configs\n");
        strcpy(config->ip ,"127.0.1.1");
        config->port = 8080;
        config->logging = true;
        strcpy(config->log_file ,"./logs/server_log.txt");
        config->log_level = 1;
        config->max_clients = 5;
        config->backup_interval = 15;

        return 1;
    }

    // Read the entire file into a string
    fseek(file, 0, SEEK_END);
    const long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(length + 1);
    fread(data, 1, length, file);
    fclose(file);
    data[length] = '\0'; // Null-terminate the string

      // Parse the JSON data
    cJSON *json = cJSON_Parse(data);
    if (!json) {
        fprintf(stderr, "Could not parse JSON: %s\n", cJSON_GetErrorPtr());
        free(data);
        return -1;
    }

    // Get the client configuration
    const cJSON *server = cJSON_GetObjectItem(json, "server");
    if (server) {
        // Extracting values from the JSON object
        strcpy(config->ip ,cJSON_GetObjectItem(server, "ip")->valuestring);
        config->port = cJSON_GetObjectItem(server, "port")->valueint;

        if(cJSON_GetObjectItem(server, "port")->valueint == 1){
            config->logging = true;
        }
        else{
            config->logging = false;
        }
        
        strcpy(config->log_file ,cJSON_GetObjectItem(server, "log_file")->valuestring);
        config->log_level = cJSON_GetObjectItem(server, "log_level")->valueint;
        config->max_clients = cJSON_GetObjectItem(server, "max_clients")->valueint;
        config->backup_interval = cJSON_GetObjectItem(server, "backup_interval")->valueint;
    } else {
        fprintf(stderr, "Could not find 'client' in JSON\n");
        cJSON_Delete(json);
        free(data);
        return -1;
    }

    // Cleanup
    cJSON_Delete(json);
    free(data);
    return 0; // Success

}