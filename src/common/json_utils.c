//
// Created by skipper on 25-10-2024.
//

/*********************************************************************************
 * file_handler.c
 * Skipper
 * 11/10/2024
 * file handling logic
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include <stdio.h>
#include <stdlib.h>
#include "file_handler.h"
#include <time.h>
#include <string.h>
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
        cJSON *board_id = cJSON_GetObjectItem(board, "id");
        if (cJSON_IsNumber(board_id) && board_id->valueint == id) {
            return board; // Return the matching board
        }
    }

    // If no matching board is found, return NULL
    return NULL;
}
cJSON *get_board_state_by_id(cJSON *boards, int state, int id ) {
    cJSON *board = get_board_by_id(boards, id);
    cJSON * board_state = NULL;

    if(state == INITIAL_STATE) {
        return cJSON_GetObjectItem(board, "starting_state");
    }
    return cJSON_GetObjectItem(board, "solution");
}
