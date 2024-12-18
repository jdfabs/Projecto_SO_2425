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
#include <limits.h>
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

void graceful_shutdown();

void accept_clients();

void *singleplayer_room_handler(void *arg);

void *multiplayer_ranked_room_handler(void *arg);

void *task_handler_multiplayer_ranked(void *arg);

void *multiplayer_casual_room_handler(void *arg);

void *task_handler_multiplayer_casual(void *arg);

void *multiplayer_coop_room_handler(void *arg);

void *task_handler_multiplayer_coop(void *arg);

void create_singleplayer_room(char *room_name);

void create_multiplayer_room(int max_players, char *room_name, multiplayer_room_type_t room_type);

void clean_room_shm(void);

server_config config;
cJSON *boards;
int num_boards;

int server_fd;

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


//BASIC SERVER AUX FUNCTIONS
void server_init(const int argc, char **argv) {
	signal(SIGINT, graceful_shutdown);
	signal(SIGTERM, graceful_shutdown);

	if (argc == 1) {
		printf("Usage: ./server [CONFIG_FILE_NAME]\n");
		log_event("./logs/server_default.json", "Servidor iniciado sem argumentos");
		exit(EXIT_FAILURE);
	}

	printf("Starting server...\n");
	if (load_server_config(argv[1], &config) < 0) {
		fprintf(stderr, "Failed to load server configuration.\n");
		log_event("./logs/server_default.json", "Carregamento de configs falhado -- A FECHAR SERVER");
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
	while (child != NULL) {
		num_boards++;
		child = child->child;
	}

	log_event(config.log_file, "Boards carregados para memoria com sucesso");
	printf("Server started...\n");
}

void setup_socket() {
	log_event(config.log_file, "A começar setup do socket de connecção");
	printf("Setting up socket...\n");
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
	address.sin_port = htons(config.port); // Porta do servidor


	const int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt failed");
		log_event(config.log_file, "setsockopt failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	// Bind ao endereço e porta
	if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
		log_event(config.log_file, "Socket falhou bind");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Socket deu bind com sucesso");

	// Ouvir pedidos de connecção
	if (listen(server_fd, 5) < 0) {
		// 5 = queue de conecções a se conectar
		log_event(config.log_file, "Listen falhou");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	log_event(config.log_file, "Server começou a ouvir conecções ao servidor");
	printf("Server listening for clients...\n");
}

void graceful_shutdown() {
	printf("Shutting down...\n");
	log_event(config.log_file, "Server shutting down gracefully");
	close(server_fd);
	for (int i = 0; i < room_count; i++) {
		// Assuming 'room_count' defines the number of rooms
		char temp[256]; // Buffer for semaphore names
		int result;

		sprintf(temp, "sem_%s_game_start", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking sem_game_start");

		sprintf(temp, "sem_%s_solucao", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking sem_solucao");

		sprintf(temp, "sem_%s_room_full", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking sem_room_full");

		sprintf(temp, "sem_%s_producer", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking sem_producer");

		sprintf(temp, "sem_%s_consumer", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking sem_consumer");

		sprintf(temp, "mut_%s_task", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking mut_task");


		clean_room_shm();


		sprintf(temp, "sem_%s_client", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking sem_client");

		sprintf(temp, "sem_%s_server", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking mut_server");

		sprintf(temp, "mut_%s_task_client", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking sem_task_client");

		sprintf(temp, "mut_%s_task_server", rooms[i].name);
		result = sem_unlink(temp);
		if (result == -1) perror("Error unlinking mut_task_server");
	}

	// Add more cleanup code if necessary
	exit(EXIT_SUCCESS);
}

void accept_clients() {
	struct sockaddr_un address;
	socklen_t addr_len = sizeof(address);
	const int client_socket = accept(server_fd, (struct sockaddr *) &address, &addr_len);
	if (client_socket < 0) {
		perror("Accept failed");
	}
	printf("New connection -");
	//HANDSHAKE
	char aux[100];
	recv(client_socket, aux, 100, 0);
	for (int i = 0; i < room_count; i++) {
		if (rooms[i].current_players < rooms[i].max_players && atoi(aux) == rooms[i].type) {
			printf(" Theres a room with correct type and space - Connecting to: %s\n", rooms[i].name);
			rooms[i].current_players++;
			sprintf(aux, "%d-%d-%s%d", client_socket, rooms[i].current_players - 1, "room_", i);
			send(client_socket, aux, strlen(aux), 0);
			return;
		}
	}
	printf(" No apropriate room - ");

	snprintf(rooms[room_count].name, 255, "room_%d", room_count);
	rooms[room_count].current_players = 1;
	rooms[room_count].type = atoi(aux);

	if (atoi(aux) != 0) rooms[room_count].max_players = config.server_size;
	if (atoi(aux) == 0) {
		printf("Type: SINGLEPLAYER\n");
		create_singleplayer_room(rooms[room_count].name);
	} else if (atoi(aux) == 1) {
		printf("Type: RANKED\n");
		multiplayer_room_type_t room_type = RANKED;
		create_multiplayer_room(rooms[room_count].max_players, rooms[room_count].name, room_type);
	} else if (atoi(aux) == 2) {
		printf("Type: CASUAL\n");
		multiplayer_room_type_t room_type = CASUAL;
		create_multiplayer_room(rooms[room_count].max_players, rooms[room_count].name, room_type);
	} else if (atoi(aux) == 3) {
		printf("Type: COOP");
		multiplayer_room_type_t room_type = COOP;
		create_multiplayer_room(rooms[room_count].max_players, rooms[room_count].name, room_type);
	}

	sprintf(aux, "%d-%d-%s", client_socket, rooms[room_count].current_players - 1, rooms[room_count].name);
	printf("%s\n", aux);
	room_count++;
	send(client_socket, aux, sizeof(aux), 0);
	//Informa o cliente qual é o seu socket (para efeitos de retornar mensagem ao cliente certo)
}

void create_singleplayer_room(char *room_name) {
	room_config_t *room_config = malloc(sizeof(room_config_t));
	if (!room_config) {
		perror("Failed to allocate memory for room_config");
		exit(EXIT_FAILURE);
	}

	room_config->max_players = 1;
	room_config->room_name = malloc(strlen(room_name) + 1);
	if (!room_config->room_name) {
		perror("Failed to allocate memory for room_name");
		free(room_config);
		exit(EXIT_FAILURE);
	}

	sprintf(room_config->room_name, "%s", room_name);

	pthread_t temp_thread;
	if (pthread_create(&temp_thread, NULL, singleplayer_room_handler, room_config) != 0) {
		perror("ptread_create fail");
		free(room_config);
		exit(EXIT_FAILURE);
	}
}

void create_multiplayer_room(int max_players, char *room_name, multiplayer_room_type_t room_type) {
	room_config_t *room_config = malloc(sizeof(room_config_t));
	if (!room_config) {
		perror("Failed to allocate memory for room_config");
		exit(EXIT_FAILURE);
	}

	room_config->max_players = max_players;

	room_config->room_name = malloc(strlen(room_name) + 1);
	if (!room_config->room_name) {
		perror("Failed to allocate memory for room_name");
		free(room_config);
		exit(EXIT_FAILURE);
	}

	sprintf(room_config->room_name, "%s", room_name);

	pthread_t temp_thread;

	void *function;
	switch (room_type) {
		case 0:
			function = multiplayer_ranked_room_handler;
			break;
		case 1:
			function = multiplayer_casual_room_handler;
			break;
		case 2:
			function = multiplayer_coop_room_handler;
			break;
		default:
			perror("Invalid room->room_name");
			exit(EXIT_FAILURE);
	}

	if (pthread_create(&temp_thread, NULL, function, room_config) != 0) {
		perror("pthread_create failed");
		free(room_config->room_name);
		free(room_config);
		exit(EXIT_FAILURE);
	}
}

//AUX Functions
void wait_for_full_room(sem_t *sem, int slots) {
	int value;
	sem_getvalue(sem, &value);
	if (value <= slots) {
		for (int i = 0; i < slots; i++) {
			sem_wait(sem);
		}
	}
}

void clean_room_shm(void) {
	shm_unlink("room_0");
	shm_unlink("room_1");
	shm_unlink("room_2");
	shm_unlink("room_3");
	shm_unlink("room_4");
	shm_unlink("room_5");
	shm_unlink("room_6");
	shm_unlink("room_7");
	shm_unlink("room_8");
	shm_unlink("room_9");
	shm_unlink("room_10");
	shm_unlink("room_11");
	shm_unlink("room_12");
	shm_unlink("room_13");
	shm_unlink("room_14");
	shm_unlink("room_15");
	shm_unlink("room_16");
	shm_unlink("room_17");
	shm_unlink("room_19");
	shm_unlink("room_18");
}

int compare_timespecs(struct timespec *a, struct timespec *b) {
	if (a->tv_sec < b->tv_sec) {
		return -1;
	} else if (a->tv_sec > b->tv_sec) {
		return 1;
	} else {
		// If seconds are equal, compare nanoseconds
		if (a->tv_nsec < b->tv_nsec) {
			return -1;
		} else if (a->tv_nsec > b->tv_nsec) {
			return 1;
		} else {
			return 0;
		}
	}
}

//ROOM FUNCTIONS
//RANKED
void setup_multiplayer_ranked_shared_memory(char room_name[100], multiplayer_ranked_room_shared_data_t **shared_data) {
	const int room_shared_memory = shm_open(room_name, O_CREAT | O_RDWR, 0666);
	if (room_shared_memory == -1) {
		perror("shm_open falhou");
		exit(EXIT_FAILURE);
	}
	if (ftruncate(room_shared_memory, sizeof(multiplayer_ranked_room_shared_data_t)) == -1) {
		perror("ftruncate falhou");
		exit(EXIT_FAILURE);
	}

	*shared_data = mmap(NULL, sizeof(multiplayer_ranked_room_shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED,
						room_shared_memory, 0);
	sprintf((*shared_data)->room_name, room_name);
	(*shared_data)->board_id = -1;
	strcpy((*shared_data)->starting_board, "");

	for (int i = 0; i < 5; i++) {
		//inicializar os valores da fila de espera
		(*shared_data)->task_queue[i].client_socket = -1;
		sprintf((*shared_data)->task_queue[i].request, "\0");
	}

	(*shared_data)->task_consumer_ptr = 0;
	(*shared_data)->task_productor_ptr = 0;
}

void multiplayer_ranked_select_new_board_and_share(multiplayer_ranked_room_shared_data_t *shared_data) {
	srand(time(NULL));
	shared_data->board_id = rand() % num_boards;
	int random_board = shared_data->board_id;

	const cJSON *round_board = cJSON_GetArrayItem(boards, random_board);
	//broadcast new board
	strcpy(shared_data->starting_board, cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));
}

void *multiplayer_ranked_room_handler(void *arg) {
	struct timespec media;
	media.tv_sec = 0;
	media.tv_nsec = 0;
	int time_counter = 0;

	room_config_t *room_config = arg;

	char room_name[100];
	sprintf(room_name, "%s", room_config->room_name);

	const int max_player = room_config->max_players;

	struct timespec start;
	struct timespec end;
	multiplayer_ranked_room_shared_data_t *shared_data;
	pthread_t soltution_checker;

	setup_multiplayer_ranked_shared_memory(room_name, &shared_data);

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

	sprintf(temp, "/mut_%s_task_server", room_name);
	sem_unlink(temp);
	sem_open(temp, O_CREAT | O_RDWR, 0666, 1);

	sprintf(temp, "/mut_%s_task_client", room_name);
	sem_unlink(temp);
	sem_open(temp, O_CREAT | O_RDWR, 0666, 1);

	sprintf(temp, "/sem_%s_producer", room_name);
	sem_unlink(temp);
	sem_open(temp, O_CREAT | O_RDWR, 0666, 5); //MUDAR PARA VARIAVEL NO CONFIG

	sprintf(temp, "/sem_%s_consumer", room_name);
	sem_unlink(temp);
	sem_open(temp, O_CREAT | O_RDWR, 0666, 0);


	//3 consumidores
	pthread_create(&soltution_checker, NULL, task_handler_multiplayer_ranked, shared_data);
	pthread_create(&soltution_checker, NULL, task_handler_multiplayer_ranked, shared_data);
	pthread_create(&soltution_checker, NULL, task_handler_multiplayer_ranked, shared_data);


	printf("%s of type Multiplayer Ranked - started with a max of %d players\n", room_name, max_player);


	wait_for_full_room(sem_room_full, max_player); // Espera que o room encha
	printf("Multiplayer Ranked %s: room full - set the games begin\n", room_name);

	// ReSharper disable once CppDFAEndlessLoop
	for (;;) {
		//sleep(5);
		multiplayer_ranked_select_new_board_and_share(shared_data);

		//Start round
		clock_gettime(CLOCK_MONOTONIC, &start);
		for (int i = 0; i < max_player; i++) {
			sem_post(sem_game_start);
		}
		printf("Multiplayer Ranked %s: game start has been signaled \n", room_name);

		for (int i = 0; i < max_player; i++) {
			sem_wait(sem_solucao_encontrada);
			clock_gettime(CLOCK_MONOTONIC, &end);
			struct timespec final;
			final.tv_sec = end.tv_sec - start.tv_sec;
			final.tv_nsec = end.tv_nsec - start.tv_nsec;

			time_counter++;
			double new_avg = ((media.tv_sec + media.tv_nsec / 1e9) * (time_counter - 1) + (
								final.tv_sec + final.tv_nsec / 1e9)) / time_counter;
			media.tv_sec = (time_t) new_avg;
			media.tv_nsec = (long) ((new_avg - media.tv_sec) * 1e9);

			printf("Novo tempo em %s: %.10f\n", room_name, final.tv_sec + final.tv_nsec / 1e9);
			printf("Media de %s: %.10f\n", room_name, media.tv_sec + media.tv_nsec / 1e9);
		}
	}
}

void *task_handler_multiplayer_ranked(void *arg) {
	multiplayer_ranked_room_shared_data_t *shared_data = arg;
	char temp[255];
	sprintf(temp, "/sem_%s_producer", shared_data->room_name);
	sem_t *sem_prod = sem_open(temp, 0);
	sprintf(temp, "/sem_%s_consumer", shared_data->room_name);
	sem_t *sem_cons = sem_open(temp, 0);
	sprintf(temp, "/mut_%s_task_server", shared_data->room_name);
	sem_t *mutex_task = sem_open(temp, 0);

	//PROBLEMA PRODUTORES CONSUMIDORES--- CONSUMIDOR
	// ReSharper disable once CppDFAEndlessLoop
	while (1) {
		//printf("TASK_HANDLER_MULTIPLAYER_RANKED - %s -- Waiting\n", shared_data->room_name);

		//PREPROTOCOLO
		sem_wait(sem_cons);
		sem_wait(mutex_task);

		//ZONA CRITICA --- ler task

		Task task = shared_data->task_queue[shared_data->task_consumer_ptr];
		shared_data->task_consumer_ptr = (shared_data->task_consumer_ptr + 1) % 5;

		usleep(rand() % 1);
		//POS PROTOCOLO
		sem_post(mutex_task);
		sem_post(sem_prod);

		int **solution = getMatrixFromJSON(
			cJSON_GetObjectItem(cJSON_GetArrayItem(boards, shared_data->board_id), "solution"));

		if (solution[task.request[2] - '0'][task.request[4] - '0'] != task.request[6] - '0') {
			send(task.client_socket, "0", sizeof("0"), 0);
		} else {
			send(task.client_socket, "1", sizeof("1"), 0);
		}
	}
}

//CASUAL
void setup_multiplayer_casual_shared_memory(char room_name[100], multiplayer_casual_room_shared_data_t **shared_data) {
	const int room_shared_memory = shm_open(room_name, O_CREAT | O_RDWR, 0666);
	if (room_shared_memory == -1) {
		perror("shm_open falhou");
		exit(EXIT_FAILURE);
	}
	if (ftruncate(room_shared_memory, sizeof(multiplayer_casual_room_shared_data_t)) == -1) {
		perror("ftruncate falhou");
		exit(EXIT_FAILURE);
	}

	*shared_data = mmap(NULL, sizeof(multiplayer_casual_room_shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED,
						room_shared_memory, 0);
	sprintf((*shared_data)->room_name, room_name);
	(*shared_data)->board_id = -1;
	strcpy((*shared_data)->starting_board, "");

	for (int i = 0; i < 20; i++) {
		//inicializar os valores da fila de espera
		(*shared_data)->task_queue[i].client_socket = -1;
		sprintf((*shared_data)->task_queue[i].request, "\0");
	}
	(*shared_data)->counter = 0;


	for (int i = 0; i < config.server_size; i++) {
		sem_init(&(*shared_data)->sems_client[i], true, 1);
	}

	for (int i = 0; i < config.server_size; i++) {
		sem_init(&(*shared_data)->sems_server[i], true, 0);
	}
	for (int i = 0; i < config.server_size; i++) {
		(*shared_data)->has_solution[i] = false;
	}
}

void multiplayer_casual_select_new_board_and_share(multiplayer_casual_room_shared_data_t *shared_data) {
	srand(time(NULL));
	shared_data->board_id = rand() % num_boards;
	int random_board = shared_data->board_id;

	const cJSON *round_board = cJSON_GetArrayItem(boards, random_board);
	//broadcast new board
	strcpy(shared_data->starting_board, cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));
}

void *multiplayer_casual_room_handler(void *arg) {
	struct timespec media;
	media.tv_sec = 0;
	media.tv_nsec = 0;
	int time_counter = 0;

	room_config_t *room_config = arg;

	char room_name[100];
	sprintf(room_name, "%s", room_config->room_name);

	const int max_player = room_config->max_players;

	struct timespec start;
	struct timespec end;
	multiplayer_casual_room_shared_data_t *shared_data;
	pthread_t soltution_checker;

	setup_multiplayer_casual_shared_memory(room_name, &shared_data);
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

	pthread_create(&soltution_checker, NULL, task_handler_multiplayer_casual, shared_data);

	printf("%s of type Multiplayer Casual - started with a max of %d players\n", room_name, max_player);


	wait_for_full_room(sem_room_full, max_player); // Espera que o room encha
	printf("Multiplayer Casual %s: room full - set the games begin\n", room_name);

	for (;;) {
		//sleep(5);
		multiplayer_casual_select_new_board_and_share(shared_data);

		//Start round
		clock_gettime(CLOCK_MONOTONIC, &start);
		for (int i = 0; i < max_player; i++) {
			sem_post(sem_game_start);
		}
		printf("Multiplayer Casual %s: game start has been signaled \n", room_name);

		for (int i = 0; i < max_player; i++) {
			sem_wait(sem_solucao_encontrada);
			clock_gettime(CLOCK_MONOTONIC, &end);
			struct timespec final;
			final.tv_sec = end.tv_sec - start.tv_sec;
			final.tv_nsec = end.tv_nsec - start.tv_nsec;

			time_counter++;
			double new_avg = ((media.tv_sec + media.tv_nsec / 1e9) * (time_counter - 1) + (
								final.tv_sec + final.tv_nsec / 1e9)) / time_counter;
			media.tv_sec = (time_t) new_avg;
			media.tv_nsec = (long) ((new_avg - media.tv_sec) * 1e9);

			printf("Novo tempo em %s: %.10f\n", room_name, final.tv_sec + final.tv_nsec / 1e9);
			printf("Media de %s: %.10f\n", room_name, media.tv_sec + media.tv_nsec / 1e9);
		}
	}
}

void *task_handler_multiplayer_casual(void *arg) {
	multiplayer_casual_room_shared_data_t *shared_data = arg;

	int current_index = 0;

	// ReSharper disable once CppDFAEndlessLoop
	while (1) {
		//PREPROTOCOLO
		if (shared_data->has_solution[current_index]) {
			current_index = (current_index + 1) % config.server_size;
			continue;
		}
		sem_wait(&shared_data->sems_server[current_index]);
		//ZONA CRITICA --- ler task

		usleep(rand() % 1);
		Task task = shared_data->task_queue[current_index];
		int **solution = getMatrixFromJSON(
			cJSON_GetObjectItem(cJSON_GetArrayItem(boards, shared_data->board_id), "solution"));

		if (task.request[0] == '1') {
			//FOUND SOLUTION - SKIP TO "STEP 6"
		} else if (solution[task.request[2] - '0'][task.request[4] - '0'] != task.request[6] - '0') {
			sprintf(shared_data->task_queue[current_index].request, "0");
		} else {
			sprintf(shared_data->task_queue[current_index].request, "1");
		}
		sleep(0);
		//POS PROTOCOLO
		sem_post(&shared_data->sems_client[current_index]);
		current_index = (current_index + 1) % config.server_size;
	}
}

//COOP
void setup_multiplayer_coop_shared_memory(char room_name[100], multiplayer_coop_room_shared_data_t **shared_data) {
	const int room_shared_memory = shm_open(room_name, O_CREAT | O_RDWR, 0666);
	if (room_shared_memory == -1) {
		perror("shm_open falhou");
		exit(EXIT_FAILURE);
	}
	if (ftruncate(room_shared_memory, sizeof(multiplayer_coop_room_shared_data_t)) == -1) {
		perror("ftruncate falhou");
		exit(EXIT_FAILURE);
	}

	*shared_data = mmap(NULL, sizeof(multiplayer_coop_room_shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED,
						room_shared_memory, 0);
	sprintf((*shared_data)->room_name, room_name);
	(*shared_data)->board_id = -1;
	strcpy((*shared_data)->current_board, "");


	for (int i = 0; i < config.server_size; i++) {
		sem_init(&(*shared_data)->sems_server[i],true, 0);
		sem_init(&(*shared_data)->sems_client[i],true, 1);
		(*shared_data)->task_queue[i].client_socket = -1;
		sprintf((*shared_data)->task_queue[i].request, "\0");
	}
	sem_init(&(*shared_data)->sem_has_requests,true, 0);
}

void multiplayer_coop_select_new_board_and_share(multiplayer_coop_room_shared_data_t *shared_data) {
	srand(time(NULL));
	shared_data->board_id = rand() % num_boards;
	int random_board = shared_data->board_id;

	const cJSON *round_board = cJSON_GetArrayItem(boards, random_board);
	//broadcast new board
	strcpy(shared_data->current_board, cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));
}

void *multiplayer_coop_room_handler(void *arg) {
	struct timespec media;
	media.tv_sec = 0;
	media.tv_nsec = 0;
	int time_counter = 0;

	room_config_t *room_config = arg;

	char room_name[100];
	sprintf(room_name, "%s", room_config->room_name);

	const int max_player = room_config->max_players;

	struct timespec start;
	struct timespec end;
	multiplayer_coop_room_shared_data_t *shared_data;
	pthread_t soltution_checker;

	setup_multiplayer_coop_shared_memory(room_name, &shared_data);
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

	//TODO RESTO DOS SEMAFOROS

	pthread_create(&soltution_checker, NULL, task_handler_multiplayer_coop, shared_data);

	printf("%s of type Multiplayer COOP - started with a max of %d players\n", room_name, max_player);

	wait_for_full_room(sem_room_full, max_player); // Espera que o room encha
	printf("Multiplayer COOP %s: room full - set the games begin\n", room_name);

	for (;;) {
		//sleep(5);
		multiplayer_coop_select_new_board_and_share(shared_data);

		//Start round
		clock_gettime(CLOCK_MONOTONIC, &start);
		for (int i = 0; i < max_player; i++) {
			sem_post(sem_game_start);
		}
		printf("Multiplayer COOP %s: game start has been signaled \n", room_name);


		sem_wait(sem_solucao_encontrada);
		clock_gettime(CLOCK_MONOTONIC, &end);
		struct timespec final;
		final.tv_sec = end.tv_sec - start.tv_sec;
		final.tv_nsec = end.tv_nsec - start.tv_nsec;

		time_counter++;
		double new_avg = ((media.tv_sec + media.tv_nsec / 1e9) * (time_counter - 1) + (
							final.tv_sec + final.tv_nsec / 1e9)) / time_counter;
		media.tv_sec = (time_t) new_avg;
		media.tv_nsec = (long) ((new_avg - media.tv_sec) * 1e9);

		printf("Novo tempo em %s: %.10f\n", room_name, final.tv_sec + final.tv_nsec / 1e9);
		printf("Media de %s: %.10f\n", room_name, media.tv_sec + media.tv_nsec / 1e9);
		//sleep(5);
	}
}

void *task_handler_multiplayer_coop(void *arg) {
	multiplayer_coop_room_shared_data_t *shared_data = arg;

	char temp[255];

	sprintf(temp, "/sem_%s_solucao", shared_data->room_name);
	sem_t *sem_solucao = sem_open(temp, 0);

	struct timespec last_processed[config.server_size];
	for (int i = 0; i < config.server_size; i++) {
		clock_gettime(CLOCK_MONOTONIC, &last_processed[i]);
	}

	struct timespec oldest_request_time;
	int selected_client;

	while (1) {
		selected_client = -1;
		oldest_request_time.tv_sec = LONG_MAX;
		oldest_request_time.tv_nsec = 999999999;

		//PRE
		sem_wait(&shared_data->sem_has_requests);

		for (int i = 0; i < config.server_size; i++) {
			int sem_value;
			sem_getvalue(&shared_data->sems_server[i], &sem_value);

			if (sem_value > 0 && compare_timespecs(&last_processed[i], &oldest_request_time) <= 0) {
				oldest_request_time = last_processed[i];
				selected_client = i;
			}
		}
		sem_wait(&shared_data->sems_server[selected_client]);

		//ZC
		usleep(rand() % 1);
		Task task = shared_data->task_queue[selected_client];
		int **solution = getMatrixFromJSON(
			cJSON_GetObjectItem(cJSON_GetArrayItem(boards, shared_data->board_id), "solution"));

		if (solution[task.request[2] - '0'][task.request[4] - '0'] == task.request[6] - '0') {
			cJSON *old_board = cJSON_Parse(shared_data->current_board);
			int **old_board_matrix = getMatrixFromJSON(old_board);
			cJSON_Delete(old_board); // Properly free all memory allocated by cJSON_Parse

			if (old_board_matrix[task.request[2] - '0'][task.request[4] - '0'] == task.request[6] - '0') {
				//printf("Already had the correct solution\n");
			}
			old_board_matrix[task.request[2] - '0'][task.request[4] - '0'] = task.request[6] - '0';

			cJSON *new_board = convertMatrixToJSON(old_board_matrix);
			char *json_string = cJSON_Print(new_board);
			sprintf(shared_data->current_board, "%s", json_string);

			//check if solution has been reached
			bool has_found_solution = true;
			for (int i = 0; i < 9; i++) {
				for (int j = 0; j < 9; j++) {
					if (old_board_matrix[i][j] == 0) {
						has_found_solution = false;
						goto outside_for;
					}
				}
			}
			outside_for:
			if (has_found_solution) {
				sem_post(sem_solucao);
			}

			free(json_string);
			free(new_board);

			//printf("Correct\n");

			for (int i = 0; i < 9; i++) {
				free(old_board_matrix[i]);
			}
			free(old_board_matrix);
		}

		for (int i = 0; i < 9; i++) {
			free(solution[i]);
		}
		free(solution);


		//POS
		clock_gettime(CLOCK_MONOTONIC, &last_processed[selected_client]);
		sem_post(&shared_data->sems_client[selected_client]);
	}
}


//SINGLEPLAYER
void setup_singleplayer_shared_memory(char room_name[100], singleplayer_room_shared_data_t **shared_data) {
	const int shm = shm_open(room_name, O_CREAT | O_RDWR, 0666);
	if (shm == -1) {
		perror("shared data could not be opened");
		exit(EXIT_FAILURE);
	}

	if (ftruncate(shm, sizeof(singleplayer_room_shared_data_t)) == -1) {
		perror("shared data could not be truncated");
		exit(EXIT_FAILURE);
	}

	*shared_data = mmap(NULL, sizeof(singleplayer_room_shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (*shared_data == MAP_FAILED) {
		perror("mmap failed");
		exit(EXIT_FAILURE);
	}

	// Initialize shared memory fields
	strcpy((*shared_data)->room_name, room_name);
	(*shared_data)->board_id = -1;
	strcpy((*shared_data)->starting_board, "");
	strcpy((*shared_data)->buffer, "");
}

void *task_handler_singleplayer(void *arg) {
	singleplayer_room_shared_data_t *shared_data = arg;

	char temp[255];
	sprintf(temp, "sem_%s_server", shared_data->room_name);
	sem_t *sem_server = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);
	sprintf(temp, "sem_%s_client", shared_data->room_name);
	sem_t *sem_client = sem_open(temp, O_CREAT | O_RDWR, 0666, 1);

	while (true) {
		sem_wait(sem_server);
		//ZONA CRITICA
		usleep(rand() % 1);//TODO SLEEP FROM CONFIG
		//VER SE TA CERTO e manda para o buffer se está certo ou não
		int **solution = getMatrixFromJSON(
			cJSON_GetObjectItem(cJSON_GetArrayItem(boards, shared_data->board_id), "solution"));
		if (solution[shared_data->buffer[2] - '0'][shared_data->buffer[4] - '0'] != shared_data->buffer[6] - '0') {
			strcpy(shared_data->buffer, "0");
		} else {
			strcpy(shared_data->buffer, "1");
		}
		sem_post(sem_client);
	}
}

void *singleplayer_room_handler(void *arg) {
	struct timespec media;
	media.tv_sec = 0;
	media.tv_nsec = 0;
	int time_counter = 0;

	printf("hello from singleplayer thread\n");

	room_config_t *room_config = arg;
	char room_name[100];
	sprintf(room_name, "%s", room_config->room_name);

	struct timespec start;
	struct timespec end;
	singleplayer_room_shared_data_t *shared_data;
	pthread_t solution_checker;

	setup_singleplayer_shared_memory(room_name, &shared_data);

	char temp[255];
	sprintf(temp, "/sem_%s_game_start", room_name);
	sem_unlink(temp);
	sem_t *sem_game_start = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);
	sprintf(temp, "sem_%s_solucao", room_name);
	sem_unlink(temp);
	sem_t *sem_solucao_encontrada = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

	pthread_create(&solution_checker, NULL, task_handler_singleplayer, shared_data);
	printf("%s of type SinglePlayer started - let the games begin\n", room_name);

	while (true) {
		//sleep(5);
		shared_data->board_id = rand() % num_boards;
		int rand_board = shared_data->board_id;

		cJSON *round_board = cJSON_GetArrayItem(boards, rand_board);
		strcpy(shared_data->starting_board, cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));

		//Start Round
		clock_gettime(CLOCK_MONOTONIC, &start);
		sem_post(sem_game_start);

		printf("Single Player %s: game start signaled\n", room_name);
		sem_wait(sem_solucao_encontrada);

		clock_gettime(CLOCK_MONOTONIC, &end);
		struct timespec final;
		final.tv_sec = end.tv_sec - start.tv_sec;
		final.tv_nsec = end.tv_nsec - start.tv_nsec;

		time_counter++;
		double new_avg = ((media.tv_sec + media.tv_nsec / 1e9) * (time_counter - 1) + (
							final.tv_sec + final.tv_nsec / 1e9)) / time_counter;
		media.tv_sec = (time_t) new_avg;
		media.tv_nsec = (long) ((new_avg - media.tv_sec) * 1e9);

		printf("Novo tempo em %s: %.10f\n", room_name, final.tv_sec + final.tv_nsec / 1e9);
		printf("Media de %s: %.10f\n", room_name, media.tv_sec + media.tv_nsec / 1e9);
	}
}
