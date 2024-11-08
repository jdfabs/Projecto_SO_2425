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

#include <cJSON.h>
#include <stdbool.h>

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistr.h>
#include <fcntl.h>

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
int **board;

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
	connect_to_server(); // Connect to server

	int client_socket;
	char room_name[100];
	recv(sock, buffer, BUFFER_SIZE, 0);
	sscanf(buffer, "%d-%99s", &client_socket, room_name);

	printf("Client socket is %d\n", client_socket);
	printf("Room name: %s\n", room_name);

	char temp[255];
	sprintf(temp, "/sem_%s_producer", room_name);
	sem_t *sem_prod = sem_open(temp, 0);
	sprintf(temp, "/sem_%s_consumer", room_name);
	sem_t *sem_cons = sem_open(temp, 0);
	sprintf(temp, "/sem_%s_solucao", room_name);
	sem_t *sem_solucao = sem_open(temp, 0);


	srand(time(NULL));

	int room_shared_memory_fd = shm_open(room_name, O_RDWR, 0666);
	if (room_shared_memory_fd == -1) {
		perror("shm_open FAIL");
		exit(EXIT_FAILURE);
	}

	multiplayer_room_shared_data_t *shared_data = mmap(NULL, sizeof(multiplayer_room_shared_data_t),
														PROT_READ | PROT_WRITE, MAP_SHARED, room_shared_memory_fd, 0);
	if (shared_data == MAP_FAILED) {
		perror("mmap FAIL");
		exit(EXIT_FAILURE);
	}

	sem_post(&shared_data->sem_room_full); // assinala que tem mais um cliente no room

	for (;;) {
		sem_wait(&shared_data->sem_game_start); // espera que o jogo comece

		int **board = getMatrixFromJSON(cJSON_Parse(shared_data->starting_board));

		// SOLUCIONAR BOARD
		for (int i = 0; i < BOARD_SIZE; i++) {
			for (int j = 0; j < BOARD_SIZE; j++) {
				if (board[i][j] == 0) {
					printf("celula (%d,%d) está vazia\n", i, j);
					for (int k = 1; k <= BOARD_SIZE; k++) {
						char message[255];
						sprintf(message, "0-%d,%d,%d", i, j, k);


						//PREPROTOCOLO
						sem_wait(sem_prod);
						pthread_mutex_lock(&shared_data->mutex_task_reader);

						//ZONA CRITICA PARA CRIAR TASK
						shared_data->task_queue[shared_data->task_productor_ptr].client_socket = client_socket;
						sprintf(shared_data->task_queue[shared_data->task_productor_ptr].request, message);
						shared_data->task_productor_ptr = (shared_data->task_productor_ptr + 1) % 5;

						printf("Pedido colocado na fila\n");
						//POS PROTOCOLO
						pthread_mutex_unlock(&shared_data->mutex_task_reader);
						sem_post(sem_cons);
						recv(sock, buffer, BUFFER_SIZE, 0);

						if (buffer[0] == '1') {
							board[i][j] = k;
							printf("Numero correto encontrado\n");
							break;
						}
						usleep((rand() % 150000));
					}
				}
			}
		}
	//Solução encontrada
	sem_post(sem_solucao);

	}

	/*
		int random_time = rand() % 10;
		printf("random time = %d\n", random_time);
		sleep(random_time);// random wait
		printf("found sol\n");
		sem_post(&shared_data->sem_found_solution);
	*/

	munmap(shared_data, sizeof(multiplayer_room_shared_data_t));


	/*
	int counter = atoi(buffer) * 100; // Initialize the counter for future use
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
	*/
	close(sock);
	exit(0);
}

void printBoard(int **matrix) {
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


	if (false) {
		printf("Client ID: %s\n", config->id);
		printf("Server IP: %s\n", config->server_ip);
		printf("Server Port: %d\n", config->server_port);
		printf("Log File: %s\n", config->log_file);
	}

	log_event(config->log_file, "Client Config Loaded");
}

void connect_to_server() {
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		log_event(config.log_file, "Erro ao criar socket!! EXIT\n");
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Socket criado com sucesso");

	server_address.sun_family = AF_UNIX;
	strncpy(server_address.sun_path, "/tmp/local_socket", sizeof(server_address.sun_path) - 1);

	if (connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
		log_event(config.log_file, "IP invalido! EXIT\n");
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
