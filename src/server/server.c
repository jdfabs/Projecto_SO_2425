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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <pthread.h>
#include <semaphore.h>

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
struct sockaddr_un address;

typedef struct {
	int client_socket;
	char request[BUFFER_SIZE];
} Task;

Task task_queue[];
int task_creator_ptr=0, task_reader_ptr=0;
pthread_mutex_t mutex_task_creators = PTHREAD_MUTEX_INITIALIZER, mutex_task_reader = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_task_creators, sem_task_reader;

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

void server_init(int argc, char **argv, Task *task_queue);
void setup_socket();
void *client_handler(void *arg);
void *task_handler ();

/************************************
 * STATIC FUNCTIONS
 ************************************/

/*void * TEST_SEND_MESSAGE_FROM_OTHER_THREAD(void * p) {
	printf("hi\n");
	while(task_queue[0].client_socket == -1 ) {
		//printf("waiting\n");
	}
	send(task_queue[0].client_socket, "message from the other thread", sizeof("message from the other thread"), 0);
}*/

int main(int argc, char *argv[]) {
	server_init(argc, argv, task_queue); // Server data structures setup
	setup_socket(); // Ready to accept connections



	pthread_t thread_id;
	for(int i = 0; i < config.task_handler_threads; i++) {
		pthread_create(&thread_id, NULL, task_handler , (int *)i);
	}

	//pthread_create(&thread_id, NULL, solution_checker ,NULL);


	while (true) {
		client_message *client_info = malloc(sizeof(client_message));
		if (!client_info) {
			perror("Failed to allocate memory for client_data");
			continue; // handle error
		}

		// Accept a new client connection
		socklen_t addr_len = sizeof(address);
		client_info->client_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len);
		if (client_info->client_socket < 0) {
			perror("Accept failed");
			free(client_info); // Cleanup allocated memory
			continue; // handle error
		}

		//printf("Client %d connected\n", client_info->client_socket);


		// Create a new thread for the client
		if (pthread_create(&thread_id, NULL, client_handler, (void *)client_info) != 0) {
			perror("Failed to create thread");
			close(client_info->client_socket);
			free(client_info); // Cleanup allocated memory
			continue; // handle error
		}

		pthread_detach(thread_id); // Detach the thread so it cleans up after itself
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

//SETUP FUNCTIONS
void server_init(int argc, char **argv, Task *task_queue) {
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
	log_event(config.log_file, "Servidor começou");

	task_queue = (Task *)malloc(config.task_queue_size * sizeof(Task));
	if (task_queue == NULL) {
		fprintf(stderr, "Memory allocation failed\n");

		log_event(config.log_file, "Criação do array de tasks falhou!");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < config.task_queue_size; i++) { //inicializar os valores do array
		task_queue[i].client_socket = -1;
		sprintf(task_queue[i].request, "\0");
	}

	sem_init(&sem_task_creators,0,config.task_queue_size);
	sem_init(&sem_task_reader,0,0);

	boards = load_boards(config.board_file_path);
	if (boards == NULL) {
		printf("Failed to load boards from %s\n", config.board_file_path);
		log_event(config.log_file, "Erro ao carregar boards!");
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file,"Boards carregados para memoria com sucesso");
}
void setup_socket() {
	log_event(config.log_file, "A começar setup do socket de connecção");

	if((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0) {
		perror("Socket falhou");
		log_event(config.log_file, "Criação do socket falhou!");
		exit(EXIT_FAILURE);
	}

	char temp[255];
	sprintf(temp, "Socket criado com sucesso. socket_fd: %d", server_fd);
	log_event(config.log_file, temp);

	//Definir ficheiro local para o socket
	address.sun_family= AF_UNIX;
	strncpy(address.sun_path, "/tmp/local_socket", sizeof(address.sun_path)-1);

	//Desconectar socket antigo (just in case)
	unlink(address.sun_path);

	//Bind ao socket local
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

//CLIENT HANDLER FUNCTIONS
void client_handshake(int socket) {
	char socket_str[20];
	sprintf(socket_str, "%d", socket);
	send(socket, socket_str, sizeof(socket_str), 0); //envia o socket do cliente
	recv(socket, socket_str, sizeof(socket_str), 0);
	printf("game type: %s\n", socket_str);

}
int receive_message(client_message *message) {
	ssize_t bytes_read = recv(message->client_socket, message->message, sizeof(message->message)-1, 0);
	if (bytes_read <= 0) {
		//printf("Erro ao receber mensagem com sucesso");
		return -1;
	}
	message->message[bytes_read] = '\0';
	return 0;
}

void *client_handler(void *arg) {
	char log_var_aux[255];
	client_message *client_info = (client_message *)arg;
	client_handshake(client_info->client_socket); // Handshake entre cliente e server

	printf("Cliente  com sucesso\n");
	while (1) { //Vai Receber todas as mensagens que o cliente mandar e mete-las na queue
		if (receive_message(client_info)== -1) {
			log_event(config.log_file, "Erro a rebecer mensagem/ cliente desligado!");
			return NULL; //erro com cliente, acabar com thread
		}

		sprintf(log_var_aux, "Mensagem recebida de %d: %s", client_info->client_socket ,client_info->message);
		log_event(config.log_file, log_var_aux);

		//PROBLEMA PRODUTORES CONSUMIDORES--- PRODUTOR
		printf("CLIENT_HANDLER socket: %d task recebida -- esperar para escrever\n", client_info->client_socket);
		sem_wait(&sem_task_creators);
		pthread_mutex_lock(&mutex_task_creators);

		//ZONA CRITICA -- criar a tasks
		printf("CLIENT_HANDLER socket: %d a escrever task\n",client_info->client_socket);
		task_queue[task_creator_ptr].client_socket = client_info->client_socket;
		sprintf(task_queue[task_creator_ptr].request, "%s", client_info->message);
		task_creator_ptr = (task_creator_ptr + 1) % config.task_queue_size;
		//FIM ZONA CRITICA
		pthread_mutex_unlock(&mutex_task_creators);
		sem_post(&sem_task_reader);

		//TO-ASK como isto fica fora da zona critica, pode ser que fique logged fora de ordem 🤔 era de mudar isto?
		sprintf(log_var_aux, "Nova task de %d adicionada na fila.",client_info->client_socket);
		log_event(config.log_file, log_var_aux);
	}
}


//TASK MANAGERS
void *task_handler (int id) {
	//PROBLEMA PRODUTORES CONSUMIDORES--- CONSUMIDOR
	while (1) {
		printf("TASK_HANDLER%d -- Waiting for something to read\n", id);

		sem_wait(&sem_task_reader);
		pthread_mutex_lock(&mutex_task_reader);
		//ZONA CRITICA --- ler task
		Task task = task_queue[task_reader_ptr];
		task_reader_ptr = (task_reader_ptr + 1) % config.task_queue_size;

		printf("SOLUTION_CHECKER -- Something to read from: %d\n", task.client_socket);
		pthread_mutex_unlock(&mutex_task_reader);
		sem_post(&sem_task_creators);
		printf("SOLUTION_CHECKER -- Processing (sleep 10secs) from: %d\n", task.client_socket);
		sleep(11);
		char temp[255];
		sprintf(temp, "request %s processed!", task.request);
		send(task.client_socket,temp, strlen(temp), 0);

	}
}


