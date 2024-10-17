/*********************************************************************************
 * soduko_solver.c
 * Skipper
 * 11/10/2024
 * Sudoku solver
 *********************************************************************************/


/************************************
 * INCLUDES
 ************************************/
#include <stdio.h>
#include <stdbool.h>
#include "server.h"



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
 * FUNCTION PROTOTYPES
 ************************************/
int wrongCellsCounter(int **board);
bool isCellValidInRow(int **board, int row, int col);
bool isCellValidInCol(int **board, int row, int col);
bool isCellValidInSquare(int **board, int row, int col);

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

// PARTIAL ALGORITHIM REF: https://medium.com/strategio/sudoku-validator-algorithm-dc848cb45093
bool isValidSudoku(int **board){
    for(int i = 0; i < SIZE ; i++){
        int row[9];
        int col[9];
        int square[9];

        for(int j = 0; j < SIZE; j++){
            col[j] = board[i][j];   //fill col #i
            row[j] = board[j][i];   //fill row #i
        }
        //printf("starting %d th check",i+1);
        if (!isValidGroup(col)) return false;
        //printf("col %d valid", i+1);
        if (!isValidGroup(row) ) return false;
        //printf("row %d valid", i+1);

        int startRow = (i / 3) * 3;  
        int startCol = (i % 3) * 3;  
        
        int counter = 0;
        for (int r = 0; r < 3; r++) { // fill square #i
            for (int c = 0; c < 3; c++) {
                square[counter++] = board[startRow + r][startCol + c];
            }
        }
        if (!isValidGroup(square)) return false;
        //printf("square %d valid\n", i+1);
    } 
    return true;
}
int wrongCellsCounter(int **board) {
    int wrongCount = 0;

    
    for (int i = 0; i < SIZE; i++) {//foreach row
        for (int j = 0; j < SIZE; j++) { //foreach col
            if (board[i][j] != 0) { //0's is unfill, never wrong
                // Check if the cell violates any constraints
                if (!isCellValidInRow(board, i, j) ||
                    !isCellValidInCol(board, i, j) ||
                    !isCellValidInSquare(board, i, j)) {
                    wrongCount++;
                }
            }
        }
    }

    return wrongCount;
}

//AUX FUNCS

//Helper to Board Validador
bool isValidGroup(int group[SIZE]){ // 0 implies not filled in
    bool seen[SIZE] = { false };
    
    for (int i = 0; i < SIZE; i++) {    //foreach check if duped
        if (group[i] != 0) {  
            if (seen[group[i] - 1] || group[i]>9) {
                
                return false;   //seen | invalid value -> return false
            }
            seen[group[i] - 1] = true;  
        }
    }
    
    return true; 
}

// Helpers To Wrong Cell counter
bool isCellValidInRow(int **board, int row, int col) {
    int num = board[row][col];
    for (int i = 0; i < SIZE; i++) { //for each cell in row
        if (i != col && board[row][i] == num) {
            return false;
        }
    }
    return true;
}
bool isCellValidInCol(int **board, int row, int col) {
    int num = board[row][col];
    for (int i = 0; i < SIZE; i++) { //foreach cell in col
        if (i != row && board[i][col] == num) {
            return false;
        }
    }
    return true;
}
bool isCellValidInSquare(int **board, int row, int col) {
    int num = board[row][col];
    int startRow = (row / 3) * 3;
    int startCol = (col / 3) * 3;
    for (int i = startRow; i < startRow + 3; i++) { //rows from aprop
        for (int j = startCol; j < startCol + 3; j++) { //cols from aprop
            if ((i != row || j != col) && board[i][j] == num) {
                return false; 
            }
        }
    }
    return true;
}


cJSON* update_sudoku_board(cJSON *currentBoard, int value, int x, int y) {

    // Get the row at index y
    cJSON *row = cJSON_GetArrayItem(currentBoard, y);
    if (!cJSON_IsArray(row)) {
        printf("Error: Row %d is not an array\n", y);
        return NULL;
    }

    // Get the current cell value at index x and update it
    cJSON *cell = cJSON_GetArrayItem(row, x);
    if (!cJSON_IsNumber(cell)) {
        printf("Error: Cell at (%d, %d) is not a number\n", x, y);
        return NULL;
    }

    // Update the cell value
    cJSON_SetNumberValue(cell, value);

    // Return the updated board
    return currentBoard;
}

int** getMatrixFromJSON(cJSON *board) {
    int **matrix = (int **)malloc(9 * sizeof(int *));
    for (int i = 0; i < 9; i++) {
        matrix[i] = (int *)malloc(9 * sizeof(int));
    }
    printf(cJSON_Print(board));
    // Iterate over the cJSON array
    for (int i = 0; i < 9; i++) {
        // Get the i-th element (which is also an array)
        cJSON *row = cJSON_GetArrayItem(board, i);
        if (row == NULL || !cJSON_IsArray(row)) {
            fprintf(stderr, "Row %d is not an array\n", i);
            for (int j = 0; j <= i; j++) {
                free(matrix[j]);
            }
            free(matrix);
            return NULL;
        }

        // Fill the matrix row with values
        for (int j = 0; j < 9; j++) {
            cJSON *value = cJSON_GetArrayItem(row, j);
            if (value == NULL || !cJSON_IsNumber(value)) {
                fprintf(stderr, "Value at row %d, column %d is not a number\n", i, j);
                for (int k = 0; k <= i; k++) {
                    free(matrix[k]);
                }
                free(matrix);
                return NULL;
            }
            matrix[i][j] = value->valueint;
        }
    }

    return matrix;
}