/*********************************************************************************
 * server.c
 * Skipper
 * 11/10/2024
 * Server entry-point
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <file_handler.h>
#include "server.h"
#include "../common/common.h"
/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
ServerConfig load_default_config();

/************************************
 * STATIC FUNCTIONS
 ************************************/

int main(int argc, char *argv[]) {
    
    ServerConfig config;

    if(load_server_config(&config)>=0){
        printf("Client IP: %s\n", config.ip);
        printf("LogFile: %s\n", config.log_file);
        printf("LogLevel: %d\n", config.log_level);
        printf("Logging: %d\n", config.logging);
        printf("Max_Clients: %d\n", config.max_clients);
        printf("Port: %d\n", config.port);

    } else {
        fprintf(stderr, "Failed to load server configuration.\n");
        exit(-1);
    }

    log_event(config.log_file,"Server Started");
    log_event(config.log_file,"Server Config Loaded");

    int testBoard[SIZE][SIZE] = 
    {
        {0, 0, 4, 6, 7, 8, 9, 1, 2},
        {6, 7, 2, 1, 9, 5, 3, 4, 8},
        {1, 9, 8, 3, 4, 2, 5, 6, 7},
        {8, 5, 9, 7, 6, 1, 4, 2, 3},
        {4, 2, 6, 8, 5, 3, 7, 9, 1},
        {7, 1, 3, 9, 2, 4, 8, 5, 6},
        {9, 6, 1, 5, 3, 7, 2, 8, 4},
        {2, 8, 7, 4, 1, 9, 6, 3, 5},
        {3, 4, 5, 2, 8, 6, 1, 7, 9}
    };

    if (isValidSudoku(testBoard)) {
        printf("The Sudoku board is valid!\n");
        log_event(config.log_file, "Sudoku board verified with success");
    } else {
        printf("The Sudoku board is invalid!\n");
        log_event(config.log_file, "Sudoku board verified without success");
    }


    exit(0);
     
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
