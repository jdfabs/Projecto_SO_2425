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
#define BOARD_DIR "./boards/"
#define MAX_PATH_LENGTH 100

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


//LOG EVENT FUNCTIONS
int log_event(const char *file_path, const char *message){
    FILE *file = fopen(file_path, "a");
    if (file == NULL) {
        fprintf(stderr, "Could not open log file %s\n", file_path);
        return -1; 
    }

    const time_t now = time(NULL); //get current time
    char *timestamp = ctime(&now); //string timestamp
    timestamp[strcspn(timestamp, "\n")] = 0; //Remove new line

    fprintf(file, "[%s] %s\n", timestamp, message);

    fclose(file);
    return 0;
}


// Utility function to construct file path
void construct_file_path(char *buffer, int id) {
    snprintf(buffer, MAX_PATH_LENGTH, "%s%d.json", BOARD_DIR, id);
}

// Utility function to read file into string
int read_file_to_string( char *filepath, char **data) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("File not found: %s\n", filepath);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    *data = (char *)malloc(length + 1);
    if (!*data) {
        printf("Error allocating memory to read file: %s\n", filepath);
        fclose(file);
        return -2;
    }

    fread(*data, 1, length, file);
    (*data)[length] = '\0';
    fclose(file);
    printf("File read to string successfully\n");
    return 0;
}

// Function to get JSON string from file
char *get_board_file_string(int id) {
    char filepath[MAX_PATH_LENGTH];
    construct_file_path(filepath, id);

    char *content = NULL;
    if (read_file_to_string(filepath, &content) != 0) {
        return NULL;
    }
    return content;
}

// Function to parse JSON string into cJSON object
cJSON *get_json_from_string(const char *json_string) {
    cJSON *json = cJSON_Parse(json_string);
    if (!json) {
        printf("Error parsing JSON string.\n");
    }
    return json;
}

// Function to get board by ID
cJSON *get_board_by_id(int id) {
    char *json_string = get_board_file_string(id);
    if (!json_string) {
        return NULL;
    }
    cJSON *json = get_json_from_string(json_string);
    free(json_string); // Free the JSON string after parsing
    return json;
}

// Function to get board state by ID
cJSON *get_board_state_by_id(int id, int state) {
    cJSON *board = get_board_by_id(id);
    if (!board) {
        return NULL;
    }

    const char *state_keys[] = {"new", "current", "solution"};
    if (state < 0 || state > 2) {
        printf("Invalid state.\n");
        return NULL;
    }

    return cJSON_GetObjectItem(board, state_keys[state]);
}

// Function to update boards with new board
cJSON *update_boards_with_new_board(cJSON *newBoard, int index, int state) {
    cJSON *boardToUpdate = get_board_by_id(index);
    if (!boardToUpdate) {
        return NULL;
    }

    const char *state_keys[] = {"new", "current", "solution"};
    if (state < 0 || state > 2) {
        fprintf(stderr, "Invalid state: %d\n", state);
        return NULL;
    }

    cJSON_ReplaceItemInObject(boardToUpdate, state_keys[state], newBoard);
    return boardToUpdate;
}

// Function to save cJSON object to file
int save_board_to_file(cJSON *board_json, int id) {
    if (!board_json) {
        fprintf(stderr, "Error: NULL cJSON object provided.\n");
        return -1;
    }

    char filepath[MAX_PATH_LENGTH];
    construct_file_path(filepath, id);

    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Error opening file for writing");
        return -1;
    }

    char *json_string = cJSON_Print(board_json);
    if (!json_string) {
        fprintf(stderr, "Error printing cJSON object to string.\n");
        fclose(file);
        return -1;
    }

    if (fputs(json_string, file) == EOF) {
        fprintf(stderr, "Error writing JSON string to file.\n");
        free(json_string);
        fclose(file);
        return -1;
    }

    free(json_string);
    fclose(file);
    return 0; // Indicate success
}

// Function to convert a matrix to cJSON
cJSON *matrix_to_JSON(int **matrix) {
    cJSON *matrix_json = cJSON_CreateArray();
    for (int i = 0; i < 9; i++) {
        cJSON *row = cJSON_CreateArray();
        for (int j = 0; j < 9; j++) {
            cJSON_AddItemToArray(row, cJSON_CreateNumber(matrix[i][j]));
        }
        cJSON_AddItemToArray(matrix_json, row);
    }
    return matrix_json;
}

// Function to parse JSON data
int parse_json(const char *data, cJSON **json) {
    *json = get_json_from_string(data);
    if (!*json) {
        return -1; // Indicate parsing error
    }
    printf("JSON parsed successfully\n");
    return 0; // Indicate success
}