/*********************************************************************************
 * client.h
 * Skipper
 * 11/10/2024
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/
typedef struct  {
    char id[19];
    char server_ip[15];
    int server_port;
    char log_file[254];
} ClientConfig;

/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int load_client_config(const char *filename, ClientConfig *config);
void printBoard(int **matrix);