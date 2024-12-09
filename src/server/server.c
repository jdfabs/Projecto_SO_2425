/*********************************************************************************
 * server.c
 * Skipper
 * 11/10/2024
 * Server entry-point
 *********************************************************************************/

#include <client.h>
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

#define separator() printf("--------------------------\n")
/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
void server_init(int argc, char **argv);
void setup_server_main_socket();
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
void *client_handler(room_t *room, int client_socket, int client_id);
void create_new_room(int client_socket, int room_type);

server_config config;
cJSON *boards;
int num_boards;

int server_fd;

room_t rooms[100];
int room_count = 0;

int main(const int argc, char *argv[]) {
	server_init(argc, argv); // Server data structures setup
	setup_server_main_socket(); // Ready to accept connections

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

void setup_server_main_socket() {
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
		char temp[256];

		const char *semaphores[] = {"game_start", "solucao", "room_full", "producer", "consumer", "task", "client", "server", "task_client", "task_server"};
		for (int j = 0; j < sizeof(semaphores) / sizeof(semaphores[0]); j++) {
			sprintf(temp, "sem_%s_%s", rooms[i].name, semaphores[j]);
			sem_unlink(temp);
		}
		clean_room_shm();
	}
	log_event(config.log_file, "Server Gracefully Shutdown");
	exit(EXIT_SUCCESS);
}

void accept_clients() {
    struct sockaddr_un address;
    socklen_t addr_len = sizeof(address);
    int client_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len);

    if (client_socket < 0) {
        perror("Accept failed");
        return;
    }

    log_event(config.log_file, "Receiving new connection");
    printf("New connection -");

    // HANDSHAKE
    char client_request[100];
    if (recv(client_socket, client_request, sizeof(client_request), 0) <= 0) {
        perror("Failed to receive client handshake");
        close(client_socket);
        return;
    }

    int requested_type = atoi(client_request);

    for (int i = 0; i < room_count; i++) {
        if (rooms[i].current_players < rooms[i].max_players && rooms[i].type == requested_type) {
            printf(" Room with correct type and space found - Connecting to: %s\n", rooms[i].name);
            rooms[i].current_players++;

            if (fork() == 0) {
                client_handler(&rooms[i], client_socket, rooms[i].current_players - 1);
                exit(0);
            }

            char log_msg[255];
            snprintf(log_msg, sizeof(log_msg),
                     "Client assigned to room: %s, ID: %d, Socket: %d",
                     rooms[i].name, rooms[i].current_players - 1, client_socket);
            log_event(config.log_file, log_msg);

            return;
        }
    }

    printf(" No appropriate room found - Creating new room\n");
    create_new_room(client_socket, requested_type);
    log_event(config.log_file, "Client handshake completed");
}

void create_new_room(int client_socket, int room_type) {
    snprintf(rooms[room_count].name, sizeof(rooms[room_count].name), "room_%d", room_count);
    rooms[room_count].current_players = 1;
    rooms[room_count].type = room_type;
    rooms[room_count].max_players = (room_type == 0) ? 1 : config.server_size;

    const char *room_type_str = NULL;
    switch (room_type) {
        case 0:
            room_type_str = "SINGLEPLAYER";
            create_singleplayer_room(rooms[room_count].name);
            log_event(config.log_file, "New Singleplayer room created and first client connected");
            break;
        case 1:
            room_type_str = "RANKED";
            create_multiplayer_room(rooms[room_count].max_players, rooms[room_count].name, RANKED);
            log_event(config.log_file, "New Ranked room created and first client connected");
            break;
        case 2:
            room_type_str = "CASUAL";
            create_multiplayer_room(rooms[room_count].max_players, rooms[room_count].name, CASUAL);
            log_event(config.log_file, "New Casual room created and first client connected");
            break;
        case 3:
            room_type_str = "COOP";
            create_multiplayer_room(rooms[room_count].max_players, rooms[room_count].name, COOP);
            log_event(config.log_file, "New Coop room created and first client connected");
            break;
        default:
            printf("Unknown room type\n");
            close(client_socket);
            return;
    }

    printf("Type: %s\n", room_type_str);

    char log_msg[255];
    snprintf(log_msg, sizeof(log_msg),
             "Client connected to room: %s, with ID: 1, Socket: %d",
             rooms[room_count].name, client_socket);
    log_event(config.log_file, log_msg);

    if (fork() == 0) {
        client_handler(&rooms[room_count], client_socket, 0);
    }

    room_count++;
}

void create_singleplayer_room(char *room_name) {
	room_config_t *room_config = malloc(sizeof(room_config_t));
	if (!room_config) {
		perror("Failed to allocate memory for room_config");
		exit(EXIT_FAILURE);
	}

	room_config->max_players = 1;
	room_config->room_name = strdup(room_name);
	if (!room_config->room_name) {
		perror("Failed to allocate memory for room_name");
		free(room_config);
		exit(EXIT_FAILURE);
	}

	pthread_t temp_thread;
	if (pthread_create(&temp_thread, NULL, singleplayer_room_handler, room_config) != 0) {
		perror("pthread_create failed");
		free(room_config->room_name);
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
	room_config->room_name = strdup(room_name);
	if (!room_config->room_name) {
		perror("Failed to allocate memory for room_name");
		free(room_config);
		exit(EXIT_FAILURE);
	}

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
			perror("Invalid room type");
		free(room_config->room_name);
		free(room_config);
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
int compare_timespecs(const struct timespec *a, const struct timespec *b) {
	if (a->tv_sec != b->tv_sec) {
		return (a->tv_sec < b->tv_sec) ? -1 : 1;
	}
	return (a->tv_nsec < b->tv_nsec) ? -1 : (a->tv_nsec > b->tv_nsec ? 1 : 0);
}


//ROOM FUNCTIONS
//RANKED
void setup_multiplayer_ranked_shared_memory(const char *room_name, multiplayer_ranked_room_shared_data_t **shared_data) {
    int room_shared_memory = shm_open(room_name, O_CREAT | O_RDWR, 0666);
    if (room_shared_memory == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(room_shared_memory, sizeof(multiplayer_ranked_room_shared_data_t)) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }

    *shared_data = mmap(NULL, sizeof(multiplayer_ranked_room_shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, room_shared_memory, 0);
    if (*shared_data == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    strncpy((*shared_data)->room_name, room_name, sizeof((*shared_data)->room_name));
    (*shared_data)->board_id = -1;
    (*shared_data)->starting_board[0] = '\0';

    for (int i = 0; i < 5; i++) {
        (*shared_data)->task_queue[i].client_socket = -1;
        (*shared_data)->task_queue[i].request[0] = '\0';
    }

    (*shared_data)->task_consumer_ptr = 0;
    (*shared_data)->task_productor_ptr = 0;
	(*shared_data)->current_player = config.server_size;
}
void multiplayer_ranked_select_new_board_and_share(multiplayer_ranked_room_shared_data_t *shared_data) {
    srand(time(NULL));
    shared_data->board_id = rand() % num_boards;
    const cJSON *round_board = cJSON_GetArrayItem(boards, shared_data->board_id);

    strncpy(shared_data->starting_board, cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")), sizeof(shared_data->starting_board));
}
void *multiplayer_ranked_room_handler(void *arg) {
    struct timespec media = {0, 0};
    int time_counter = 0;

    room_config_t *room_config = (room_config_t *)arg;
    char room_name[100];
    snprintf(room_name, sizeof(room_name), "%s", room_config->room_name);

    int max_player = room_config->max_players;
    struct timespec start, end;
    multiplayer_ranked_room_shared_data_t *shared_data;
    pthread_t solution_checker;

    setup_multiplayer_ranked_shared_memory(room_name, &shared_data);

    char temp[255];
    snprintf(temp, sizeof(temp), "/sem_%s_solucao", room_name);
    sem_unlink(temp);
    sem_t *sem_solution_found = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

    snprintf(temp, sizeof(temp), "/sem_%s_room_full", room_name);
    sem_unlink(temp);
    sem_t *sem_room_full = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

    snprintf(temp, sizeof(temp), "/sem_%s_game_start", room_name);
    sem_unlink(temp);
    sem_t *sem_game_start = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

    snprintf(temp, sizeof(temp), "/mut_%s_task_server", room_name);
    sem_unlink(temp);
    sem_open(temp, O_CREAT | O_RDWR, 0666, 1);

    snprintf(temp, sizeof(temp), "/mut_%s_task_client", room_name);
    sem_unlink(temp);
    sem_open(temp, O_CREAT | O_RDWR, 0666, 1);

    snprintf(temp, sizeof(temp), "/sem_%s_producer", room_name);
    sem_unlink(temp);
    sem_open(temp, O_CREAT | O_RDWR, 0666, 5);

    snprintf(temp, sizeof(temp), "/sem_%s_consumer", room_name);
    sem_unlink(temp);
    sem_open(temp, O_CREAT | O_RDWR, 0666, 0);

    pthread_create(&solution_checker, NULL, task_handler_multiplayer_ranked, shared_data);
    pthread_create(&solution_checker, NULL, task_handler_multiplayer_ranked, shared_data);
    pthread_create(&solution_checker, NULL, task_handler_multiplayer_ranked, shared_data);

    printf("%s of type Multiplayer Ranked - started with a max of %d players\n", room_name, max_player);

    wait_for_full_room(sem_room_full, max_player);
    printf("Multiplayer Ranked %s: room full - let the games begin\n", room_name);


    while (1) {
    	int current_players = shared_data->current_player;
    	if (current_players == 0) {
    		printf("SERVER IS EMPTY \n");
			return 0;
    	}
        multiplayer_ranked_select_new_board_and_share(shared_data);

        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < current_players; i++) {
            sem_post(sem_game_start);
        }

        printf("Multiplayer Ranked %s: game start has been signaled\n", room_name);

        for (int i = 0; i < current_players; i++) {
            sem_wait(sem_solution_found);
            clock_gettime(CLOCK_MONOTONIC, &end);

            struct timespec round_time = {
                .tv_sec = end.tv_sec - start.tv_sec,
                .tv_nsec = end.tv_nsec - start.tv_nsec
            };

            time_counter++;
            double new_avg = ((media.tv_sec + media.tv_nsec / 1e9) * (time_counter - 1) +
                             (round_time.tv_sec + round_time.tv_nsec / 1e9)) / time_counter;
            media.tv_sec = (time_t)new_avg;
            media.tv_nsec = (long)((new_avg - media.tv_sec) * 1e9);

            printf("New time for %s: %.10f\n", room_name, round_time.tv_sec + round_time.tv_nsec / 1e9);
        }

        printf("Average time for %s: %.10f\n", room_name, media.tv_sec + media.tv_nsec / 1e9);
    }
}
void *task_handler_multiplayer_ranked(void *arg) {
    multiplayer_ranked_room_shared_data_t *shared_data = (multiplayer_ranked_room_shared_data_t *)arg;

    char temp[255];
    snprintf(temp, sizeof(temp), "/sem_%s_producer", shared_data->room_name);
    sem_t *sem_prod = sem_open(temp, 0);

    snprintf(temp, sizeof(temp), "/sem_%s_consumer", shared_data->room_name);
    sem_t *sem_cons = sem_open(temp, 0);

    snprintf(temp, sizeof(temp), "/mut_%s_task_server", shared_data->room_name);
    sem_t *mutex_task = sem_open(temp, 0);

    while (1) {
        sem_wait(sem_cons);
        sem_wait(mutex_task);

        Task task = shared_data->task_queue[shared_data->task_consumer_ptr];
        shared_data->task_consumer_ptr = (shared_data->task_consumer_ptr + 1) % 5;

        sem_post(mutex_task);
        sem_post(sem_prod);

        int **solution = getMatrixFromJSON(
            cJSON_GetObjectItem(cJSON_GetArrayItem(boards, shared_data->board_id), "solution"));

        int row = task.request[2] - '0';
        int col = task.request[4] - '0';
        int value = task.request[6] - '0';

        if (solution[row][col] != value) {
            send(task.client_socket, "2", sizeof("2"), 0);
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


	(*shared_data)->counter = 0;


	for (int i = 0; i < config.server_size; i++) {
		(*shared_data)->task_queue[i].client_socket = -1;
		sprintf((*shared_data)->task_queue[i].request, "\0");
		sem_init(&(*shared_data)->sems_client[i], true, 1);
		(*shared_data)->has_solution[i] = false;
		(*shared_data)->still_alive[i] = true;
		sem_init(&(*shared_data)->sems_server[i], true, 0);
	}
	(*shared_data)->current_player = config.server_size;
	//TODO LOGS
}
void multiplayer_casual_select_new_board_and_share(multiplayer_casual_room_shared_data_t *shared_data) {
	srand(time(NULL));
	shared_data->board_id = rand() % num_boards;
	int random_board = shared_data->board_id;

	const cJSON *round_board = cJSON_GetArrayItem(boards, random_board);
	//broadcast new board
	strcpy(shared_data->starting_board, cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));
	//TODO LOGS
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
		int current_players = shared_data->current_player;
		if (current_players == 0) {
			printf("SERVER IS EMPTY \n");
			return 0;
		}
		multiplayer_casual_select_new_board_and_share(shared_data);

		//Start round
		clock_gettime(CLOCK_MONOTONIC, &start);
		for (int i = 0; i < current_players; i++) {
			sem_post(sem_game_start);
		}
		separator();
		printf("Multiplayer Casual %s: game start has been signaled \n", room_name);

		for (int i = 0; i < current_players; i++) {
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
		}
		printf("Media de %s: %.10f\n", room_name, media.tv_sec + media.tv_nsec / 1e9);
	}
	//TODO LOGS
}
void *task_handler_multiplayer_casual(void *arg) {
	multiplayer_casual_room_shared_data_t *shared_data = arg;

	int current_index = 0;
	//TODO LOGS
	// ReSharper disable once CppDFAEndlessLoop
	while (1) {
		//PREPROTOCOLO

		if (shared_data->has_solution[current_index] || !shared_data->still_alive[current_index]) {
			current_index = (current_index + 1) % config.server_size;
			continue;
		}

		sem_wait(&shared_data->sems_server[current_index]);
		//ZONA CRITICA --- ler task

		//usleep(rand() % 1);
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
		//sleep(0);
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
	(*shared_data)->current_player = config.server_size;
}
void multiplayer_coop_select_new_board_and_share(multiplayer_coop_room_shared_data_t *shared_data) {
	srand(time(NULL));
	shared_data->board_id = rand() % num_boards;
	int random_board = shared_data->board_id;

	const cJSON *round_board = cJSON_GetArrayItem(boards, random_board);
	//broadcast new board
	strcpy(shared_data->current_board, cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));
	//TODO LOGS
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

	//TODO LOGS


	printf("%s of type Multiplayer COOP - started with a max of %d players\n", room_name, max_player);

	wait_for_full_room(sem_room_full, max_player); // Espera que o room encha
	printf("Multiplayer COOP %s: room full - set the games begin\n", room_name);

	pthread_create(&soltution_checker, NULL, task_handler_multiplayer_coop, shared_data);
	for (;;) {
		int current_players = shared_data->current_player;
		if (current_players == 0) {
			printf("SERVER IS EMPTY \n");
			return 0;
		}
		multiplayer_coop_select_new_board_and_share(shared_data);

		//Start round
		clock_gettime(CLOCK_MONOTONIC, &start);
		for (int i = 0; i < current_players; i++) {
			sem_post(sem_game_start);
		}
		separator();
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
		//sleep(5);
	}
	printf("Media de %s: %.10f\n", room_name, media.tv_sec + media.tv_nsec / 1e9);
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
	//TODO LOGS

	while (1) {
		selected_client = -1;
		oldest_request_time.tv_sec = LONG_MAX;
		oldest_request_time.tv_nsec = 999999999;

		//PRE

		for (int i =0; i < config.server_size; i++) {
			int temp;
			sem_getvalue(&shared_data->sems_server[i], &temp);
			printf("client index:%d hasRequest: %d last requested: %d\n", i, temp, last_processed[i]);
		}


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
		//usleep(rand() % 1);
		//sleep(1.5);
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
	//TODO LOGS
}
void *task_handler_singleplayer(void *arg) {
	singleplayer_room_shared_data_t *shared_data = arg;

	char temp[255];
	sprintf(temp, "sem_%s_server", shared_data->room_name);
	sem_t *sem_server = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);
	sprintf(temp, "sem_%s_client", shared_data->room_name);
	sem_t *sem_client = sem_open(temp, O_CREAT | O_RDWR, 0666, 1);
	//TODO LOGS

	while (true) {
		sem_wait(sem_server);
		//ZONA CRITICA
		//usleep(rand() % 1); //TODO SLEEP FROM CONFIG
		//VER SE TA CERTO e manda para o buffer se está certo ou não
		int **solution = getMatrixFromJSON( cJSON_GetObjectItem(cJSON_GetArrayItem(boards, shared_data->board_id), "solution"));
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
	//TODO LOGS
	while (true) {
		//sleep(5);
		shared_data->board_id = rand() % num_boards;
		int rand_board = shared_data->board_id;

		cJSON *round_board = cJSON_GetArrayItem(boards, rand_board);
		strcpy(shared_data->starting_board, cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));

		//Start Round
		clock_gettime(CLOCK_MONOTONIC, &start);
		sem_post(sem_game_start);
		separator();
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


//CLIENT HANDLER

void send_solution_attempt_multiplayer_ranked(int x, int y, int novo_valor, sem_t *sem_sync_2, sem_t *mutex_task,
											multiplayer_ranked_room_shared_data_t *multiplayer_ranked_shared_data,
											int client_socket, sem_t *sem_sync_1) {
	char message[255];
	sprintf(message, "0-%d,%d,%d", x, y, novo_valor);
	//printf("%s\n", message);
	//PREPROTOCOLO
	sem_wait(sem_sync_2); //produtores
	sem_wait(mutex_task);

	//ZONA CRITICA PARA CRIAR TASK
	multiplayer_ranked_shared_data->task_queue[multiplayer_ranked_shared_data->task_productor_ptr].client_socket =
			client_socket;
	sprintf(multiplayer_ranked_shared_data->task_queue[multiplayer_ranked_shared_data->task_productor_ptr].request,
			message);
	multiplayer_ranked_shared_data->task_productor_ptr = (multiplayer_ranked_shared_data->task_productor_ptr + 1) % 5;
	//printf("Pedido colocado na fila\n");

	//usleep(rand() % (config.slow_factor + 0));
	//POS PROTOCOLO
	sem_post(mutex_task);
	sem_post(sem_sync_1);
}

void send_solution_attempt_multiplayer_casual(int x, int y, int novo_valor,
											multiplayer_casual_room_shared_data_t *multiplayer_casual_room_shared_data,
											int client_index) {
	char message[255];
	if (x == -1) {
		sprintf(message, "1--1,-1,-1", y, novo_valor);
	} else sprintf(message, "0-%d,%d,%d", x, y, novo_valor);
	//PREPROTOCOLO
	sem_wait(&multiplayer_casual_room_shared_data->sems_client[client_index]);

	//ZC
	sprintf(multiplayer_casual_room_shared_data->task_queue[client_index].request, message);
	//printf("Pedido colocado no array\n");

	//usleep(rand() % (config.slow_factor + 0));

	//POSPROTOCOLO
	sem_post(&multiplayer_casual_room_shared_data->sems_server[client_index]);
}

void send_solution_attempt_multiplayer_coop(multiplayer_coop_room_shared_data_t *multiplayer_coop_room_shared_data,
											int client_index) {

	// PREPROTOCOLO
	sem_wait(&multiplayer_coop_room_shared_data->sems_client[client_index]);
	// ZC
	//usleep(rand() % (config.slow_factor*2 + 0));

	cJSON *json_board = cJSON_Parse(multiplayer_coop_room_shared_data->current_board);
	if (json_board == NULL) {
		printf("Error parsing JSON\n");
		return;
	}
	//TODO FIX
	//clear();

	int x, y, value;
	int **old_board = getMatrixFromJSON(json_board);

	//printBoard(old_board);

	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			if (old_board[i][j] == 0) {
				//printf("celula (%d,%d) está vazia\n", i, j);
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

	//usleep(rand() % (config.slow_factor + 0));
}

void send_solution_attempt_single_player(int x, int y, int novo_valor, sem_t *sem_sync_2,
										singleplayer_room_shared_data_t *singleplayer_room_shared_data,
										sem_t *sem_sync_1) {
	char message[255];
	sprintf(message, "0-%d,%d,%d", x, y, novo_valor);
	//PREPROTOCOLO

	sem_wait(sem_sync_2); // redundante??
	//ZC
	//usleep(rand() % (config.slow_factor + 1));
	//printf("%s\n", message);
	strcpy(singleplayer_room_shared_data->buffer, message);

	//POSPROTOCOLO
	sem_post(sem_sync_1); // passa para o server
}

bool receice_answer_single_player(sem_t *sem_sync_2, singleplayer_room_shared_data_t *singleplayer_room_shared_data) {


	sem_wait(sem_sync_2); //espera pela resposta do server
	//usleep(rand() % (config.slow_factor + 1));
	bool answer = atoi(singleplayer_room_shared_data->buffer);
	sem_post(sem_sync_2); //vai tratar das continhas (vai escrever outra vez) REDUNDANTE??

	return answer;
};
bool receive_answer_multiplayer_casual(multiplayer_casual_room_shared_data_t *multiplayer_casual_room_shared_data,int client_index) {
	char response[1024];
	//PRE
	sem_wait(&multiplayer_casual_room_shared_data->sems_client[client_index]);
	//ZC
	sprintf(response, multiplayer_casual_room_shared_data->task_queue[client_index].request);

	//POS
	sem_post(&multiplayer_casual_room_shared_data->sems_client[client_index]);
	return response[0] == '0' ? false : true;
}

void *client_handler(room_t *room, int client_socket, int client_index) {
	signal(SIGINT, NULL);
	signal(SIGTERM, NULL);


	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%d-%d-%s", client_socket, client_index, room->name);
	send(client_socket, buffer, sizeof(buffer), 0);
	multiplayer_ranked_room_shared_data_t *multiplayer_ranked_shared_data;
	multiplayer_casual_room_shared_data_t *multiplayer_casual_room_shared_data;
	multiplayer_coop_room_shared_data_t *multiplayer_coop_room_shared_data;
	singleplayer_room_shared_data_t *singleplayer_room_shared_data;

	sem_t *sem_solucao;
	sem_t *sem_room_full;
	sem_t *sem_game_start;

	sem_t *mutex_task;
	sem_t *sem_sync_1;
	sem_t *sem_sync_2;

	sleep(2);

	if (room->type == 0) {
		char temp[255];

		sprintf(temp, "/sem_%s_solucao", room->name);
		sem_solucao = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_game_start", room->name);
		sem_game_start = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_server", room->name);
		sem_sync_1 = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_client", room->name);
		sem_sync_2 = sem_open(temp, 0);

		printf("Abrir memoria partilhada da sala\n");
		const int room_shared_memory_fd = shm_open(room->name, O_CREAT | O_RDWR, 0666);
		if (room_shared_memory_fd == -1) {
			perror("shm_open fail");
			exit(EXIT_FAILURE);
		}

		singleplayer_room_shared_data = mmap(NULL, sizeof(singleplayer_room_shared_data_t), PROT_READ | PROT_WRITE,
											MAP_SHARED, room_shared_memory_fd, 0);
		if (singleplayer_room_shared_data == MAP_FAILED) {
			perror("mmap fail");
			exit(EXIT_FAILURE);
		}
		printf("PRONTO PARA JOGAR!\nAssinalando a Sala que esta a espera\n");
	}
	else if (room->type == 1) {
		char temp[255];
		sprintf(temp, "/sem_%s_producer", room->name);
		sem_sync_2 = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_consumer", room->name);
		sem_sync_1 = sem_open(temp, 0);

		sprintf(temp, "/sem_%s_solucao", room->name);
		sem_solucao = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_room_full", room->name);
		sem_room_full = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_game_start", room->name);
		sem_game_start = sem_open(temp, 0);
		sprintf(temp, "/mut_%s_task_client", room->name);
		mutex_task = sem_open(temp, 0);


		printf("Abrir memoria partilhada da sala\n");
		const int room_shared_memory_fd = shm_open(room->name, O_RDWR, 0666);
		if (room_shared_memory_fd == -1) {
			perror("shm_open FAIL");
			exit(EXIT_FAILURE);
		}

		multiplayer_ranked_shared_data = mmap(NULL, sizeof(multiplayer_ranked_room_shared_data_t),
											PROT_READ | PROT_WRITE, MAP_SHARED, room_shared_memory_fd, 0);
		if (multiplayer_ranked_shared_data == MAP_FAILED) {
			perror("mmap FAIL");
			exit(EXIT_FAILURE);
		}
		printf("PRONTO PARA JOGAR!\nAssinalando a Sala que esta a espera\n");
		sem_post(sem_room_full); // assinala que tem mais um cliente no room
	}
	else if (room->type == 2) {
		printf("SHM Multiplayer casual\n");
		char temp[255];
		sprintf(temp, "/sem_%s_solucao", room->name);
		sem_solucao = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_room_full", room->name);
		sem_room_full = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_game_start", room->name);
		sem_game_start = sem_open(temp, 0);

		printf("Abrir memoria partilhada da sala\n");
		const int room_shared_memory_fd = shm_open(room->name, O_RDWR, 0666);
		if (room_shared_memory_fd == -1) {
			perror("shm_open FAIL");
			printf(room->name);
			exit(EXIT_FAILURE);
		}
		multiplayer_casual_room_shared_data = mmap(NULL, sizeof(multiplayer_casual_room_shared_data_t),
													PROT_READ | PROT_WRITE, MAP_SHARED, room_shared_memory_fd, 0);

		if (multiplayer_casual_room_shared_data == MAP_FAILED) {
			printf("mmap FAIL");
			exit(EXIT_FAILURE);
		}
		printf("PRONTO PARA JOGAR!\nAssinalando a Salla que esta a espera\n");
		sem_post(sem_room_full);
	}
	else if (room->type == 3) {
		printf("SHM Multiplayer coop ");

		char temp[255];
		sprintf(temp, "/sem_%s_solucao", room->name);
		sem_solucao = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_room_full", room->name);
		sem_room_full = sem_open(temp, 0);
		sprintf(temp, "/sem_%s_game_start", room->name);
		sem_game_start = sem_open(temp, 0);

		printf("Abrir memoria partilhada da sala\n");
		const int room_shared_memory_fd = shm_open(room->name, O_RDWR, 0666);
		if (room_shared_memory_fd == -1) {
			perror("shm_open FAIL");
			printf(room->name);
			exit(EXIT_FAILURE);
		}
		multiplayer_coop_room_shared_data = mmap(NULL, sizeof(multiplayer_casual_room_shared_data_t),
												PROT_READ | PROT_WRITE, MAP_SHARED, room_shared_memory_fd, 0);

		if (multiplayer_coop_room_shared_data == MAP_FAILED) {
			printf("mmap FAIL");
			exit(EXIT_FAILURE);
		}
		printf("PRONTO PARA JOGAR!\nAssinalando a Sala que esta a espera\n");
		sem_post(sem_room_full);
	}


	//TODO FIX UI
	separator();
	printf("ESPERANDO\n");

	while (true) {

		new_round:
		sem_wait(sem_game_start); // espera que o jogo comece
		int **board;
		bool is_first_attempt = true;

		switch (room->type) {
			case 0:
				board = getMatrixFromJSON(cJSON_Parse(singleplayer_room_shared_data->starting_board));
				break;
			case 1:
				board = getMatrixFromJSON(cJSON_Parse(multiplayer_ranked_shared_data->starting_board));
				break;
			case 2:
				board = getMatrixFromJSON(cJSON_Parse(multiplayer_casual_room_shared_data->starting_board));
				break;
			case 3:
				board = getMatrixFromJSON(cJSON_Parse(multiplayer_coop_room_shared_data->current_board));
			break;
		}
		if (room->type == 2) {
			multiplayer_casual_room_shared_data->has_solution[client_index] = false;
		}

		sprintf(buffer, "0-%s", cJSON_Print(convertMatrixToJSON(board)));
		send(client_socket, buffer, sizeof(buffer), 0); //UPDATE BOARD
		send(client_socket, "3", sizeof("3"), 0); // START GAME

		while (true) {
			ssize_t recv_ret = recv(client_socket, buffer, sizeof(buffer), 0);
			if (recv_ret == 0) {
				//TODO REMOVE CLIENT FROM SERVER AND SHIT
				printf("999\n");
				switch (room->type) {
					case 0:
						//TODO
							break;
					case 1:
						multiplayer_ranked_shared_data->current_player--;
						sem_post(sem_solucao);
					break;
					case 2:
						multiplayer_casual_room_shared_data->current_player--;
						multiplayer_casual_room_shared_data->starting_board[client_index] = false;
						multiplayer_casual_room_shared_data->has_solution[client_index] = true;
						sem_post(&multiplayer_casual_room_shared_data->sems_server[client_index]);
						sem_post(sem_solucao);
					break;
					case 3:
						multiplayer_coop_room_shared_data->current_player--;
						if (multiplayer_coop_room_shared_data->current_player == 0) {
							sem_post(sem_solucao);
						}
					break;
				}
				exit(0);
			}
			char *message = buffer;
			message += 2;
			int i;
			int j;
			int k;

			switch (buffer[0]) {
				case '0':
					//Solution Request
						if (room->type == 3) {
							send_solution_attempt_multiplayer_coop(multiplayer_coop_room_shared_data, client_index);
							board = getMatrixFromJSON(cJSON_Parse(multiplayer_coop_room_shared_data->current_board));
							sprintf(buffer, "0-%s", cJSON_Print(convertMatrixToJSON(board)));
							send(client_socket, buffer, sizeof(buffer), 0); //UPDATE BOARD
							send(client_socket, "3", sizeof("3"), 0);
							break;
						}
					sscanf(message, "%d-%d-%d",&i,&j,&k);
					if (k >= 10) k /= 10;

					switch (room->type) {
						case 0:
							send_solution_attempt_single_player(i, j, k, sem_sync_2, singleplayer_room_shared_data, sem_sync_1);
							if (receice_answer_single_player(sem_sync_2, singleplayer_room_shared_data)) {
								send(client_socket, "2", sizeof("2"), 0);
							}
							else {
								send(client_socket, "1", sizeof("1"), 0);
							}
							break;
						case 1:
							send_solution_attempt_multiplayer_ranked(i,j,k,sem_sync_2,mutex_task,multiplayer_ranked_shared_data,client_socket,sem_sync_1);
							break;
						case 2:
							send_solution_attempt_multiplayer_casual(i,j,k,multiplayer_casual_room_shared_data,client_index);
							//sleep(1);
							if (receive_answer_multiplayer_casual(multiplayer_casual_room_shared_data, client_index)) {
								send(client_socket, "2", sizeof("2"), 0);
							}
							else {
								send(client_socket, "1", sizeof("1"), 0);
							}
							break;
					}
					break;
				case '1':
					//has solution already
					if (room->type == 2) {
						int temp;
						sem_getvalue(&multiplayer_casual_room_shared_data->sems_server[client_index], &temp);

						if (temp == 0) {
							sem_post(&multiplayer_casual_room_shared_data->sems_server[client_index]);
						}
						multiplayer_casual_room_shared_data->has_solution[client_index] = true;
					}
						sem_post(sem_solucao);

						goto new_round;
				case '2':
					break;
				case '3':
					break;
				case '4':
					break;
				case '5':
					break;
				case '6':
					break;
				case '7':
					break;
				case '9':
				 break;
			}
		}
	}
}