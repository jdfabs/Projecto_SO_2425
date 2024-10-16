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
#include "cJSON.h"

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
void testFunction(){
    printf("000\n");
}

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

char* readBoards(const char* filename){
    FILE* file = fopen(filename, "r");
    if (file == NULL){
        printf("Erro ao abrir o ficheiro.\n");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = (char*)malloc(length+1);
    if (content == NULL)
    {
        printf("Erro ao alocar memóroa.\n");
        fclose(file);
        return -1;
    }
    
    fread(content, 1, length, file);
    fclose(file);
    content[length] = '\0';

    return content;
    
}

int** getMatrizFromJSON (const char* jsonString, const char* boardName, const char* matrixType){
    cJSON* json = cJON_Parse(jsonString);
    if (json == NULL){
        printf("Erro ao encontrar 'boards'.\n");
        return -1;
    }

    cJSON* boards = cJSON_GetObjectItem(json, "boards"); 

    if (boards == NULL) {
        printf("Erro ao encontrar 'borads'.\n");
        cJSON_Delete(json);
        return -1; 
    }
    
    cJSON* board = NULL;

    cJSON_ArrayForEach(board, boards) {
        cJSON* boardID = cJSON_GetObjectItem(board, boardName);
        
        if (boardID) {
            cJSON* matrix = cJSON_GetObjectItem(boardID, matrixType); // Obter a matriz com o tipo correto
            
            if (matrix) {
                int** matriz = (int**)malloc(9 * sizeof(int*));
                for (int i = 0; i < 9; i++) {
                    matriz[i] = (int*)malloc(9 * sizeof(int));
                    cJSON* row = cJSON_GetArrayItem(matrix, i);
                    if (row == NULL) {
                        printf("Erro ao obter linha %d da matriz.\n", i);
                        // Liberar a memória antes de retornar
                        for (int j = 0; j < i; j++) {
                            free(matriz[j]);
                        }
                        free(matriz);
                        cJSON_Delete(json);
                        return NULL; // Retorna NULL se houver erro
                    }
                    for (int j = 0; j < 9; j++) { // Alterado de count para 9
                        matriz[i][j] = cJSON_GetArrayItem(row, j)->valueint; // Corrigido GetArrayIte para GetArrayItem
                    }
                }
                cJSON_Delete(json);
                return matriz; // Retorna a matriz
            }
        }
    }

    cJSON_Delete(json);
    return -1;
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

