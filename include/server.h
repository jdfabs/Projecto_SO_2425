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
#include "common.h"
#include <arpa/inet.h>
/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/
typedef struct {
	char log_file[255];
	char board_file_path[255];
	int task_queue_size;
	int task_handler_threads;
 char ip_address[255];
 int port;
} server_config;


/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int load_server_config(const char *filename, server_config *config);




bool isValidGroup(int group[SIZE]);
int **generate_sudoku();
