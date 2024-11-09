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

	/*
	server_init(argc, argv, task_queue); // Server data structures setup
	setup_socket(); // Ready to accept connections



	pthread_t thread_id;
	pthread_create(&thread_id, NULL, task_handler ,NULL);
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
	*/


	generate_sudoku();

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


	while (1) { //Vai Receber todas as mensagens que o cliente mandar
		if (receive_message(client_info)== -1) {
			log_event(config.log_file, "Erro a rebecer mensagem/ cliente desligado!");
			return NULL; //erro com cliente, acabar com thread
		}

		sprintf(log_var_aux, "Mensagem recebida de %d: %s", client_info->client_socket ,client_info->message);
		log_event(config.log_file, log_var_aux);

		//PROBLEMA LEITORES ESCRITORES --- ESCRITOR
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
/*
	log_event(config.log_file, "Thread para cliente criado");
	const int client_socket = *(int *)arg;

	printf("client socket: %d\n", client_socket);
	char buffer[BUFFER_SIZE];

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
	return NULL;*/
}


//TASK MANAGERS
void *task_handler () {
	//PROBLEMA LEITORES ESCRITORES ---- LEITOR!!
	while (1) {
		printf("SOLUTION_CHECKER -- Waiting for something to read\n");

		sem_wait(&sem_task_reader);
		pthread_mutex_lock(&mutex_task_reader);
		//ZONA CRITICA --- ler task
		Task task = task_queue[task_reader_ptr];
		task_reader_ptr = (task_reader_ptr + 1) % config.task_queue_size;

<<<<<<< HEAD
		printf("SOLUTION_CHECKER -- Something to read from: %d\n", task.client_socket);
		pthread_mutex_unlock(&mutex_task_reader);
		sem_post(&sem_task_creators);
		printf("SOLUTION_CHECKER -- Processing (sleep 10secs) from: %d\n", task.client_socket);
		//sleep(10);
		char temp[255];
		sprintf(temp, "request %s processed!", task.request);
		send(task.client_socket,temp, strlen(temp), 0);
=======
		//POS PROTOCOLO
		pthread_mutex_unlock(&shared_data->mutex_task_reader);
		sem_post(sem_prod);

		printf("%s\n",task.request);

		solution = getMatrixFromJSON(cJSON_GetObjectItem(cJSON_GetArrayItem(boards, shared_data->board_id),"solution"));

		int i = task.request[2]- '0';
		int j = task.request[4] - '0';
		int k = task.request[6] - '0';

		if(solution[i][j] != k) {
			printf("nope\n");
			send(task.client_socket, "0", sizeof("0"), 0);
		}
		else {
			printf("yep\n");
			send(task.client_socket, "1", sizeof("1"), 0);
		}
	}
}

//ROOM FUNCTIONS
void setup_shared_memory(char room_name[100], multiplayer_room_shared_data_t **shared_data) {
	int room_shared_memory = shm_open(room_name, O_CREAT | O_RDWR,0666);
	if(room_shared_memory == -1) {
		perror("shm_open falhou");
		exit(EXIT_FAILURE);
	}
	if(ftruncate(room_shared_memory, sizeof(multiplayer_room_shared_data_t)) == -1) {
		perror("ftruncate falhou");
		exit(EXIT_FAILURE);
	}

	*shared_data = mmap(NULL, sizeof( multiplayer_room_shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED,room_shared_memory, 0);

	sprintf((*shared_data)->room_name,room_name);
	(*shared_data)->board_id = -1;
	strcpy((*shared_data)->starting_board, "");

	sem_init(&(*shared_data)->sem_game_start, 1, 0);
	sem_init(&(*shared_data)->sem_room_full, 1, 0);
	sem_init(&(*shared_data)->sem_found_solution, 1,0);

	for (int i = 0; i < 5; i++) { //inicializar os valores da fila de espera
		(*shared_data)->task_queue[i].client_socket = -1;
		sprintf((*shared_data)->task_queue[i].request, "\0");
	}

	(*shared_data)->task_consumer_ptr = 0;
	(*shared_data)->task_productor_ptr = 0;
	pthread_mutex_init(&(*shared_data)->mutex_task_creators, NULL);
	pthread_mutex_init(&(*shared_data)->mutex_task_reader, NULL);
}
void room_setup(void *arg, char(*room_name)[100], int *max_player, struct timespec *start, struct timespec *end, multiplayer_room_shared_data_t **shared_data, sem_t **sem_solucao_encontrada, pthread_t *soltution_checker) {
	room_config_t *room_config =arg;
	sprintf(*room_name,"%s",room_config->room_name);
	*max_player = room_config->max_players;

	setup_shared_memory(*room_name, &*shared_data);
	char temp[255];
	sprintf(temp, "/sem_%s_solucao", *room_name);
	*sem_solucao_encontrada = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

	pthread_create(&*soltution_checker, NULL, task_handler, (void *)*shared_data);

	printf("MULTIPLAYER ROOM %s started with a max of %d players\n", *room_name, *max_player);
}

void wait_for_full_room(sem_t *sem, int slots) {
	int value;
	sem_getvalue(sem,&value);
	if(value <= slots) {
		for(int i = 0; i < slots; i++) {
			sem_wait(sem);
		}
	}

}
void select_new_board_and_share(multiplayer_room_shared_data_t *shared_data) {
	cJSON *round_board;

	srand(time(NULL));
	shared_data->board_id = rand()% num_boards;
	int random_board = shared_data->board_id;

	round_board = cJSON_GetArrayItem(boards, random_board);
	//broadcast new board
	strcpy(shared_data->starting_board,cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));
}

void shared_memory_clean_up(char room_name[100], multiplayer_room_shared_data_t *shared_data) { //TODO -- MISSING PROD_CONS VARS
	sem_destroy(&shared_data->sem_game_start);
	sem_destroy(&shared_data->sem_room_full);
	munmap(shared_data, sizeof(multiplayer_room_shared_data_t));
	shm_unlink(room_name);
}


void *room_handler(void *arg) {
	char room_name[100];
	int max_player;
	struct timespec start;
	struct timespec end;
	multiplayer_room_shared_data_t *shared_data;
	sem_t *sem_solucao_encontrada;
	pthread_t soltution_checker;
	room_setup(arg, &room_name, &max_player, &start, &end, &shared_data, &sem_solucao_encontrada, &soltution_checker);


	wait_for_full_room(&shared_data->sem_room_full, max_player); // Espera que o room encha
	printf("ROOM_HANDLER %s: Full room - games are starting\n",room_name);

	for(;;) {
		printf("ROOM_HANDLER %s: NEW GAME!!",room_name);
		sleep(5);
		select_new_board_and_share(shared_data);

		//Start round
		clock_gettime(CLOCK_MONOTONIC, &start);
		for(int i = 0; i < max_player; i++) {
			sem_post(&shared_data->sem_game_start);
		}
		printf("Multiplayer Room Handler: game start has been signaled \n");

		for(int i = 0; i < max_player; i++) {
			sem_wait(sem_solucao_encontrada);
			clock_gettime(CLOCK_MONOTONIC, &end);
			printf("%.2f\n",(end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9) ;
		}
>>>>>>> parent of 7e38406 (UI Basica do Cliente)

	}
}
