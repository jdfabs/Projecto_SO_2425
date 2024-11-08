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


typedef struct client_message{
 int client_socket;
 char message[MESSAGE_SIZE];
} client_message_t;

typedef struct multiplayer_room_shared_data {
 sem_t sem_game_start;
 sem_t sem_room_full;
 sem_t sem_found_solution;
 pthread_mutex_t mutex1;
 pthread_mutex_t mutex2;
 int board_id;
 char starting_board[BUFFER_SIZE];
} multiplayer_room_shared_data_t;
/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int log_event(const char *file_path, const char *message);
int **getMatrixFromJSON(cJSON *board);
