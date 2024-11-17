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
#include "../../include/file_handler.h"
#include <time.h>
#include <string.h>
#include "../../include/common.h"

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


//LOG EVENT FUNCTIONS - CHECKED
int log_event(const char *file_path, const char *message) {
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

cJSON *load_boards(char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s\n", path);
        return NULL;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the file content
    char *file_content =  malloc(file_size + 1);
    if (file_content == NULL) {
        printf("Error: Could not allocate memory\n");
        fclose(file);
        return NULL;
    }

    // Read the file content into memory
    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0'; // Null-terminate the string
    fclose(file);

    // Parse the JSON content
    cJSON *boards_json = cJSON_GetObjectItem(cJSON_Parse(file_content), "sudoku_boards");
    if (boards_json == NULL) {
        printf("Error: Could not parse JSON\n");
        free(file_content);
        return NULL;
    }


    // Clean up
    free(file_content);
    return boards_json;
}



// Utility function to construct file path
void construct_file_path(char *buffer, int id) {
    snprintf(buffer, MAX_PATH_LENGTH, "%s%d.json", BOARD_DIR, id);
}
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