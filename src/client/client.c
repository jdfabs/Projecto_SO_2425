/******************************SERVER_DIR***************************************************
 * client.c
 * Skipper
 * 11/10/2024
 * Client Entry-point
 *********************************************************************************/

#define clear()	printf("\033[H\033[J")
#define separator() printf("--------------------------\n")
/************************************
 * INCLUDES
 ************************************/
#include "client.h"

#include <cJSON.h>
#include <stdbool.h>

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

client_config config;
int **board;

int sock = 0;
struct sockaddr_un server_address;
char buffer[BUFFER_SIZE] = {0};

int client_socket;
int client_index;


/************************************
 * STATIC FUNCTION PROTOTYPES
 ***********************************/
void client_init(int argc, char *argv[], client_config *config);

void connect_to_server();

void graceful_shutdown() {
	send(client_socket, "9", BUFFER_SIZE, 0);
	close(client_socket);

	exit(0);
}

int main(int argc, char *argv[]) {
	client_init(argc, argv, &config); // Client data structures setup

	char aux[100];
	if (config.manual_mode) {
		printf("Welcome to the sudoku server! Please select the game mode you want to play!\n");
		printf("0) Single Player\n");
		printf("1) Ranked Multiplayer\n");
		printf("2) Casual Multiplayer\n");
		printf("3) Coop Multiplayer\n");
		printf("4) EXIT\n");

		int game_mode = -1;
		while (1) {
			printf("Enter your choice: ");
			if (scanf("%d", &game_mode) == 1 && game_mode >= 0 && game_mode <= 4) {
				if (game_mode == 4) {
					printf("Exiting the client. Goodbye!\n");
					graceful_shutdown();
				}
				sprintf(aux, "%d", game_mode);
				break;
			} else {
				printf("Invalid input. Please enter a number between 0 and 4.\n");
				while (getchar() != '\n');
			}
		}
		sprintf(aux, "%d", game_mode);
		config.game_type = game_mode;
	}
	else {
		sprintf(aux, "%d", config.game_type);
	}
	connect_to_server(); // Connect to server

	//Handshake com server --- é enviado o socket do cliente e o nome do room em que este fica
	send(sock, aux, strlen(aux), 0);

	char room_name[100];
	recv(sock, buffer, BUFFER_SIZE, 0);
	printf("%s\n", buffer);
	clear();

	sscanf(buffer, "%d-%d-%99s", &client_socket, &client_index, room_name);

	printf("Socked do Cliente: %d\n", client_socket);
	printf("Nome da sala connectada: %s\n", room_name);
	printf("Client Index: %d\n", client_index);
	separator();

	if (config.manual_mode) {
		printf("PRESS ENTER TO CONTINUE\n");
		int c;
		while ((c = getchar()) != '\n' && c != EOF);
		while (getchar() != '\n');
	}

	int last_i;
	int last_j;
	int last_k;

	while (true) {
		exit_for:
		recv(sock, buffer, BUFFER_SIZE, 0);
		char *message = buffer;
		message += 2;

		switch (buffer[0]) {
			case '0':
				//Update Board
				printf("Updating Board\n");
				board = getMatrixFromJSON(cJSON_Parse(message));
				printBoard(board);
				break;
			case '1':
			//Correct Guess
				log_event(config.log_file, "Correct Guess");
				board[last_i][last_j] = last_k;//update local board
				printf("Correct Guess\n");
				//move to case 2 (print board)
			case '2':
			//Wrong Guess
				printBoard(board);
				//move to case 3 (new attempt)
			case '3':
				//Game Start/Take a guess again
				if (config.game_type != 3) {
					for (int i = 0; i < 9; i++) {
						for (int j = 0; j < 9; j++) {
							if (board[i][j] == 0) {	// FIND FIRST EMPTY SPOT
								printf("celula (%d,%d) está vazia\n", i, j);
								int k = 0;
								if (config.manual_mode) {
									while (1) {
										printf("Enter your choice: ");
										if (scanf("%d", &k) == 1 && k >= 1 && k <=9) {
											break;
										}
										else {
											printf("Invalid input. Please enter a number between 1 and 9.\n");
											while (getchar() != '\n');
										}
									}
								}
								else {
									k = rand() % 9 + 1; // random try
								}

								last_i = i;
								last_j = j;
								last_k = k;
								clear();
								printf("ROOM: %s\n", room_name);

								//usleep(rand() % (config.slow_factor + 1));


								sprintf(buffer, "0-%d-%d-%d", i, j, k);
								send(sock, buffer, strlen(buffer), 0);
								printf("Pedido de verificação enviado: %d em (%d,%d)\n",k,i,j);


								goto exit_for;
							}
						}
					}
					send(sock, "1", strlen("1"), 0);
					printf("SOLVED\n");
				}
				else {
					if (config.manual_mode) {
						printf("PRESS ENTER TO SEND RANDOM SOLUTION REQUEST\n");
						int c;
						while ((c = getchar()) != '\n' && c != EOF);
						while (getchar() != '\n');
					}
					else {
						usleep(rand() % config.slow_factor*2+1);
					}
					clear();
					send(sock, "0", strlen("0"), 0);
					printf("PEDIDO ENVIADO\n");
				}

				break;
		}
	}
}


void printBoard(int **matrix) {
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
}

void client_init(const int argc, char *argv[], client_config *config) {
	signal(SIGINT, graceful_shutdown);
	signal(SIGTERM, graceful_shutdown);


	srand(time(NULL));
	clear();
	separator();
	printf("Inicialização do cliente\n");
	const char *config_file = argc > 1 ? argv[1] : "client_1";
	printf("Carregar argumento de entrada\n");

	if (argc <= 1) {
		printf("FICHEIRO DE CONFIGURACAO NAO ESPECIFICADO, UTILIZANDO DEFAULT (client_1)\n");
	}
	if (load_client_config(config_file, config) < 0) {
		fprintf(stderr, "Failed to load client configuration.\n");
		exit(-1);
	}

	printf("Configuracoes carregadas com sucesso\n");
	log_event(config->log_file, "Client Started");
	log_event(config->log_file, "Client Config Loaded");
	separator();
}

void connect_to_server() {
	printf("Criando Socket\n");
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_event(config.log_file, "Erro ao criar socket!! EXIT\n");
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Socket criado com sucesso");

	// Configurar endereço IP e porta do servidor
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(config.server_port); // Porta do servidor
	if (inet_pton(AF_INET, config.server_ip, &server_address.sin_addr) <= 0) {
		// Endereço IP do servidor
		log_event(config.log_file, "Endereço inválido! EXIT\n");
		exit(EXIT_FAILURE);
	}

	printf("Pedindo ao Servidor a conexão\n");
	if (connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
		log_event(config.log_file, "Conexão falhou! EXIT\n");
		exit(EXIT_FAILURE);
	}

	printf("Conectado ao servidor!\n");
	log_event(config.log_file, "Conectado ao servidor");
	separator();
}
