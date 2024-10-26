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
#include "file_handler.h"

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


int parse_json(const char *data, cJSON **json) {
	*json = cJSON_Parse(data);
	if (!*json) {
		return -1; // Indicate parsing error
	}
	printf("JSON parsed successfully\n");
	return 0; // Indicate success
}

//CHECKED
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
	printf("File read to string successfully\n");
	return 0;
}


void load_default_client_config(ClientConfig *client_config) {
	strcpy(client_config->id, "default_client");
	strcpy(client_config->server_ip, "127.0.0.1");
	client_config->server_port = 8080;
	strcpy(client_config->log_file, "./logs/client_default.log");
}

void load_default_server_config(server_config *server_config) {
	strcpy(server_config->ip, "127.0.1.1");
	server_config->port = 8080;
	server_config->logging = 1;
	strcpy(server_config->log_file, "./logs/server_default.log");
	server_config->log_level = 1;
	server_config->max_clients = 5;
	server_config->backup_interval = 15;
	server_config->board_file_path, "./boards/boards.json";
}


int load_client_config(const char *filename, ClientConfig *config) {
	char *data = NULL;
	cJSON *json = NULL;


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


	// Read file content to string
	if (read_file_to_string(filePath, &data) < 0) {
		printf("Loading default client configurations.\n");
		log_event(CLIENT_LOG_PATH, "Falha a carregar ficheiro de config! Carregando default.");
		load_default_client_config(config);
		log_event(CLIENT_LOG_PATH, "Config default cliente carregada.");
		return 1;
	}

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

	// Extracting values from the JSON object
	strcpy(config->id, cJSON_GetObjectItem(client, "id")->valuestring);
	strcpy(config->server_ip, cJSON_GetObjectItem(client, "server_ip")->valuestring);
	config->server_port = cJSON_GetObjectItem(client, "server_port")->valueint;
	strcpy(config->log_file, cJSON_GetObjectItem(client, "log_file")->valuestring);

	// Cleanup
	cJSON_Delete(json);
	return 0; // Success
}

int load_server_config(const char *filename, server_config *config) {
	printf("A Carregar Config File Do Server\n");
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


	// Extracting values from the JSON object
	strcpy(config->ip, cJSON_GetObjectItem(json, "ip")->valuestring);
	config->port = cJSON_GetObjectItem(json, "port")->valueint;
	config->logging = cJSON_GetObjectItem(json, "enable_logging")->valueint;
	strcpy(config->log_file, cJSON_GetObjectItem(json, "log_file")->valuestring);
	config->log_level = cJSON_GetObjectItem(json, "log_level")->valueint;
	config->max_clients = cJSON_GetObjectItem(json, "max_clients")->valueint;
	config->backup_interval = cJSON_GetObjectItem(json, "backup_interval")->valueint;
	strcpy(config->board_file_path, cJSON_GetObjectItem(json, "board_file_path")->valuestring);
	// Cleanup
	cJSON_Delete(json);
	free(filePath);
	log_event(SERVER_LOG_PATH, "Config server carregada.");
	return 0; // Success
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
