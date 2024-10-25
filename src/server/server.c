/*********************************************************************************
 * server.c
 * Skipper
 * 11/10/2024
 * Server entry-point
 *********************************************************************************/

/************************************
 * INCLUDES
 ************************************/
#include "server.h"
#include "file_handler.h"
#include <arpa/inet.h> //para aceitar sockets pela net (ou local 127.0.0.1)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
ServerConfig config;
cJSON *boards;
/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

void server_init(int argc, char **argv);
/************************************
 * STATIC FUNCTIONS
 ************************************/



int main(int argc, char *argv[]) {
	server_init(argc, argv);

	int server_fd, new_socket;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char buffer[BUFFER_SIZE];

	const char *hello = "Hello from server";

	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("Socket falhou");
		log_event(config.log_file, "Criação do socket falhou!");
		exit(EXIT_FAILURE);
	}

	char temp[255];
	sprintf(temp, "Socket criado com sucesso%d", server_fd);
	log_event(config.log_file, temp);


	//reutilizar o binding address caso já esteja em uso
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(config.port);

	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("Socket falhou bind");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	//Ouvir pedidos de connecção
	if(listen(server_fd, 5) < 0) { //5 = queue de conecções a se conectar
		perror("Listen falhou");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	while (true) {
		//aceitar connecções
		if((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen)) < 0) {
			perror("Connecção falhada");
			close(server_fd);
			exit(EXIT_FAILURE);
		}

		int valread = read(new_socket, buffer, BUFFER_SIZE);
		printf("%s\n", buffer);

		send(new_socket, hello, strlen(hello), 0);
		printf("%s\n", hello);


		printf(cJSON_Print(get_board_state_by_id(boards,INITIAL_STATE,1)));

	}


	close(new_socket);
	close(server_fd);

	exit(EXIT_FAILURE);
}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/
/*matrix = getMatrixFromJSON(get_board_state_by_id(0,STARTING_STATE));

cJSON *startingStateMatrix = matrix_to_JSON(matrix);
printf("\nReset current state\n");
save_board_to_file(update_boards_with_new_board(startingStateMatrix,0,CURRENT_STATE),1);

printf("\nFICHEIRO JSON CURRENT STATE\n");
printf(cJSON_Print(get_board_state_by_id(0,CURRENT_STATE)));

int **solution;
solution = getMatrixFromJSON(get_board_state_by_id(0,END_STATE));

printBoard(matrix);
printf("\nStarting to solve from initial state\n");

cJSON *finishedMatrix = matrix_to_JSON(matrix);
printf("\nsaving matrix to file\n");
save_board_to_file(update_boards_with_new_board(finishedMatrix, 0, CURRENT_STATE),1);
*/


void server_init(int argc, char **argv) {
	if (argc == 1) {
		printf("Usage: ./server [CONFIG_FILE_NAME]\n");
		log_event("./logs/server_default.json","Servidor iniciado sem argumentos");
		exit(EXIT_FAILURE);
	}

	printf("Starting server...\n");

	if (load_server_config(argv[1], &config) < 0) {
		fprintf(stderr, "Failed to load server configuration.\n");
		log_event("./logs/server_default.json","Carregamento de configs falhado -- A FECHAR SERVER");
		exit(EXIT_FAILURE);
	}
	printf("............................................\n");
	printf("Config Servidor Carregado Correctamente:\n");
	printf("Server IP: %s\n", config.ip);
	printf("Port: %d\n", config.port);
	printf("Logging: %d\n", config.logging);
	printf("Log File: %s\n", config.log_file);
	printf("Log Level: %d\n", config.log_level);
	printf("Max Clients: %d\n", config.max_clients);
	printf("Backup Interval: %d\n", config.backup_interval);
	printf("............................................\n");

	log_event(config.log_file, "Servidor começou");

	boards = load_boards(config.board_file_path);
	if (boards == NULL) {
		log_event(config.log_file, "Erro ao carregar boards para a memoria!");
		exit(-1);
	}
	log_event(config.log_file,"Boards carregados para memoria com sucesso");
}
