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

