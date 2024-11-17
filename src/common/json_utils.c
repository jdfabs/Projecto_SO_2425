/*********************************************************************************
 * json_utils.c
 * Skipper
 * 25/10/2025
 * cJSON aux crap
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"

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

/************************************
 * STATIC FUNCTIONS
 ************************************/

cJSON *get_board_by_id(cJSON *boards, int id) {
    // Check if boards is not NULL and is an array
    if (boards == NULL || !cJSON_IsArray(boards)) {
        return NULL;
    }

    // Iterate through the array to find the board with the specified id
    cJSON *board = NULL;
    cJSON_ArrayForEach(board, boards) {
     const cJSON *board_id = cJSON_GetObjectItem(board, "id");
        if (cJSON_IsNumber(board_id) && board_id->valueint == id) {
            return board; // Return the matching board
        }
    }
    // If no matching board is found, return NULL
    return NULL;
}
cJSON *get_board_state_by_id(cJSON *boards, int state, int id ) {
 const cJSON *board = get_board_by_id(boards, id);

    if(state == INITIAL_STATE) {
        return cJSON_GetObjectItem(board, "starting_state");
    }
    return cJSON_GetObjectItem(board, "solution");
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
