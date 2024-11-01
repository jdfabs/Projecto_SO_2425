/*********************************************************************************
 * common.h
 * Skipper
 * 11/10/2024
 * common functions  
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/


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
 char message[MESSAGE_SIZE];
} client_message;
/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int log_event(const char *file_path, const char *message);
