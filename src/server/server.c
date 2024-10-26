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
#include <pthread.h>

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
server_config config;
cJSON *boards;


int server_fd ;
struct sockaddr_in address;
int addrlen = sizeof(address);

int total_clients_connected_counter = 0;
/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

void server_init(int argc, char **argv);
void setup_socket(void);

void *client_handler(void *arg);
void client_handshake(int client_socket);

/************************************
 * STATIC FUNCTIONS
 ************************************/

int main(int argc, char *argv[]) {
	server_init(argc, argv);
	setup_socket();

	while (true) {
		int new_socket;

		//aceitar connecções
		if((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen)) < 0) {
			perror("Connecção falhada");
			close(server_fd);
			exit(EXIT_FAILURE);
		}
		log_event(config.log_file,"A receber pedido de conecção");
		// thread cliente
		pthread_t thread_id;
		if (pthread_create(&thread_id, NULL, client_handler, (void *)&new_socket) != 0) {
			perror("Failed to create thread");
			log_event(config.log_file,"Criação de thread para connecção falhou!!");
			close(new_socket);
		} else {
			pthread_detach(thread_id);
		}
		//printf(cJSON_Print(get_board_state_by_id(boards,INITIAL_STATE,1)));
	}
	close(server_fd);

	exit(EXIT_SUCCESS);
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

	if (false) {
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
	}


	log_event(config.log_file, "Servidor começou");

	boards = load_boards(config.board_file_path);
	if (boards == NULL) {
		log_event(config.log_file, "Erro ao carregar boards para a memoria!");
		exit(-1);
	}
	log_event(config.log_file,"Boards carregados para memoria com sucesso");
}
void setup_socket(void) {
	log_event(config.log_file, "A começar setup do socket de connecção");
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
		log_event(config.log_file,"Opções do socket falharam");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file,"Opções do socket aplicadas com sucesso");

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(config.port);

	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		log_event(config.log_file,"Socket falhou bind");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file,"Socket deu bind com sucesso");
	//Ouvir pedidos de connecção
	if(listen(server_fd, 5) < 0) { //5 = queue de conecções a se conectar
		log_event(config.log_file,"Listen falhou");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file,"Server começou a ouvir conecções ao servidor");
}

void *client_handler(void *arg) {
	log_event(config.log_file, "Thread para cliente criado");
	const int client_socket = *(int *)arg;
	char buffer[BUFFER_SIZE];

	client_handshake(client_socket);

	while (1) {
		int valread = read(client_socket, buffer, BUFFER_SIZE);
		if (valread <= 0) {
			// Client disconnected or error occurred
			close(client_socket);
			break;
		}

		buffer[valread] = '\0'; // Null-terminate the buffer to make it a proper string
		printf("Received: %s\n", buffer);

	}

	printf("Cliente desconnectado -- Thread %lu acabando\n", pthread_self());
	return NULL;
}

void client_handshake(int client_socket) {
	total_clients_connected_counter++;
	const char *connection_message = "Hello from server";
	send(client_socket, connection_message, strlen(connection_message), 0);


}
