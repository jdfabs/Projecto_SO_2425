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
#include <signal.h>

#define MAX_PENDING_CONNECTIONS 5
#define MAX_ROOMS 20
#define BUFFER_SIZE 100
#define MAX_PLAYERS_DEFAULT 1

typedef struct {
    int max_players;
    char *room_name;
} room_config_t;

server_config config;
cJSON *boards;
int num_boards;

int server_fd;
struct sockaddr_in address;
pthread_t multiplayer_rooms[10];

typedef struct {
    char name[255];
    int type;
    int max_players;
    int current_players;
    pthread_t thread_id;
} room_t;

room_t rooms[MAX_ROOMS];
int room_count = 0;

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
void server_init(int argc, char **argv);
void setup_socket();
void accept_clients();
void graceful_shutdown(int signum);
void *task_handler(void *arg);
void create_new_room(int max_players, char *room_name);
void *room_handler(void *arg);
void setup_shared_memory(char room_name[100], multiplayer_room_shared_data_t **shared_data);
void room_setup(void *arg, char (*room_name)[100], int *max_player, struct timespec *start, struct timespec *end, multiplayer_room_shared_data_t **shared_data, sem_t **sem_solution_found, pthread_t *solution_checker);
void wait_for_full_room(sem_t *sem, int slots);
void select_new_board_and_share(multiplayer_room_shared_data_t *shared_data);
void shared_memory_clean_up(char room_name[100], multiplayer_room_shared_data_t *shared_data);

int main(int argc, char *argv[]) {
    // Initialize server data structures and configurations
    server_init(argc, argv);
    // Set up server socket ready to accept connections
    setup_socket();

    while (1) {
        accept_clients(); // Accept connections and perform handshake
    }

    close(server_fd);
    return EXIT_SUCCESS;
}

// BASIC SERVER AUX FUNCTIONS
void graceful_shutdown(int signum) {
    log_event(config.log_file, "Server shutting down gracefully");
    close(server_fd);
    // Add more cleanup code if necessary
    exit(EXIT_SUCCESS);
}

void server_init(int argc, char **argv) {
    signal(SIGINT, graceful_shutdown);
    signal(SIGTERM, graceful_shutdown);

    if (argc != 2) {
        fprintf(stderr, "Usage: ./server [CONFIG_FILE_NAME]\n");
        log_event("./logs/server_default.json", "Server started without configuration arguments");
        exit(EXIT_FAILURE);
    }

    printf("Starting server...\n");
    if (load_server_config(argv[1], &config) < 0) {
        fprintf(stderr, "Failed to load server configuration.\n");
        log_event("./logs/server_default.json", "Failed to load server configuration - Shutting down server");
        exit(EXIT_FAILURE);
    }
    log_event(config.log_file, "Server started successfully");

    boards = load_boards(config.board_file_path);
    if (boards == NULL) {
        fprintf(stderr, "Failed to load boards from %s\n", config.board_file_path);
        log_event(config.log_file, "Failed to load boards - Shutting down server");
        exit(EXIT_FAILURE);
    }

    // Count the number of boards loaded
    cJSON *child = boards->child;
    while (child != NULL) {
        num_boards++;
        child = child->next;
    }

    log_event(config.log_file, "Boards successfully loaded into memory");
}

void setup_socket() {
    log_event(config.log_file, "Starting socket setup");

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        log_event(config.log_file, "Socket creation failed");
        exit(EXIT_FAILURE);
    }

    char temp[255];
    snprintf(temp, sizeof(temp), "Socket created successfully. socket_fd: %d", server_fd);
    log_event(config.log_file, temp);

    // Configure server address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
    address.sin_port = htons(config.port); // Server port

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        log_event(config.log_file, "setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Bind to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        log_event(config.log_file, "Socket bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    log_event(config.log_file, "Socket bound successfully");

    // Listen for incoming connection requests
    if (listen(server_fd, MAX_PENDING_CONNECTIONS) < 0) {
        perror("Listen failed");
        log_event(config.log_file, "Socket listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    log_event(config.log_file, "Server is now listening for incoming connections");
}

void accept_clients() {
    socklen_t addr_len = sizeof(address);
    int client_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len);
    if (client_socket < 0) {
        perror("Accept failed");
        return;
    }

    // HANDSHAKE
    char aux[BUFFER_SIZE] = {0};
    if (recv(client_socket, aux, sizeof(aux) - 1, 0) < 0) {
        perror("Receive failed");
        close(client_socket);
        return;
    }
    printf("Received: %s\n", aux);

    for (int i = 0; i < room_count; i++) {
        if (rooms[i].current_players < rooms[i].max_players) {
            printf("ROOM WITH SPACE\n");
            snprintf(aux, sizeof(aux), "%d-%s%d", client_socket, "room_", i);
            rooms[i].current_players++;
            send(client_socket, aux, strlen(aux), 0);
            close(client_socket);
            return;
        }
    }

    printf("NO APPROPRIATE ROOMS - CREATING NEW ROOM\n");
    snprintf(rooms[room_count].name, sizeof(rooms[room_count].name), "room_%d", room_count);
    rooms[room_count].current_players = 1;
    rooms[room_count].max_players = MAX_PLAYERS_DEFAULT;
    rooms[room_count].type = atoi(aux);

    create_new_room(rooms[room_count].max_players, rooms[room_count].name);

    snprintf(aux, sizeof(aux), "%d-%s", client_socket, rooms[room_count].name);
    room_count++;

    send(client_socket, aux, strlen(aux), 0);
    close(client_socket);
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
    strncpy(room_config->room_name, room_name, strlen(room_name) + 1);

    pthread_t temp_thread;
    int ret = pthread_create(&temp_thread, NULL, room_handler, (void *)room_config);
    if (ret != 0) {
        perror("pthread_create failed");
        free(room_config->room_name); // Clean up allocated memory
        free(room_config);
        exit(EXIT_FAILURE);
    }

    log_event(config.log_file, "New room created successfully");
}

// TASK MANAGERS
void *task_handler(void *arg) {
    // Cast the void pointer to the appropriate type
    multiplayer_room_shared_data_t *shared_data = (multiplayer_room_shared_data_t *)arg;

    char aux[100];
    snprintf(aux, sizeof(aux), "/sem_%s_producer", shared_data->room_name);
    sem_unlink(aux);
    sem_t *sem_prod = sem_open(aux, O_CREAT | O_RDWR, 0666, 5);
    if (sem_prod == SEM_FAILED) {
        perror("Failed to create producer semaphore");
        pthread_exit(NULL);
    }

    snprintf(aux, sizeof(aux), "/sem_%s_consumer", shared_data->room_name);
    sem_unlink(aux);
    sem_t *sem_cons = sem_open(aux, O_CREAT | O_RDWR, 0666, 0);
    if (sem_cons == SEM_FAILED) {
        perror("Failed to create consumer semaphore");
        sem_close(sem_prod);
        pthread_exit(NULL);
    }

    int **solution;

    // PRODUCER-CONSUMER PROBLEM --- CONSUMER
    while (1) {
        printf("TASK_HANDLER -- Waiting for something to read\n");

        // PRE-PROTOCOL
        sem_wait(sem_cons);
        pthread_mutex_lock(&shared_data->mutex_task_reader);

        // CRITICAL SECTION --- read task
        Task task = shared_data->task_queue[shared_data->task_consumer_ptr];
        shared_data->task_consumer_ptr = (shared_data->task_consumer_ptr + 1) % 5;

        // POST-PROTOCOL
        pthread_mutex_unlock(&shared_data->mutex_task_reader);
        sem_post(sem_prod);

        printf("%s\n", task.request);

        solution = getMatrixFromJSON(cJSON_GetObjectItem(cJSON_GetArrayItem(boards, shared_data->board_id), "solution"));

        int i = task.request[2] - '0';
        int j = task.request[4] - '0';
        int k = task.request[6] - '0';

        if (solution[i][j] != k) {
            printf("nope\n");
            send(task.client_socket, "0", sizeof("0"), 0);
        } else {
            printf("yep\n");
            send(task.client_socket, "1", sizeof("1"), 0);
        }
    }

    // Cleanup semaphores before exit
    sem_close(sem_prod);
    sem_close(sem_cons);
    pthread_exit(NULL);
}

// ROOM FUNCTIONS
void setup_shared_memory(char room_name[100], multiplayer_room_shared_data_t **shared_data) {
    int room_shared_memory = shm_open(room_name, O_CREAT | O_RDWR, 0666);
    if (room_shared_memory == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(room_shared_memory, sizeof(multiplayer_room_shared_data_t)) == -1) {
        perror("ftruncate failed");
        close(room_shared_memory);
        exit(EXIT_FAILURE);
    }

    *shared_data = mmap(NULL, sizeof(multiplayer_room_shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, room_shared_memory, 0);
    if (*shared_data == MAP_FAILED) {
        perror("mmap failed");
        close(room_shared_memory);
        exit(EXIT_FAILURE);
    }

    snprintf((*shared_data)->room_name, sizeof((*shared_data)->room_name), "%s", room_name);
    (*shared_data)->board_id = -1;
    strcpy((*shared_data)->starting_board, "");

    sem_init(&(*shared_data)->sem_game_start, 1, 0);
    sem_init(&(*shared_data)->sem_room_full, 1, 0);
    sem_init(&(*shared_data)->sem_found_solution, 1, 0);

    for (int i = 0; i < 5; i++) { // Initialize the task queue
        (*shared_data)->task_queue[i].client_socket = -1;
        snprintf((*shared_data)->task_queue[i].request, sizeof((*shared_data)->task_queue[i].request), "");
    }

    (*shared_data)->task_consumer_ptr = 0;
    (*shared_data)->task_productor_ptr = 0;
    pthread_mutex_init(&(*shared_data)->mutex_task_creators, NULL);
    pthread_mutex_init(&(*shared_data)->mutex_task_reader, NULL);
}

void room_setup(void *arg, char (*room_name)[100], int *max_player, struct timespec *start, struct timespec *end, multiplayer_room_shared_data_t **shared_data, sem_t **sem_solution_found, pthread_t *solution_checker) {
    room_config_t *room_config = (room_config_t *)arg;
    snprintf(*room_name, sizeof(*room_name), "%s", room_config->room_name);
    *max_player = room_config->max_players;

    setup_shared_memory(*room_name, shared_data);
    char temp[255];
    snprintf(temp, sizeof(temp), "/sem_%s_solution", *room_name);
    *sem_solution_found = sem_open(temp, O_CREAT | O_RDWR, 0666, 0);
    if (*sem_solution_found == SEM_FAILED) {
        perror("Failed to create solution semaphore");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(solution_checker, NULL, task_handler, (void *)*shared_data) != 0) {
        perror("Failed to create solution checker thread");
        sem_close(*sem_solution_found);
        exit(EXIT_FAILURE);
    }

    printf("MULTIPLAYER ROOM %s started with a max of %d players\n", *room_name, *max_player);
}

void wait_for_full_room(sem_t *sem, int slots) {
    int value;
    sem_getvalue(sem, &value);
    if (value <= slots) {
        for (int i = 0; i < slots; i++) {
            sem_wait(sem);
        }
    }
}

void select_new_board_and_share(multiplayer_room_shared_data_t *shared_data) {
    cJSON *round_board;

    srand(time(NULL));
    shared_data->board_id = rand() % num_boards;
    int random_board = shared_data->board_id;

    round_board = cJSON_GetArrayItem(boards, random_board);
    // Broadcast new board
    strcpy(shared_data->starting_board, cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));
}

void shared_memory_clean_up(char room_name[100], multiplayer_room_shared_data_t *shared_data) {
    sem_destroy(&shared_data->sem_game_start);
    sem_destroy(&shared_data->sem_room_full);
    sem_destroy(&shared_data->sem_found_solution);
    pthread_mutex_destroy(&shared_data->mutex_task_creators);
    pthread_mutex_destroy(&shared_data->mutex_task_reader);
    munmap(shared_data, sizeof(multiplayer_room_shared_data_t));
    shm_unlink(room_name);
}

void *room_handler(void *arg) {
    char room_name[100];
    int max_player;
    struct timespec start;
    struct timespec end;
    multiplayer_room_shared_data_t *shared_data;
    sem_t *sem_solution_found;
    pthread_t solution_checker;

    room_setup(arg, &room_name, &max_player, &start, &end, &shared_data, &sem_solution_found, &solution_checker);

    wait_for_full_room(&shared_data->sem_room_full, max_player); // Wait for the room to be full
    printf("ROOM_HANDLER %s: Full room - games are starting\n", room_name);

    for (;;) {
        printf("ROOM_HANDLER %s: NEW GAME!!\n", room_name);
        sleep(5);
        select_new_board_and_share(shared_data);

        // Start round
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < max_player; i++) {
            sem_post(&shared_data->sem_game_start);
        }
        printf("Multiplayer Room Handler: game start has been signaled\n");

        for (int i = 0; i < max_player; i++) {
            sem_wait(sem_solution_found);
            clock_gettime(CLOCK_MONOTONIC, &end);
            printf("%.2f\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9);
        }
    }

    shared_memory_clean_up(room_name, shared_data);
    pthread_exit(NULL);
}
