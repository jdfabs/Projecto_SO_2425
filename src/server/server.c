/*********************************************************************************
 * server.c
 * Skipper
 * 11/10/2024
 * Server entry-point
 *********************************************************************************/

#include <server.h>
#include <file_handler.h>
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
#include <signal.h>

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
void server_init(int argc, char **argv);
void setup_socket();
void accept_clients();
void *task_handler (void *arg);
void create_multiplayer_ranked_room(int max_players, char *room_name );
void *multiplayer_ranked_room_handler(void *arg);

server_config config;
cJSON *boards;
int num_boards;

int server_fd ;
room_t rooms[20];
int room_count = 0;

int main(const int argc, char *argv[]) {
	server_init(argc, argv); // Server data structures setup
	setup_socket(); // Ready to accept connections

	// ReSharper disable once CppDFAEndlessLoop
	while (true) {
		accept_clients(); //Aceitar connectões e handshake
	}
}
void graceful_shutdown() {
	printf("Shutting down...\n");
	log_event(config.log_file, "Server shutting down gracefully");
	close(server_fd);
	for (int i = 0; i < room_count; i++) {
		char temp[100];
		sprintf(temp, "sem_%s_game_start", rooms[i].name);
		sem_unlink(temp);
		sprintf(temp, "sem_%s_solucao", rooms[i].name);
		sem_unlink(temp);
		sprintf(temp, "sem_%s_room_full", rooms[i].name);
		sem_unlink(temp);
		sprintf(temp, "sem_%s_producer", rooms[i].name);
		sem_unlink(temp);
		sprintf(temp, "sem_%s_consumer", rooms[i].name);
		sem_unlink(temp);
		sprintf(temp, "mut_%s_task", rooms[i].name);
		sem_unlink(temp);

	}
	// Add more cleanup code if necessary
	exit(EXIT_SUCCESS);
}
//BASIC SERVER AUX FUNCTIONS
void server_init(const int argc, char **argv) {
	signal(SIGINT, graceful_shutdown);
	signal(SIGTERM, graceful_shutdown);

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

	const cJSON *child = boards->child;
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


	const int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt failed");
		log_event(config.log_file, "setsockopt failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}


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
	struct sockaddr_un address;
	socklen_t addr_len = sizeof(address);
	const int client_socket = accept(server_fd, (struct sockaddr *) &address, &addr_len);
	if (client_socket  < 0) {
		perror("Accept failed");
	}


	//HANDSHAKE
	char aux[100];
	recv(client_socket,aux , 100, 0);
	printf("%s\n",aux);


	for(int i = 0; i < room_count ; i++) {
		if(rooms[i].current_players < rooms[i].max_players && atoi(aux) == rooms[i].type){
			printf("ROOM WITH SPACE && CORRECT TYPE\n");
			sprintf(aux, "%d-%s%d", client_socket, "room_",i);
			rooms[i].current_players++;
			send(client_socket, aux, strlen(aux), 0);
			return;
		}
	}
	printf("NO APROPRIATE ROOMS-CREATING NEW ROOM\n");

	snprintf(rooms[room_count].name,255, "room_%d", room_count);
	rooms[room_count].current_players = 1;
	rooms[room_count].type = atoi(aux);

	if(atoi(aux) == 2) {
		printf("CREATING ROOM OF TYPE SINGLEPLAYER\n");
		rooms[room_count].type = 1;
		printf("NAO IMPLEMENTADO!!!\n");
	}
	else {
	rooms[room_count].max_players = 2;
		if(atoi(aux) == 1) {
			printf("CREATING ROOM OF TYPE RANKED\n");
			create_multiplayer_ranked_room(rooms[room_count].max_players, rooms[room_count].name);
		}
		else if(atoi(aux) == 2) {
			printf("CREATING ROOM OF TYPE CASUAL\n");
			printf("NAO IMPLEMENTADO!!!\n");
		}
	}

	sprintf(aux, "%d-%s", client_socket, rooms[room_count].name);
	room_count++;
	send(client_socket, aux, sizeof(aux), 0); //Informa o cliente qual é o seu socket (para efeitos de retornar mensagem ao cliente certo)
}
void create_multiplayer_ranked_room(int max_players, char *room_name) {
	room_config_t *room_config = malloc(sizeof(room_config_t));
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
	if (pthread_create(&temp_thread, NULL, multiplayer_ranked_room_handler, room_config) != 0) {
		perror("pthread_create failed");
		free(room_config->room_name); // Clean up allocated memory
		free(room_config);
		exit(EXIT_FAILURE);
	}
}


//TASK MANAGERS
void *task_handler(void *arg) {
	// Cast the void pointer to the appropriate type
	multiplayer_room_shared_data_t *shared_data = arg;
	char temp[255];
	sprintf(temp, "/sem_%s_producer", shared_data->room_name);
	sem_t *sem_prod = sem_open(temp, 0);
	sprintf(temp, "/sem_%s_consumer", shared_data->room_name);
	sem_t *sem_cons = sem_open(temp, 0);
	sprintf(temp, "/mut_%s_task", shared_data->room_name);
	sem_t *mutex_task = sem_open(temp, 0);

	//PROBLEMA PRODUTORES CONSUMIDORES--- CONSUMIDOR
	// ReSharper disable once CppDFAEndlessLoop
	while (1) {
		printf("TASK_HANDLER -- Waiting for something to read\n");

		//PREPROTOCOLO
		sem_wait(sem_cons);
		sem_wait(mutex_task);

		//ZONA CRITICA --- ler task
		Task task = shared_data->task_queue[shared_data->task_consumer_ptr];
		shared_data->task_consumer_ptr = (shared_data->task_consumer_ptr + 1) % 5;

		//POS PROTOCOLO
		sem_post(mutex_task);
		sem_post(sem_prod);

		printf("%s\n",task.request);

		int **solution = getMatrixFromJSON(
			cJSON_GetObjectItem(cJSON_GetArrayItem(boards, shared_data->board_id), "solution"));

		if(solution[task.request[2]- '0'][task.request[4] - '0'] != task.request[6] - '0') {
			send(task.client_socket, "0", sizeof("0"), 0);
		}
		else {
			send(task.client_socket, "1", sizeof("1"), 0);
		}
	}
}

//ROOM FUNCTIONS
void setup_shared_memory(char room_name[100], multiplayer_room_shared_data_t **shared_data) {
	const int room_shared_memory = shm_open(room_name, O_CREAT | O_RDWR,0666);
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

	for (int i = 0; i < 5; i++) { //inicializar os valores da fila de espera
		(*shared_data)->task_queue[i].client_socket = -1;
		sprintf((*shared_data)->task_queue[i].request, "\0");
	}

	(*shared_data)->task_consumer_ptr = 0;
	(*shared_data)->task_productor_ptr = 0;
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
	srand(time(NULL));
	shared_data->board_id = rand()% num_boards;
	int random_board = shared_data->board_id;

	const cJSON *round_board = cJSON_GetArrayItem(boards, random_board);
	//broadcast new board
	strcpy(shared_data->starting_board,cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));
}

void *multiplayer_ranked_room_handler(void *arg) {
	room_config_t *room_config =arg;

	const char room_name[100];
	sprintf(room_name,"%s",room_config->room_name);

	const int max_player = room_config->max_players;

	struct timespec start;
	struct timespec end;
	multiplayer_room_shared_data_t *shared_data;
	pthread_t soltution_checker;

	setup_shared_memory(room_name, &shared_data);

	char temp[255];
	sprintf(temp, "/sem_%s_solucao", room_name);
	sem_unlink(temp);
	sem_t *sem_solucao_encontrada = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

	sprintf(temp, "/sem_%s_room_full", room_name);
	sem_unlink(temp);
	sem_t *sem_room_full = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

	sprintf(temp, "/sem_%s_game_start", room_name);
	sem_unlink(temp);
	sem_t *sem_game_start = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

	sprintf(temp, "/mut_%s_task", room_name);
	sem_unlink(temp);
	sem_open(temp, O_CREAT | O_RDWR, 0666, 1);

	sprintf(temp, "/sem_%s_producer", room_name);
	sem_unlink(temp);
	sem_open(temp, O_CREAT | O_RDWR, 0666,5);

	sprintf(temp, "/sem_%s_consumer", room_name);
	sem_unlink(temp);
	sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

	pthread_create(&soltution_checker, NULL, task_handler, shared_data);
	printf("MULTIPLAYER ROOM %s started with a max of %d players\n", room_name, max_player);



	wait_for_full_room(sem_room_full, max_player); // Espera que o room encha
	printf("ROOM_HANDLER %s: Full room - games are starting\n",room_name);

	// ReSharper disable once CppDFAEndlessLoop
	for(;;) {
		printf("ROOM_HANDLER %s: NEW GAME!!",room_name);
		sleep(5);
		select_new_board_and_share(shared_data);

		//Start round
		clock_gettime(CLOCK_MONOTONIC, &start);
		for(int i = 0; i < max_player; i++) {
			sem_post(sem_game_start); //TODO FIX THIS SHIT
		}
		printf("Multiplayer Room Handler: game start has been signaled \n");

		for(int i = 0; i < max_player; i++) {
			sem_wait(sem_solucao_encontrada);
			clock_gettime(CLOCK_MONOTONIC, &end);
			printf("%.10f\n",end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1e9) ;
		}
	}
}


