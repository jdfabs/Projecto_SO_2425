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
#include <stdlib.h>
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
bool isValidSudoku(int **board)
{
    for (int i = 0; i < SIZE; i++)
    {
        int row[9];
        int col[9];
        int square[9];

        for (int j = 0; j < SIZE; j++)
        {
            col[j] = board[i][j]; // fill col #i
            row[j] = board[j][i]; // fill row #i
        }
        // printf("starting %d th check",i+1);
        if (!isValidGroup(col))
            return false;
        // printf("col %d valid", i+1);
        if (!isValidGroup(row))
            return false;
        // printf("row %d valid", i+1);

        int startRow = i / 3 * 3;
        int startCol = i % 3 * 3;

        int counter = 0;
        for (int r = 0; r < 3; r++)
        { // fill square #i
            for (int c = 0; c < 3; c++)
            {
                square[counter++] = board[startRow + r][startCol + c];
            }
        }
        if (isValidGroup(square))
            return false;
        // printf("square %d valid\n", i+1);
    }
    return true;
}
int wrongCellsCounter(int **board)
{
    int wrongCount = 0;

    for (int i = 0; i < SIZE; i++)
    { // foreach row
        for (int j = 0; j < SIZE; j++)
        { // foreach col
            if (board[i][j] != 0)
            { // 0's is unfill, never wrong
                // Check if the cell violates any constraints
                if (!isCellValidInRow(board, i, j) ||
                    !isCellValidInCol(board, i, j) ||
                    !isCellValidInSquare(board, i, j))
                {
                    wrongCount++;
                }
            }
        }
    }

    return wrongCount;
}

// AUX FUNCS

// Helper to Board Validador
bool isValidGroup(int group[SIZE])
{ // 0 implies not filled in
    bool seen[SIZE] = {false};

    for (int i = 0; i < SIZE; i++)
    { // foreach check if duped
        if (group[i] != 0)
        {
            if (seen[group[i] - 1] || group[i] > 9)
            {

                return false; // seen | invalid value -> return false
            }
            seen[group[i] - 1] = true;
        }
    }

    return true;
}

// Helpers To Know Wrong Cells counter
bool isCellValidInRow(int **board, int row, int col)
{
    int num = board[row][col];
    for (int i = 0; i < SIZE; i++)
    { // for each cell in row
        if (i != col && board[row][i] == num)
        {
            return false;
        }
    }
    return true;
}
bool isCellValidInCol(int **board, int row, int col)
{
    int num = board[row][col];
    for (int i = 0; i < SIZE; i++)
    { // foreach cell in col
        if (i != row && board[i][col] == num)
        {
            return false;
        }
    }
    return true;
}
bool isCellValidInSquare(int **board, int row, int col)
{
    int num = board[row][col];
    int startRow = row / 3 * 3;
    int startCol = col / 3 * 3;
    for (int i = startRow; i < startRow + 3; i++)
    { // rows from aprop
        for (int j = startCol; j < startCol + 3; j++)
        { // cols from aprop
            if ((i != row || j != col) && board[i][j] == num)
            {
                return false;
            }
        }
    }
    return true;
}



int **getMatrixFromJSON(cJSON *board)
{
    int **matrix = malloc(9 * sizeof(int *));
    for (int i = 0; i < 9; i++)
    {
        matrix[i] = (int *)malloc(9 * sizeof(int));
    }

    for (int i = 0; i < 9; i++)
    {
        cJSON *row = cJSON_GetArrayItem(board, i);
        if (row == NULL || !cJSON_IsArray(row))
        {
            fprintf(stderr, "Row %d is not an array\n", i);
            for (int j = 0; j <= i; j++)
            {
                free(matrix[j]);
            }
            free(matrix);
            return NULL;
        }

        // Fill the matrix row with values
        for (int j = 0; j < 9; j++)
        {
            cJSON *value = cJSON_GetArrayItem(row, j);
            if (value == NULL || !cJSON_IsNumber(value))
            {
                fprintf(stderr, "Value at row %d, column %d is not a number\n", i, j);
                for (int k = 0; k <= i; k++)
                {
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


bool is_safe(int **grid, int row, int col, int num) {
    for (int x = 0; x < SIZE; x++) {
        if (grid[row][x] == num) {
            return false;
        }
    }

    for (int x = 0; x < SIZE; x++) {
        if (grid[x][col] == num) {
            return false;
        }
    }

    int startRow = row - row % 3;
    int startCol = col - col % 3;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (grid[i + startRow][j + startCol] == num) {
                return false;
            }
        }
    }

    return true;
}

bool fill_sudoku(int **grid, int row, int col) {
    if (row == SIZE - 1 && col == SIZE) {
        return true;
    }

    if (col == SIZE) {
        row++;
        col = 0;
    }
    if (grid[row][col] != 0) {
        return fill_sudoku(grid, row, col + 1);
    }
    for (int num = 1; num <= SIZE; num++) {
        if (is_safe(grid, row, col, num)) {
            grid[row][col] = num;
            if (fill_sudoku(grid, row, col + 1)) {
                return true;
            }
            grid[row][col] = 0;
        }
    }

    return false;
}

void print_grid(int **grid) {
    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++) {
            printf("%2d ", grid[row][col]);
        }
        printf("\n");
    }
}

void fill_diagonal(int **grid) {
    for (int i = 0; i < SIZE; i += 3) {
        int num;
        bool used[SIZE + 1] = {false};
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                do {
                    num = rand() % 9 + 1;
                } while (used[num]);
                used[num] = true;
                grid[i + j][i + k] = num;
            }
        }
    }
}

int **generate_sudoku() { 
    // Allocate memory for the grid dynamically
    int **grid = malloc(SIZE * sizeof(int *));
    for (int i = 0; i < SIZE; i++) {
        grid[i] = malloc(SIZE * sizeof(int));
    }

    // Initialize the grid with zeros
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            grid[i][j] = 0;
        }
    }

    // Fill the grid as required
    fill_diagonal(grid);
    fill_sudoku(grid, 0, 0);

    // Print the Sudoku grid
    /*
    printf("\nQuadro de Sudoku:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", grid[i][j]);
            if ((j + 1) % 3 == 0 && j != 8) {
                printf("| ");
            }
        }
        printf("\n");
        if ((i + 1) % 3 == 0 && i != 8) {
            printf("---------------------\n");
        }
    }
    printf("\n");
    */

    
    int **grid_sol = grid;

    /*
    printf("\nQuadro de Sudoku:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", grid_sol[i][j]);
            if ((j + 1) % 3 == 0 && j != 8) {
                printf("| ");
            }
        }
        printf("\n");
        if ((i + 1) % 3 == 0 && i != 8) {
            printf("---------------------\n");
        }
    }
    printf("\n");
    */

    // 
    for (size_t i = 0; i < SIZE; i++)
    {
        for (size_t j = 0; j < SIZE; j++)
        {
            int prob = rand() % 100;
            
            //printf("%d\n",prob);
            
            if (prob<65){
                grid[i][j]=0;
            }
        }
    }
    
    printf("\nQuadro de Sudoku:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", grid[i][j]);
            if ((j + 1) % 3 == 0 && j != 8) {
                printf("| ");
            }
        }
        printf("\n");
        if ((i + 1) % 3 == 0 && i != 8) {
            printf("---------------------\n");
        }
    }
    printf("\n");
    
    return grid, grid_sol;
}





/*
int **create_new_board()
{

    int new_board[SIZE][SIZE];
    int board_array[81] = {5,3,4,6,7,8,9,1,2,6,7,2,1,9,5,3,4,8,1,9,8,3,4,2,5,6,7,8,5,9,7,6,1,4,2,3,4,2,6,8,5,3,7,9,1,7,1,3,9,2,4,8,5,6,9,6,1,5,3,7,2,8,4,2,8,7,4,1,9,6,3,5,3,4,5,2,8,6,1,7,9};

    
    
    for (int i = 80; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = board_array[i];
        board_array[i] = board_array[j];
        board_array[j] = temp;
    }
    
    for (int i = 0; i < 81; i++)
    printf("%d\n", board_array[i]);

    int counter = 0;
    for (int i; i < 9; i++){
        for(int j; j<9 ; j++){
            new_board[i][j] = board_array[counter];
            counter++;
        }

    }

    printf("\nQuadro de Sudoku:\n");
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			printf("%d ", new_board[i][j]); // Print the number
			// Print a vertical separator for the 3x3 blocks
			if ((j + 1) % 3 == 0 && j != 8) {
				printf("| ");
			}
		}
		printf("\n"); // Move to a new line after each row
		// Print a horizontal separator for the 3x3 blocks
		if ((i + 1) % 3 == 0 && i != 8) {
			printf("---------------------\n");
		}
	}
	printf("\n");

    return new_board;
}

*/