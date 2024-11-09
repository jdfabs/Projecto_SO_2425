/*********************************************************************************
 * coms.c
 * Skipper
 * 19/10/2024
 * Common config loading
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include "common.h"
#include "server.h"
#include "client.h"

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
 * STATIC FUNCTION PROTOTYPE
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/


int parse_json(const char *data, cJSON **json) {
	*json = cJSON_Parse(data);
	if (!*json) {
		return -1; // Indicate parsing error
	}
	//printf("JSON parsed successfully\n");
	return 0; // Indicate success
}
int read_file_to_string(char *filepath, char **data) {
	FILE *file = fopen(filepath, "r");
	if (!file) {
		printf("File not found: %s\n", filepath);
		return -1;
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	*data = (char *) malloc(length + 1);
	if (!*data) {
		printf("Error allocating memory to read file: %s\n", filepath);
		fclose(file);
		return -2;
	}

	fread(*data, 1, length, file);
	(*data)[length] = '\0';
	fclose(file);
	//printf("File read to string successfully\n");
	return 0;
}

void load_default_client_config(client_config *client_config) { //TODO -- fix
	strcpy(client_config->id, "default_client");
	strcpy(client_config->server_ip, "127.0.0.1");
	client_config->server_port = 8080;
	strcpy(client_config->log_file, "./logs/client_default.log");
}
void load_default_server_config(server_config *server_config) { //TODO -- fix
	strcpy(server_config->log_file, "./logs/server_default.log");
	server_config->board_file_path, "./boards/boards.json";
	server_config->task_queue_size = 10;
	server_config->task_handler_threads = 5;
}

int load_client_config(const char *filename, client_config *config) {
	char *data = NULL;
	cJSON *json = NULL;

	printf("Construir caminho para ficheiro de config\n");
	size_t length = strlen("./config/") + strlen(filename) + strlen(".json") + 1;
	char *filePath = (char *) malloc(length);
	if (!filePath) {
		printf("Failed to allocate memory for file path.\n");
		return -1; // Handle the error as needed
	}

	// Build the file path
	strcpy(filePath, "./config/");
	strcat(filePath, filename);
	strcat(filePath, ".json"); // Read file content to string

	printf("Carregar ficheiro para memoria\n");
	// Read file content to string
	if (read_file_to_string(filePath, &data) < 0) {
		printf("Loading default client configurations.\n");
		log_event(CLIENT_LOG_PATH, "Falha a carregar ficheiro de config! Carregando default.");
		load_default_client_config(config);
		log_event(CLIENT_LOG_PATH, "Config default cliente carregada.");
		return 1;
	}
	printf("Parsing ficheiro config para JSON\n");
	// Parse the JSON data
	if (parse_json(data, &json) < 0) {
		free(data);
		printf("Failed to parse JSON for client configuration.\n");
		log_event(CLIENT_LOG_PATH, "Falha a carregar ficheiro de config! Carregando default.");
		load_default_client_config(config);
		log_event(CLIENT_LOG_PATH, "Config default cliente carregada.");
		return 1;
	}
	free(data); // Free the allocated memory for data


	// Get the client configuration
	const cJSON *client = cJSON_GetObjectItem(json, "client");
	if (!client) {
		printf("Could not find 'client' in JSON\n");
		log_event(CLIENT_LOG_PATH, "Falha a carregar ficheiro de config! Carregando default.");
		load_default_client_config(config);
		log_event(CLIENT_LOG_PATH, "Config default cliente carregada.");
		cJSON_Delete(json);
		return 1;
	}
	printf("Carregar valores da config para struct em memoria\n");
	// Extracting values from the JSON object
	strcpy(config->id, cJSON_GetObjectItem(client, "id")->valuestring);
	strcpy(config->server_ip, cJSON_GetObjectItem(client, "server_ip")->valuestring);
	config->server_port = cJSON_GetObjectItem(client, "server_port")->valueint;
	strcpy(config->log_file, cJSON_GetObjectItem(client, "log_file")->valuestring);
	config->game_type = cJSON_GetObjectItem(client, "game_type")->valueint;
	// Cleanup
	cJSON_Delete(json);
	return 0; // Success
}
int load_server_config(const char *filename, server_config *config) {
	//printf("A Carregar Config File Do Server\n");
	char *data = NULL;
	cJSON *json = NULL;

	size_t length = strlen("./config/") + strlen(filename) + strlen(".json") + 1;
	char *filePath = (char *) malloc(length);
	if (!filePath) {
		printf("ERRO ao allocar memoria.\n");
		return -1; // Handle the error as needed
	}

	// Build the file path
	strcpy(filePath, "./config/");
	strcat(filePath, filename);
	strcat(filePath, ".json"); // Read file content to string


	if (read_file_to_string(filePath, &data) < 0) {
		printf("A Carregar Defauld Config.\n");
		log_event(SERVER_LOG_PATH, "Falha a carregar ficheiro de config! Carregando default.");
		load_default_server_config(config);
		log_event(SERVER_LOG_PATH, "Config default server carregada.");
		return 1;
	}

	// Parse the JSON data
	if (parse_json(data, &json) < 0) {
		free(data);
		printf("ERRO a dar parse ao JSON.\n");
		log_event(SERVER_LOG_PATH, "Falha a carregar ficheiro de config! Carregando default.");
		load_default_server_config(config);
		log_event(SERVER_LOG_PATH, "Config default server carregada.");
		return 1;
	}
	free(data); // Free the allocated memory for data

	//TODO -- FIX CONFIG WITH NEW ATTRIBUTES!!
	// Extracting values from the JSON object
	strcpy(config->log_file, cJSON_GetObjectItem(json, "log_file")->valuestring);
	strcpy(config->board_file_path, cJSON_GetObjectItem(json, "board_file_path")->valuestring);
	config->task_queue_size = cJSON_GetObjectItem(json, "task_queue_size")->valueint;
	config->task_handler_threads = cJSON_GetObjectItem(json, "event_handler_threads")->valueint;
	// Cleanup
	cJSON_Delete(json);
	free(filePath);
	log_event(SERVER_LOG_PATH, "Config server carregada.");
	return 0; // Success
}
/************************************
 * GLOBAL FUNCTIONS
 ************************************/
