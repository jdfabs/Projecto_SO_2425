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
#include <file_handler.h>
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



//GET cJSON OBJECTS
char* get_board_file_string(int id){
    
    char filepath[100];  
    sprintf(filepath, "./boards/%d.json", id);

    printf(filepath);
    FILE *file = fopen(filepath, "r");
    if (file == NULL){
        printf("Erro ao abrir o ficheiro.\n");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = (char*)malloc(length+1);
    if (content == NULL)
    {
        printf("Erro ao alocar memória.\n");
        fclose(file);
        return NULL;
    }
    
    fread(content, 1, length, file);
    fclose(file);
    content[length] = '\0';

    return content;
}

cJSON* get_json_from_string(const char* json_string) {
    cJSON *json = cJSON_Parse(json_string);
    return json;
}

cJSON* get_board_by_id(int id) {
    return get_json_from_string(get_board_file_string(id));

}

cJSON* get_board_state_by_id(int id, int state) {
    switch (state) {
        case STARTING_STATE:
            return cJSON_GetObjectItem(get_board_by_id(id), "new");
        case CURRENT_STATE:
            return cJSON_GetObjectItem(get_board_by_id(id), "current");
        case END_STATE:
            return cJSON_GetObjectItem(get_board_by_id(id), "solution");
        default:
            printf("Invalid state.\n");
            break;
    }
    return NULL;

}

//UPDATE cJSON OBJECTS

cJSON* update_boards_with_new_board(cJSON *newBoard, int index, int state) {
    cJSON *boardToUpdate = get_board_by_id(index);
    printf("000\n");
    switch (state) {
        case 0: // Update 'new' state
            cJSON_ReplaceItemInObject(boardToUpdate, "new", newBoard);
            break;
        case 1: // Update 'current' state
            cJSON_ReplaceItemInObject(boardToUpdate, "current", newBoard);
            break;
        case 2: // Update 'solution' state
            cJSON_ReplaceItemInObject(boardToUpdate, "solution", newBoard);
            break;
        default:
            fprintf(stderr, "Invalid state: %d\n", state);
        return NULL;
    }
    printf(cJSON_Print(boardToUpdate));

    return boardToUpdate;
}

//CREATE NEW cJSON BOARDS

//WRITE cJSON TO FILE
int save_board_to_file(cJSON *board_json, int id) {
    printf("Saving board %d\n", id);
    if (!board_json) {
        fprintf(stderr, "Error: NULL cJSON object provided.\n");
        return -1; // Indicate failure due to NULL pointer
    }

    char filepath[100];
    sprintf(filepath, "./boards/%d.json", id);
    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Error opening file for writing");
        return -1; // Indicate failure to open the file
    }

    // Convert the cJSON object to a string
    char *json_string = cJSON_Print(board_json);
    if (!json_string) {
        fprintf(stderr, "Error printing cJSON object to string.\n");
        fclose(file);
        return -1; // Indicate failure to print JSON
    }

    // Write the JSON string to the file
    if (fputs(json_string, file) == EOF) {
        fprintf(stderr, "Error writing JSON string to file.\n");
        free(json_string); // Free the string allocated by cJSON_Print
        fclose(file); // Close the file
        return -1; // Indicate failure to write to the file
    }

    // Free allocated memory
    free(json_string); // Free the string allocated by cJSON_Print
    fclose(file); // Close the file
    return 0; // Indicate success
}



cJSON* matrix_to_JSON(int **matrix) {

    cJSON *matrix_json = cJSON_CreateArray();
    for (int i = 0; i < 9; i++) {
        // Create a new cJSON array for the current row
        cJSON *row = cJSON_CreateArray();

        // Loop to add elements to the current row
        for (int j = 0; j < 9; j++) {
            cJSON_AddItemToArray(row, cJSON_CreateNumber(matrix[i][j]));
        }

        // Add the row to the main matrix array
        cJSON_AddItemToArray(matrix_json, row);
    }
    return matrix_json;
}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/

