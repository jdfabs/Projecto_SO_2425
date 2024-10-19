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

int validateBoard(int **board);

void printBoard(int **matrix);
void solve_by_brute_force(int **matrix, int **solution);

/************************************
 * STATIC FUNCTIONS
 ************************************/



int main(int argc, char *argv[]) {

    // Helper To how to run the server
    if(argc == 1) {
        printf("Usage: ./server [CONFIG_FILE_NAME]\n");
        exit(EXIT_FAILURE);
    }


    printf("Starting server...\n");

    if(load_server_config(argv[1],&config) >= 0) {
        printf("............................................\n");
        printf("Config Servidor Carregado Correctamente:\n");
        printf("Server IP: %s\n", config.ip);
        printf("Port: %d\n", config.port);
        printf("Logging: %d\n", config.logging);
        printf("Log File: %s\n", config.log_file);
        printf("Log Level: %d\n", config.log_level);
        printf("Max Clients: %d\n", config.max_clients);
        printf("Backup Interval: %d\n", config.backup_interval);
        printf("............................................\n");
    } else {
        fprintf(stderr, "Failed to load server configuration.\n");
        exit(EXIT_FAILURE);
    }
    log_event(config.log_file, "Servidor começou");





    int **matrix;
    matrix = getMatrixFromJSON(get_board_state_by_id(1,STARTING_STATE));

    cJSON *startingStateMatrix = matrix_to_JSON(matrix);
    printf("\nReset current state\n");
    save_board_to_file(update_boards_with_new_board(startingStateMatrix,1,CURRENT_STATE),1);

    printf("\nFICHEIRO JSON CURRENT STATE\n");
    printf(cJSON_Print(get_board_state_by_id(1,CURRENT_STATE)));

    int **solution;
    solution = getMatrixFromJSON(get_board_state_by_id(1,END_STATE));

    printBoard(matrix);
    printf("\nStarting to solve from initial state\n");
    solve_by_brute_force(matrix, solution);

    cJSON *finishedMatrix = matrix_to_JSON(matrix);
    printf("\nsaving matrix to file\n");
    save_board_to_file(update_boards_with_new_board(finishedMatrix, 1, CURRENT_STATE),1);

    exit(0);
}

//TEMPORARIO PARA DEMONSTRAÇÂO !!!! vai mudar para cliente

void printBoard(int **matrix) {
    printf("\nQuadro de Sudoku:\n");
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
    printf("\n");
}


void solve_by_brute_force(int **matrix, int **solution) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (matrix[i][j] == 0) {
                printf("celula (%d,%d) está vazia\n", i, j);
                for (int k = 1; k <= SIZE; k++) {
                    printf("a tentar %d\n", k);
                    if(k == solution[i][j]) {
                        matrix[i][j] = k;
                        printf("Numero correto encontrado\n");
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
