/*********************************************************************************
 * file_handler.h
 * Skipper
 * 11/10/2024
 * file handler includes  
 *********************************************************************************/


/************************************
 * INCLUDES
 ************************************/

#include "cJSON.h"


/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/


#define STARTING_STATE 0
#define CURRENT_STATE 1
#define END_STATE 2
/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/






int read_file_to_string(char *filepath, char **data);
int parse_json(const char *data, cJSON **json);



cJSON *load_boards(char *path);
cJSON *get_board_state_by_id(cJSON *boards, int state, int id );
