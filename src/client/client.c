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


client_config config;
int **board;

int sock = 0;
struct sockaddr_un server_address;
char buffer[BUFFER_SIZE] = {0};

int client_socket;
multiplayer_room_shared_data_t *multiplayer_ranked_shared_data;
singleplayer_room_shared_data_t *singleplayer_room_shared_data;
sem_t *sem_solucao;
sem_t *sem_room_full;
sem_t *sem_game_start;

sem_t *mutex_task;
sem_t *sem_sync_1;
sem_t *sem_sync_2;
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
	//TODO
}
void send_solution_attempt_single_player(int x, int y, int novo_valor) {
	//TODO
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
	sscanf(buffer, "%d-%99s", &client_socket, room_name);

	printf("Socked do Cliente: %d\n", client_socket);
	printf("Nome da sala connectada: %s\n", room_name);
	separator();
	printf("Inicializacao dos meios de sincronizacao da sala\n");
	sleep(1);

	if(config.game_type == 0) {
		char temp[255];

		sprintf(temp, "/sem_%s_solucao", room_name);
		sem_solucao = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_game_start", room_name);
		sem_game_start = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_server", room_name);
		sem_sync_1 = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_client", room_name);
		sem_sync_2 = sem_open(temp, 0);

		printf("Abrirmemoria partilhada da sala\n");
		const int room_shared_memory_fd = shm_open(room_name, O_CREAT | O_RDWR, 0666);
		if (room_shared_memory_fd == -1) {
			perror("shm_open fail");
			exit(EXIT_FAILURE);
		}

		singleplayer_room_shared_data = mmap (NULL, sizeof(singleplayer_room_shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, room_shared_memory_fd, 0);
		if(singleplayer_room_shared_data == MAP_FAILED) {
			perror("mmap fail");
			exit(EXIT_FAILURE);
		}
		printf("PRONTO PARA JOGAR!\nAssinalando a Sala que esta a espera\n");
	}
	if(config.game_type == 1) {
		char temp[255];
		sprintf(temp, "/sem_%s_producer", room_name);
		sem_sync_2 = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_consumer", room_name);
		sem_sync_1 = sem_open(temp, 0);

		sprintf(temp, "/sem_%s_solucao", room_name);
		sem_solucao = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_room_full", room_name);
		sem_room_full = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_game_start", room_name);
		sem_game_start = sem_open(temp, 0);
		sprintf(temp, "/mut_%s_task_client", room_name);
		mutex_task = sem_open(temp, 0);


		printf("Abrir memoria partilhada da sala\n");
		const int room_shared_memory_fd = shm_open(room_name, O_RDWR, 0666);
		if (room_shared_memory_fd == -1) {
			perror("shm_open FAIL");
			exit(EXIT_FAILURE);
		}

		multiplayer_ranked_shared_data = mmap(NULL, sizeof(multiplayer_room_shared_data_t),PROT_READ | PROT_WRITE, MAP_SHARED, room_shared_memory_fd, 0);
		if (multiplayer_ranked_shared_data == MAP_FAILED) {
			perror("mmap FAIL");
			exit(EXIT_FAILURE);
		}
		printf("PRONTO PARA JOGAR!\nAssinalando a Sala que esta a espera\n");
		sem_post(sem_room_full); // assinala que tem mais um cliente no room
	}
	else if(config.game_type == 2) {
		printf("Logica para Multiplayer casual NAO IMPLEMENTADO");
	}

	separator();
	printf("ESPERANDO\n");

	for (;;) {
		sem_wait(sem_game_start); // espera que o jogo comece
		clear();

		int **board;

		switch (config.game_type) {
			case 0:
				printf("111\n");
				board = getMatrixFromJSON(cJSON_Parse(singleplayer_room_shared_data->starting_board));
			break;
			case 1:
				printf("222\n");
				board = getMatrixFromJSON(cJSON_Parse(multiplayer_ranked_shared_data->starting_board));
			break;
			case 2:
				printf("333\n");
				printf("TODO!!!");
		}


		// SOLUCIONAR BOARD
		for (int i = 0; i < BOARD_SIZE; i++) {
			for (int j = 0; j < BOARD_SIZE; j++) {
				if (board[i][j] == 0) {
					printf("celula (%d,%d) está vazia\n", i, j);
					while(true) {
						const int k = rand() %9+1;
						clear();
						printf("ROOM: %s\n", room_name);
						printBoard(board);
						usleep(rand() % (config.slow_factor*10+1));
						printf("Pedido de verificação enviado: %d em (%d,%d)\n",k,i,j);
						switch (config.game_type) {
							case 0:
								send_solution_attempt_single_player(i,j,k);
								if(receice_answer_single_player()) {
									board[i][j] = k;
									printf("Numero correto encontrado\n");
									goto after_while;
								}
								break;
							case 1:
								send_solution_attempt_multiplayer_ranked(i, j, k);
								recv(sock, buffer, BUFFER_SIZE, 0);
								if (buffer[0] == '1') {
									board[i][j] = k;
									printf("Numero correto encontrado\n");
									goto after_while;
								}
								break;
							case 2:
								printf("TODO\n");
								while(true) {}
							break;
						}


					}
after_while: continue;
				}
			}
		}
	//Solução encontrada
		clear();
		printf("ROOM: %s\n", room_name);
		printBoard(board);
		sem_post(sem_solucao);

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

