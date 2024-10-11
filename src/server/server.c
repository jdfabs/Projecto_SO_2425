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
 ServerConfig config;
/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
ServerConfig load_default_config();
int validadeBoard(int board[SIZE][SIZE]);


/************************************
 * STATIC FUNCTIONS
 ************************************/

int main(int argc, char *argv[]) {
    
   

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
        {1, 3, 4, 6, 7, 8, 9, 1, 2},
        {6, 7, 2, 1, 9, 5, 3, 4, 8},
        {0, 9, 8, 3, 4, 2, 5, 6, 7},
        {8, 5, 9, 7, 6, 1, 4, 2, 3},
        {4, 2, 6, 8, 5, 3, 7, 9, 1},
        {7, 1, 3, 9, 2, 4, 8, 5, 6},
        {9, 6, 1, 5, 3, 7, 2, 8, 4},
        {2, 8, 7, 4, 1, 9, 6, 3, 5},
        {3, 4, 5, 2, 8, 6, 1, 7, 9}
    };

    validadeBoard(testBoard);
    

    exit(0);
     
}

int validadeBoard(int board[SIZE][SIZE]){
    //Validation Function is less computationaly expensive
    //So it's first checked if it's valid and only if not checked how many cells are possibly wrong
    int wrong;
    if (isValidSudoku(board)) {
        printf("The Sudoku board is valid!\n");
        log_event(config.log_file, "Sudoku board verified with success");
        return wrong;
    } else {
        printf("The Sudoku board is invalid!\n");
        wrong = wrongCellsCounter(board);
        printf("has %d possibly wrong cells\n", wrong);

        char log_message[100];
        sprintf(log_message, "Sudoku board verified without success. %d possibly wrong cells", wrong);

        log_event(config.log_file, log_message);
    }
    return wrong;
}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/
