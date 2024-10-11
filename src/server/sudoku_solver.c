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
 * STATIC FUNCTION PROTOTYPES
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

// PARTIAL ALGORITHIM REF: https://medium.com/strategio/sudoku-validator-algorithm-dc848cb45093


bool isValidSudoku(int board[SIZE][SIZE]){
    for(int i = 0; i < SIZE ; i++){
        int row[9];
        int col[9];
        int square[9];

        for(int j = 0; j < SIZE; j++){
            col[j] = board[i][j];   //fill col #i
            row[j] = board[j][i];   //fill row #i
        }
        printf("starting %d th check",i+1);
        if (!isValidGroup(col)) return false;
        printf("col %d valid", i+1);
        if (!isValidGroup(row) ) return false;
        printf("row %d valid", i+1);

        int startRow = (i / 3) * 3;  
        int startCol = (i % 3) * 3;  
        
        int counter = 0;
        for (int r = 0; r < 3; r++) { // fill square #i
            for (int c = 0; c < 3; c++) {
                square[counter++] = board[startRow + r][startCol + c];
            }
        }

        if (!isValidGroup(square)) return false;
          printf("square %d valid\n", i+1);

       
        
    } 
    return true;
}

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