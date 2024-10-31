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
	char ip[16];
	int port;
	bool logging;
	char log_file[255];
	int log_level;
	int max_clients;
	int backup_interval;
	char board_file_path[255];
	int task_queue_size;
	int event_handler_threads;
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
