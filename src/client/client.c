/******************************SERVER_DIR***************************************************
 * client.c
 * Skipper
 * 11/10/2024
 * Client Entry-point
 *********************************************************************************/


/************************************
 * INCLUDES
 ************************************/
#include "client.h"

#include <stdbool.h>

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
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
ClientConfig config;
int board[BOARD_SIZE][BOARD_SIZE];

int sock = 0;
struct sockaddr_in serv_addr;
char buffer[BUFFER_SIZE] = {0};

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
void client_init(int argc, char *argv[], ClientConfig *config);
void connect_to_server();
void server_handshake();

/************************************
 * STATIC FUNCTIONS
 ************************************/
int main(int argc, char *argv[]) {
	client_init(argc, argv, &config);

	const char *message = "Hello from client";

	connect_to_server();

	int valread = read(sock, buffer, BUFFER_SIZE);
	printf("Server: %s\n", buffer);

	while (1) {
		log_event(config.log_file, "A Enviar Mensagem");
		send(sock, message, strlen(message), 0);

		sleep(1);
	}

	close(sock);

	//solve_by_brute_force(board,board);

	exit(0);
}

void printBoard(int matrix[][BOARD_SIZE]) {
	printf("\nQuadro de Sudoku:\n");
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			printf("%d ", matrix[i][j]); // Print the number
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
}
void client_init(int argc, char *argv[], ClientConfig *config) {
	const char *config_file = (argc > 1) ? argv[1] : "client_1";
	if (load_client_config(config_file, config) < 0) {
		fprintf(stderr, "Failed to load client configuration.\n");
		exit(-1);
	}
	if (argc <= 1) {
		printf("NO SPECIFIC CONFIG FILE PROVIDED, USING DEFAULT FILE\n");
	}

	log_event(config->log_file, "Client Started");


	if(false) {
		printf("Client ID: %s\n", config->id);
		printf("Server IP: %s\n", config->server_ip);
		printf("Server Port: %d\n", config->server_port);
		printf("Log File: %s\n", config->log_file);
	}

	log_event(config->log_file, "Client Config Loaded");
}

void connect_to_server() {
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_event(config.log_file,"Erro ao criar socket!! EXIT\n");
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Socket criado com sucesso");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(config.server_port);

	if (inet_pton(AF_INET, config.server_ip, &serv_addr.sin_addr) <= 0) {
		log_event(config.log_file,"IP invalido! EXIT\n");
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "IP valido");

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		log_event(config.log_file,"Pedido de conecção falhou! EXIT\n");
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Conectado ao servidor");

	server_handshake();


}

void server_handshake() {

}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
