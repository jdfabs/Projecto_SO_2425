/*********************************************************************************
 * client_config.c
 * Skipper
 * 11/10/2024
 * Client Config handler 
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <cJSON.h> 
#include "client.h"

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
cJSON* parseFileToJson(FILE *file) ;

/************************************
 * STATIC FUNCTIONS
 ************************************/

void loadDefaulClientConfigs(ClientConfig *config) {
    printf("No config file found - Loading Default configs\n");
    strcpy(config->id ,"Client 1");
    strcpy(config ->server_ip , "127.0.0.1");
    config->server_port = 8080;
    strcpy(config->log_file, "./config/client_1.json");
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
int load_client_config(const char *filename, ClientConfig *config)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
        loadDefaulClientConfigs(config);
        return 1;

    }

      // Parse the JSON data
    cJSON *json = parseFileToJson(file);
   
    if (!json) {
        fprintf(stderr, "Could not parse JSON: %s\n", cJSON_GetErrorPtr());
        return -1;
    }

    // Get the client configuration
    const cJSON *client = cJSON_GetObjectItem(json, "client");
    if (client) {
        // Extracting values from the JSON object
        strcpy(config->id, cJSON_GetObjectItem(client, "id")->valuestring);
        strcpy(config->server_ip, cJSON_GetObjectItem(client, "server_ip")->valuestring);
        config->server_port = cJSON_GetObjectItem(client, "server_port")->valueint;
        strcpy(config->log_file, cJSON_GetObjectItem(client, "log_file")->valuestring);
    } else {
        fprintf(stderr, "Could not find 'client' in JSON\n");
        cJSON_Delete(json);
        return -1;
    }

    // Cleanup
    cJSON_Delete(json);
    return 0; // Success
}

cJSON* parseFileToJson(FILE *file) {
    
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    const long length = ftell(file);
    fseek(file, 0, SEEK_SET);   
    
    char *data = malloc(length + 1);
    if (data == NULL) {
        fclose(file);
        return NULL;
    }

    fread(data, 1, length, file);
    fclose(file);
    data[length] = '\0';  

    cJSON *json = cJSON_Parse(data);

    free(data);

    return json;
}