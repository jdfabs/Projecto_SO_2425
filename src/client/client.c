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
	close(client_socket);

	exit(0);
}

int main(int argc, char *argv[]) {
	client_init(argc, argv, &config); // Client data structures setup
	connect_to_server(); // Connect to server

	//Handshake com server --- é enviado o socket do cliente e o nome do room em que este fica
	char aux[100];
	sprintf(aux, "%d", config.game_type);
	send(sock, aux, strlen(aux), 0);

	char room_name[100];
	recv(sock, buffer, BUFFER_SIZE, 0);
	//sleep(1);
	printf("%s\n", buffer);

	sscanf(buffer, "%d-%d-%99s", &client_socket, &client_index, room_name);

	printf("Socked do Cliente: %d\n", client_socket);
	printf("Nome da sala connectada: %s\n", room_name);
	printf("Client Index: %d\n", client_index);
	separator();
	printf("Inicializacao dos meios de sincronizacao da sala\n");
	//sleep(1);

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
				board[last_i][last_j] = last_k;
			case '2':
			//Wrong Guess

			case '3':
				//Game Start/Take a guess again
				//JOGOS INDIVIDUAIS
				if (config.game_type != 3) {
					for (int i = 0; i < 9; i++) {
						for (int j = 0; j < 9; j++) {
							if (board[i][j] == 0) {	// FIND FIRST EMPTY SPOT
								printf("celula (%d,%d) está vazia\n", i, j);

								const int k = rand() % 9 + 1; // random try
								last_i = i;
								last_j = j;
								last_k = k;
								clear();
								printf("ROOM: %s\n", room_name);
								printBoard(board);
								//usleep(rand() % (config.slow_factor + 1));


								sprintf(buffer, "0-%d-%d-%d", i, j, k);
								send(sock, buffer, strlen(buffer), 0);
								printf("Pedido de verificação enviado: %d em (%d,%d)\n",k,i,j);


								goto exit_for;
								switch (config.game_type) {
									case 0:


									case 1:
									//TODO !IMPORTANT FIX --- BROKEN
									//recv(sock, buffer, BUFFER_SIZE, 0);
										if (buffer[0] == '1') {
											board[i][j] = k;
											//printf("Numero correto encontrado\n");
										}
										break;

								}

							}
						}

					}
					send(sock, "1", strlen("1"), 0);
					printf("SOLVED\n");


					//Solução encontrada
					/*
					if (room->type == 2) {
						send_solution_attempt_multiplayer_casual(-1, -1, -1, multiplayer_casual_room_shared_data,
																client_index); //signal for has solution
						multiplayer_casual_room_shared_data->has_solution[client_index] = true;
						is_first_attempt = true;
					}*/
					//TODO
					//clear();
					//printf("ROOM: %s\n", room->name);
					//TODO
					//printBoard(board);
					//sem_post(sem_solucao);
				}
				break;
			case '4':
				break;
			case '5':
				break;
			case '6':
				break;
			case '7':
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
