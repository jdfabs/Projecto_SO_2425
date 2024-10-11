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

#define SIZE 9

/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int load_server_config(ServerConfig *config);

bool isValidSudoku(int board[SIZE][SIZE]);
bool isValidGroup(int group[SIZE]);