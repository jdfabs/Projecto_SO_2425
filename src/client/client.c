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

void send_solution_attempt_multiplayer_ranked(int x, int y, int novo_valor) {
	char message[255];
	sprintf(message, "0-%d,%d,%d", x, y, novo_valor);
	printf("%s\n", message);
	//PREPROTOCOLO
	sem_wait(sem_sync_2); //produtores
	sem_wait(mutex_task);

	//ZONA CRITICA PARA CRIAR TASK
	multiplayer_ranked_shared_data->task_queue[multiplayer_ranked_shared_data->task_productor_ptr].client_socket = client_socket;
	sprintf(multiplayer_ranked_shared_data->task_queue[multiplayer_ranked_shared_data->task_productor_ptr].request, message);
	multiplayer_ranked_shared_data->task_productor_ptr = (multiplayer_ranked_shared_data->task_productor_ptr + 1) % 5;
	printf("Pedido colocado na fila\n");

	usleep(rand() % (config.slow_factor+0));
	//POS PROTOCOLO
	sem_post(mutex_task);
	sem_post(sem_sync_1);
}
void send_solution_attempt_multiplayer_casual(int x, int y, int novo_valor) {
	char message[255];
	if (x == -1) {
		sprintf(message, "1--1,-1,-1", y, novo_valor);
	}
	else sprintf(message, "0-%d,%d,%d", x, y, novo_valor);
	//PREPROTOCOLO
	sem_wait(&multiplayer_casual_room_shared_data->sems_client[client_index]);

	//ZC
	sprintf(multiplayer_casual_room_shared_data->task_queue[client_index].request, message);
	printf("Pedido colocado no array\n");

	usleep(rand() % (config.slow_factor+0));

	//POSPROTOCOLO
	sem_post(&multiplayer_casual_room_shared_data->sems_server[client_index]);
}
void send_solution_attempt_multiplayer_coop() {
	// PREPROTOCOLO
	sem_wait(&multiplayer_coop_room_shared_data->sems_client[client_index]);

	// ZC
	usleep(rand() % (config.slow_factor+0));
	cJSON *json_board = cJSON_Parse(multiplayer_coop_room_shared_data->current_board);
	if (json_board == NULL) {
		printf("Error parsing JSON\n");
		return;
	}
	clear();

	int x, y, value;
	int **old_board = getMatrixFromJSON(json_board);

	printBoard(old_board);

	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			if (old_board[i][j] == 0) {
				printf("celula (%d,%d) está vazia\n", i, j);
				x = i;
				y = j;
				value = rand() % 9 + 1;
				sprintf(multiplayer_coop_room_shared_data->task_queue[client_index].request, "0-%d,%d,%d", x, y, value);
				printf("%d,%d,%d\n", x, y, value);
				goto outside_for;
			}
		}
	}
	outside_for:

		for (int i = 0; i < BOARD_SIZE; i++) {
			free(old_board[i]);
		}
	free(old_board);

	// Free the JSON object
	cJSON_Delete(json_board);

	// POSPROTOCOLO
	sem_post(&multiplayer_coop_room_shared_data->sems_server[client_index]);
	sem_post(&multiplayer_coop_room_shared_data->sem_has_requests);

	usleep(rand() % (config.slow_factor+0));
}
bool get_response_multiplayer_casual() {
	char response[1024];
	//PRE
	sem_wait(&multiplayer_casual_room_shared_data->sems_client[client_index]);
	//ZC
	sprintf(response, multiplayer_casual_room_shared_data->task_queue[client_index].request);

	//POS
	sem_post(&multiplayer_casual_room_shared_data->sems_client[client_index]);
	return response[0] == '0' ? false : true;
}



void send_solution_attempt_single_player(int x, int y, int novo_valor) {
	char message[255];
	sprintf(message, "0-%d,%d,%d", x, y, novo_valor);
	//PREPROTOCOLO
	sem_wait(sem_sync_2);// redundante??
	//ZC
	usleep(rand() % (config.slow_factor+1));
	printf("%s\n", message);
	strcpy(singleplayer_room_shared_data->buffer, message);

	//POSPROTOCOLO
	sem_post(sem_sync_1);// passa para o server
}
bool receice_answer_single_player() {
	sem_wait(sem_sync_2); //espera pela resposta do server
	usleep(rand() % (config.slow_factor+1));
	bool answer = atoi(singleplayer_room_shared_data->buffer);
	sem_post(sem_sync_2); //vai tratar das continhas (vai escrever outra vez) REDUNDANTE??
	return answer;

};

int main(int argc, char *argv[]) {

	client_init(argc, argv, &config); // Client data structures setup
	connect_to_server(); // Connect to server

	//Handshake com server --- é enviado o socket do cliente e o nome do room em que este fica
	char aux[100];
	sprintf(aux, "%d", config.game_type);
	send(sock, aux, sizeof(aux),0);

	char room_name[100];
	recv(sock, buffer, BUFFER_SIZE, 0);
	sleep(1);
	sscanf(buffer, "%d-%d-%99s", &client_socket, &client_index, room_name);

	printf("Socked do Cliente: %d\n", client_socket);
	printf("Nome da sala connectada: %s\n", room_name);
	printf("Client Index: %d\n", client_index);
	separator();
	printf("Inicializacao dos meios de sincronizacao da sala\n");
	sleep(1);



		
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
	if (inet_pton(AF_INET, config.server_ip, &server_address.sin_addr) <= 0) { // Endereço IP do servidor
		log_event(config.log_file, "Endereço inválido! EXIT\n");
		exit(EXIT_FAILURE);
	}

	printf("Pedindo ao Servidor a conexão\n");
	if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
		log_event(config.log_file, "Conexão falhou! EXIT\n");
		exit(EXIT_FAILURE);
	}

	printf("Conectado ao servidor!\n");
	log_event(config.log_file, "Conectado ao servidor");
	separator();
}

