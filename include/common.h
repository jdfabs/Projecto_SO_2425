/*********************************************************************************
 * common.h
 * Skipper
 * 11/10/2024
 * common functions  
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <pthread.h>
#include <semaphore.h>
#include "cJSON.h"
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
#define MESSAGE_SIZE 256


typedef struct {
	int client_socket;
	char request[BUFFER_SIZE];
} Task;

typedef struct multiplayer_room_shared_data {
 int board_id;
 char starting_board[BUFFER_SIZE], room_name[BUFFER_SIZE];

 Task task_queue[5];
 int task_productor_ptr, task_consumer_ptr;

} multiplayer_room_shared_data_t;
/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int log_event(const char *file_path, const char *message);
int **getMatrixFromJSON(cJSON *board);
