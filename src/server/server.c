/*********************************************************************************
 * server.c
 * Skipper
 * 11/10/2024
 * Server entry-point
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include "server.h"
#include "../common/common.h"
#include "file_handler.h"
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

int validateBoard(int **board);

void printBoard(int **matrix);
void solve_by_brute_force(int **matrix, int **solution);

/************************************
 * STATIC FUNCTIONS
 ************************************/



int main(int argc, char *argv[]) {

    if(argc == 1) {
        printf("Usage: ./server [CONFIG_FILE_NAME]\n");
        exit(-1);
    }

    printf("Starting server...\n");

    if(argc > 0 && load_server_config(&config,argv[1]) >= 0) {
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

    log_event(config.log_file, "Server Started");

    log_event(config.log_file, "Server Config Loaded");





    int **matrix;
    matrix = getMatrixFromJSON(get_board_state_by_id(0,STARTING_STATE));

    cJSON *startingStateMatrix = matrix_to_JSON(matrix);
    printf("Reset current state\n");
    save_boards_file(update_boards_with_new_board(startingStateMatrix,0,CURRENT_STATE));

    printf("FICHEIRO JSON CURRENT STATE\n");
    printf(cJSON_Print(get_board_state_by_id(0,CURRENT_STATE)));

    int **solution;
    solution = getMatrixFromJSON(get_board_state_by_id(0,END_STATE));

    printBoard(matrix);
    printf("\nStarting to solve from initial state");
    solve_by_brute_force(matrix, solution);

    cJSON *finishedMatrix = matrix_to_JSON(matrix);
    printf("saving matrix to file\n");
    save_boards_file(update_boards_with_new_board(finishedMatrix, 0, CURRENT_STATE));


    //cJSON *board = get_board_state_by_id(0,STARTING_STATE);
    //printf("Board state:\n");
    //save_boards_file(update_boards_with_new_board(update_sudoku_board(board,9,2,0),0,CURRENT_STATE));
    //printf(cJSON_Print(get_board_state_by_id(0,CURRENT_STATE)));
    exit(0);



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

    validateBoard(testBoard);


    exit(0);
}

int validateBoard(int **board) {
    //Validation Function is less computationaly expensive
    //So it's first checked if it's valid and only if not checked how many cells are possibly wrong
    int wrong = 0;
    if (isValidSudoku(board)) {
        printf("The Sudoku board is valid!\n");
        log_event(config.log_file, "Sudoku board verified with success");
        return wrong;
    }
    printf("The Sudoku board is invalid!\n");
    wrong = wrongCellsCounter(board);
    printf("has %d possibly wrong cells\n", wrong);

    char log_message[100];
    sprintf(log_message, "Sudoku board verified without success. %d possibly wrong cells", wrong);

    log_event(config.log_file, log_message);
    return wrong;
}

void printBoard(int **matrix) {
    printf("The Sudoku board is:\n");
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            printf("%d ", matrix[i][j]); // Print the number
            // Print a vertical separator for the 3x3 blocks
            if ((j + 1) % 3 == 0 && j != 8) {
                printf("| ");
            }
        }
        printf("\n"); // Move to a new line after each row
        // Print a horizontal separator for the 3x3 blocks
        if ((i + 1) % 3 == 0 && i != 8) {
            printf("---------------------\n");
        }
    }
}


void solve_by_brute_force(int **matrix, int **solution) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (matrix[i][j] == 0) {
                printf("cell (%d,%d) is empty\n", i, j);
                for (int k = 1; k <= SIZE; k++) {
                    printf("trying %d\n", k);
                    if(k == solution[i][j]) {
                        matrix[i][j] = k;
                        printf("Correct number found\n");
                        printBoard(matrix);
                        
                        break;
                    }
                }
            }
        }
    }
}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/
