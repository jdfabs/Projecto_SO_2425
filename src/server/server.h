/*********************************************************************************
 * server.h
 * Skipper
 * 11/10/2024
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cJSON.h"
#include "../common/common.h"
/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/
typedef struct 
{
    char ip[16]; 
    int port;
    bool logging;
    char log_file[255];
    int log_level;
    int max_clients;
    int backup_interval;
} ServerConfig;



/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int load_server_config(const char *filename, ServerConfig *config);


bool isValidGroup(int group[SIZE]);

int** getMatrixFromJSON(cJSON *board);

cJSON* matrix_to_JSON(int **matrix);