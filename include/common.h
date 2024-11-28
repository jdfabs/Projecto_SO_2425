/*********************************************************************************
 * common.h
 * Skipper
 * 11/10/2024
* common functions
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include "cJSON.h"
#include <semaphore.h>
#include <stdbool.h>
/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/
#pragma once
#define SERVER_LOG_PATH "logs/server.log"
#define CLIENT_LOG_PATH "logs/client.log"
#define SIZE 9

#define INITIAL_STATE 0
#define SOLUTION_STATE 1

#define BUFFER_SIZE 1024


typedef struct {
	int client_socket;
	char request[BUFFER_SIZE];
} Task;

typedef struct multiplayer_ranked_room_shared_data {
 int board_id;
 char starting_board[BUFFER_SIZE], room_name[BUFFER_SIZE];

 Task task_queue[5];
 int task_productor_ptr, task_consumer_ptr;

} multiplayer_ranked_room_shared_data_t;

typedef struct multiplayer_casual_room_shared_data {
 int board_id;
 char starting_board[BUFFER_SIZE], room_name[BUFFER_SIZE];

 Task task_queue[20];//TODO malloc this?
 sem_t sems_client[20];
 sem_t sems_server[20];
 bool has_solution[20];

 int counter;
} multiplayer_casual_room_shared_data_t;

typedef struct multiplayer_coop_room_shared_data {
 int board_id;
 char starting_board[BUFFER_SIZE], room_name[BUFFER_SIZE];
 //TODO
} multiplayer_coop_room_shared_data_t;



typedef struct singleplayer_room_shared_data {
 int board_id;
 char starting_board[BUFFER_SIZE], room_name[BUFFER_SIZE];

 char buffer[1024];
} singleplayer_room_shared_data_t;
/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int log_event(const char *file_path, const char *message);
int **getMatrixFromJSON(cJSON *board);
