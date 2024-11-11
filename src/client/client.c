/******************************SERVER_DIR***************************************************
 * client.c
 * Skipper
 * 11/10/2024
 * Client Entry-point
 *********************************************************************************/

#define clear()	printf("\033[H\033[J")
#define separator() printf("---------------------------------------------------------------------------------------------\n")
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

int client_socket;
multiplayer_room_shared_data_t *shared_data;
sem_t *sem_solucao;
sem_t *sem_cons;
sem_t *sem_prod;
/************************************
 * STATIC FUNCTION PROTOTYPES
 ***********************************/
void client_init(int argc, char *argv[], client_config *config);
void connect_to_server();

void send_solution_attempt(int x, int y, int novo_valor) {
	char message[255];
	sprintf(message, "0-%d,%d,%d", x, y, novo_valor);

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
}

/************************************
 * STATIC FUNCTIONS
 ************************************/
int main(int argc, char *argv[]) {
	client_init(argc, argv, &config); // Client data structures setup
	connect_to_server(); // Connect to server

	//Handshake com server --- é enviado o socket do cliente e o nome do room em que este fica
	char room_name[100];
	recv(sock, buffer, BUFFER_SIZE, 0);
	sscanf(buffer, "%d-%99s", &client_socket, room_name);

	printf("Socked do Cliente: %d\n", client_socket);
	printf("Nome da sala connectada: %s\n", room_name);
	separator();
	sleep(3);
	printf("Inicializacao dos meios de sincronizacao da sala\n");
	char temp[255];
	sprintf(temp, "/sem_%s_producer", room_name);
	sem_prod = sem_open(temp, 0);
	sprintf(temp, "/sem_%s_consumer", room_name);
	sem_cons = sem_open(temp, 0);
	sprintf(temp, "/sem_%s_solucao", room_name);
	sem_solucao = sem_open(temp, 0);


	srand(time(NULL));
	printf("Abrir memoria partilhada da sala\n");
	int room_shared_memory_fd = shm_open(room_name, O_RDWR, 0666);
	if (room_shared_memory_fd == -1) {
		perror("shm_open FAIL");
		exit(EXIT_FAILURE);
	}


	shared_data = mmap(NULL, sizeof(multiplayer_room_shared_data_t),
						PROT_READ | PROT_WRITE, MAP_SHARED, room_shared_memory_fd, 0);
	if (shared_data == MAP_FAILED) {
		perror("mmap FAIL");
		exit(EXIT_FAILURE);
	}
	printf("PRONTO PARA JOGAR!\nAssinalando a Sala que esta a espera\n");
	sem_post(&shared_data->sem_room_full); // assinala que tem mais um cliente no room
	separator();
	printf("ESPERANDO\n");

	for (;;) {
		sem_wait(&shared_data->sem_game_start); // espera que o jogo comece
		clear();

		int **board = getMatrixFromJSON(cJSON_Parse(shared_data->starting_board));

		// SOLUCIONAR BOARD
		for (int i = 0; i < BOARD_SIZE; i++) {
			for (int j = 0; j < BOARD_SIZE; j++) {
				if (board[i][j] == 0) {
					printf("celula (%d,%d) está vazia\n", i, j);
					while(true) {
						int k = (rand() %9)+1;
						send_solution_attempt(i, j, k);
						clear();
						printBoard(board);
						printf("Pedido de verificação enviado: %d em (%d,%d)\n",k,i,j);
						recv(sock, buffer, BUFFER_SIZE, 0);

						if (buffer[0] == '1') {
							board[i][j] = k;
							printf("Numero correto encontrado\n");
							break;
						}
						usleep((rand() % 1500000));

					}
				}
			}
		}
	//Solução encontrada
	sem_post(sem_solucao);

	}

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
	clear();
	separator();
	printf("Inicialização do cliente\n");
	const char *config_file = (argc > 1) ? argv[1] : "client_1";
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
	sleep(3);
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
	sleep(3);
}


//TODO -- enviar e receber quadros
void send_solution() {
}

void receive_answer() {
}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/
