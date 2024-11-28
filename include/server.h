/*********************************************************************************
 * server.h
 * Skipper
 * 11/10/2024
 *********************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cJSON.h"
#include "common.h"
#include <arpa/inet.h>

typedef struct {
	char log_file[255];
	char board_file_path[255];
	int task_queue_size;
	int task_handler_threads;
	char ip_address[255];
	int port;
	int server_size;
} server_config;

typedef struct {
	char name[255];
	int type;
	int max_players;
	int current_players;
	pthread_t thread_id;
} room_t;

typedef struct {
	int max_players;
	char *room_name;
} room_config_t;

typedef enum {
	RANKED = 0,
	CASUAL = 1,
	COOP = 2
}multiplayer_room_type_t ;



/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int load_server_config(const char *filename, server_config *config);




bool isValidGroup(int group[SIZE]);
int **generate_sudoku();
