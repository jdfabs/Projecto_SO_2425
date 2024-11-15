/*********************************************************************************
 * server.c
 * Skipper
 * 11/10/2024
 * Server entry-point
 *********************************************************************************/

#include "server.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>

typedef struct {
	int max_players;
	char *room_name;
} room_config_t;



server_config config;
cJSON *boards;
int num_boards;

int server_fd ;
struct sockaddr_un address;
pthread_t multiplayer_rooms[10];
/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

void server_init(int argc, char **argv);
void setup_socket();
void accept_clients();

void *task_handler (void *arg);
void create_new_room(int max_players, char *room_name );
void *room_handler(void *arg);

typedef struct {
	char name[255];
	int type;
	int max_players;
	int current_players;
	pthread_t thread_id;
} room_t;

room_t rooms[20];
int room_count = 0;

int main(int argc, char *argv[]) {
	server_init(argc, argv); // Server data structures setup
	setup_socket(); // Ready to accept connections

	//TODO -- SETUP MORE ROOMS
	//Setup multiplayer rooms
	//create_new_room(2, "room_1");

	while (true) {
		accept_clients(); //Aceitar connectões e handshake
	}

	close(server_fd);
	exit(EXIT_SUCCESS);
}

//BASIC SERVER AUX FUNCTIONS
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
	log_event(config.log_file, "Servidor começou");



	boards = load_boards(config.board_file_path);
	if (boards == NULL) {
		printf("Failed to load boards from %s\n", config.board_file_path);
		log_event(config.log_file, "Erro ao carregar boards!");
		exit(EXIT_FAILURE);
	}

	cJSON *child = boards->child;
	while (child !=NULL) {
		num_boards++;
		child = child->child;
	}



	log_event(config.log_file,"Boards carregados para memoria com sucesso");
}
void setup_socket() {
	log_event(config.log_file, "A começar setup do socket de connecção");

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("Socket falhou");
		log_event(config.log_file, "Criação do socket falhou!");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	char temp[255];
	sprintf(temp, "Socket criado com sucesso. socket_fd: %d", server_fd);
	log_event(config.log_file, temp);

	// Configurar o endereço IP e porta do servidor
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY; // Aceitar conexões de qualquer IP
	address.sin_port = htons(config.port);      // Porta do servidor

	// Bind ao endereço e porta
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		log_event(config.log_file, "Socket falhou bind");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Socket deu bind com sucesso");

	// Ouvir pedidos de connecção
	if (listen(server_fd, 5) < 0) { // 5 = queue de conecções a se conectar
		log_event(config.log_file, "Listen falhou");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Server começou a ouvir conecções ao servidor");
}
void accept_clients() {
	socklen_t addr_len = sizeof(address);
	int client_socket = accept(server_fd, (struct sockaddr *) &address, &addr_len);
	if (client_socket  < 0) {
		perror("Accept failed");
	}


	//HANDSHAKE
	char aux[100];
	recv(client_socket,aux , 100, 0);
	printf("%s\n",aux);


	for(int i = 0; i < room_count ; i++) {
		if(rooms[i].current_players < rooms[i].max_players) {
			printf("ROOM WITH SPACE\n");
			sprintf(aux, "%d-%s%d", client_socket, "room_",i);
			rooms[i].current_players++;
			send(client_socket, aux, strlen(aux), 0);
			return;
		}
	}
	printf("NO APROPRIATE ROOMS-CREATING NEW ROOM\n");
	snprintf(rooms[room_count].name,255, "room_%d", room_count);
	rooms[room_count].current_players = 1;
	rooms[room_count].max_players = 1;
	rooms[room_count].type = atoi(aux);

	create_new_room(rooms[room_count].max_players, rooms[room_count].name);

	sprintf(aux, "%d-%s", client_socket, rooms[room_count].name);


	room_count++;
	send(client_socket, aux, sizeof(aux), 0); //Informa o cliente qual é o seu socket (para efeitos de retornar mensagem ao cliente certo)
}
void create_new_room(int max_players, char *room_name) {
	room_config_t *room_config;
	room_config = malloc(sizeof(room_config_t));
	if (!room_config) {
		perror("Failed to allocate memory for room_config");
		exit(EXIT_FAILURE);
	}

	room_config->max_players = max_players;

	// Allocate memory for room_name
	room_config->room_name = malloc(strlen(room_name) + 1); // +1 for null terminator
	if (!room_config->room_name) {
		perror("Failed to allocate memory for room_name");
		free(room_config); // Free previously allocated memory
		exit(EXIT_FAILURE);
	}

	// Copy the room name into allocated memory
	sprintf(room_config->room_name, "%s", room_name);

	pthread_t temp_thread;
	int ret = pthread_create(&temp_thread, NULL, room_handler, (void *) room_config);
	if (ret != 0) {
		perror("pthread_create failed");
		free(room_config->room_name); // Clean up allocated memory
		free(room_config);
		exit(EXIT_FAILURE);
	}
}


//TASK MANAGERS
void *task_handler(void *arg) {
	// Cast the void pointer to the appropriate type
	multiplayer_room_shared_data_t *shared_data = (multiplayer_room_shared_data_t *)arg;

	char aux[100];
	sprintf(aux, "/sem_%s_producer", shared_data->room_name);
	sem_unlink(aux);
	sem_t *sem_prod = sem_open(aux, O_CREAT | O_RDWR, 0666,5);


	sprintf(aux, "/sem_%s_consumer", shared_data->room_name);
	sem_unlink(aux);
	sem_t *sem_cons = sem_open(aux, O_CREAT | O_RDWR, 0666, 0);


	int **solution;

	//PROBLEMA PRODUTORES CONSUMIDORES--- CONSUMIDOR
	while (1) {
		printf("TASK_HANDLER -- Waiting for something to read\n");

		//PREPROTOCOLO
		sem_wait(sem_cons);
		pthread_mutex_lock(&shared_data->mutex_task_reader);

		//ZONA CRITICA --- ler task
		Task task = shared_data->task_queue[shared_data->task_consumer_ptr];
		shared_data->task_consumer_ptr = (shared_data->task_consumer_ptr + 1) % 5;

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
			sem_post(&shared_data->sem_game_start); //TODO FIX THIS SHIT
		}
		printf("Multiplayer Room Handler: game start has been signaled \n");

		for(int i = 0; i < max_player; i++) {
			sem_wait(sem_solucao_encontrada);
			clock_gettime(CLOCK_MONOTONIC, &end);
			printf("%.2f\n",(end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9) ;
		}

	}

	shared_memory_clean_up(room_name, shared_data);
}


