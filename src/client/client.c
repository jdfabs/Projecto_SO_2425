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

#include <sys/socket.h>
#include <sys/un.h>
//#include <unistr.h>

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
client_config config;
int board[BOARD_SIZE][BOARD_SIZE];

int sock = 0;
struct sockaddr_un server_address;
char buffer[BUFFER_SIZE] = {0};


/************************************
 * STATIC FUNCTION PROTOTYPES
 ***********************************/
void client_init(int argc, char *argv[], client_config *config);
void connect_to_server();

/************************************
 * STATIC FUNCTIONS
 ************************************/
int main(int argc, char *argv[]) {
	client_init(argc, argv, &config); // Client data structures setup
	connect_to_server(); //Connect to server

	//HANDSHAKE WITH SERVER
	char buffer[BUFFER_SIZE];
	recv(sock, buffer, BUFFER_SIZE, 0);
	int counter = atoi(buffer)*100;

	printf("%d\n",config.game_type );
	sprintf(buffer, "%d", config.game_type);
	send(sock,buffer , sizeof(buffer), 0);


	//TODO -- fix handshake
	//hand shake them de informar o servidor se o jogo é individual ou competição -- 2 comportamentos distintos
	//if (config.game_mode == individual) -> função x
	//else -> função y
	//HANDSHAKE END

	printBoard(board);
//exit(0);
	while (1) {
		sprintf(buffer, "%d", counter);
		send(sock, buffer, strlen(buffer), 0);
		printf("Request sent: %d ---- WAITING\n", counter);
		counter++;


		ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0'; // Null-terminate the received string
			printf("Received from server: %s\n", buffer);
		}
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

void client_init(int argc, char *argv[], client_config *config) {
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
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		log_event(config.log_file,"Erro ao criar socket!! EXIT\n");
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Socket criado com sucesso");

	server_address.sun_family = AF_UNIX;
	strncpy(server_address.sun_path, "/tmp/local_socket", sizeof(server_address.sun_path)-1);

	if (connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
		log_event(config.log_file,"IP invalido! EXIT\n");
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Conectado ao servidor");
}

//TODO -- enviar e receber quadros
void send_solution() {
}
void receive_answer() {
}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/
