/**********************************************ODO***********************************
 * client.h
 * Skipper
 * 11/10/2024
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/
typedef struct  {
    char id[19];
    char server_ip[15];
    int server_port;
    char log_file[254];
    int game_type;
    int slow_factor;
} client_config;

#define BOARD_SIZE 9

/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

int load_client_config(const char *filename, client_config *config);
void solve_by_brute_force(int matrix[][BOARD_SIZE], int solution[][BOARD_SIZE]);
void printBoard(int **matrix);