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
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>

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
int num_boards;

int server_fd ;
struct sockaddr_un address;


/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

void server_init(int argc, char **argv);
void setup_socket();
void *client_handler(void *arg);
void *task_handler ();
void *multiplayer_room_handler(void *arg);

/************************************
 * STATIC FUNCTIONS
 ************************************/


int main(int argc, char *argv[]) {
	pthread_t multiplayer_rooms[10];

	server_init(argc, argv); // Server data structures setup
	setup_socket(); // Ready to accept connections

	//TODO -- SETUP MORE ROOMS
	//Setup multiplayer rooms
	pthread_create(&multiplayer_rooms[0], NULL, multiplayer_room_handler, NULL);

	while (true) {
		int client_socket;
		// Accept a new client connection
		socklen_t addr_len = sizeof(address);
		client_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len);
		if (client_socket  < 0) {
			perror("Accept failed");
			// handle error
		}
		char aux[100];
		sprintf(aux, "%d", client_socket);
		send(client_socket, aux, sizeof(aux), 0);
	} //Aceitar connectões e criar threads para cada cliente

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


//TASK MANAGERS
void *task_handler(void *arg) {
	// Cast the void pointer to the appropriate type
	multiplayer_room_shared_data_t *shared_data = (multiplayer_room_shared_data_t *)arg;

	sem_t *sem_prod = sem_open("/sem_room_one_producer", O_CREAT | O_RDWR, 0666,5);
	sem_t *sem_cons = sem_open("/sem_room_one_consumer", O_CREAT | O_RDWR, 0666, 0);


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

	sem_init(&(*shared_data)->sem_game_start, 1, 0);
	sem_init(&(*shared_data)->sem_room_full, 1, 0);
	sem_init(&(*shared_data)->sem_found_solution, 1,0);

	(*shared_data)->board_id = 0;
	strcpy((*shared_data)->starting_board, ""); //TODO

	for (int i = 0; i < 5; i++) { //inicializar os valores do array
		(*shared_data)->task_queue[i].client_socket = -1;
		sprintf((*shared_data)->task_queue[i].request, "\0");
	}
	(*shared_data)->task_consumer_ptr = 0;
	(*shared_data)->task_productor_ptr = 0;
	pthread_mutex_init(&(*shared_data)->mutex_task_creators, NULL);
	pthread_mutex_init(&(*shared_data)->mutex_task_reader, NULL);
}
void shared_memory_clean_up(char room_name[100], multiplayer_room_shared_data_t *shared_data) { //TODO -- MISSING PROD_CONS VARS
	sem_destroy(&shared_data->sem_game_start);
	sem_destroy(&shared_data->sem_room_full);
	munmap(shared_data, sizeof(multiplayer_room_shared_data_t));
	shm_unlink(room_name);
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

void *multiplayer_room_handler(void *arg) {
	char room_name[100] = "/room_one";
	int max_player = 5;
	printf("MULTIPLAYER ROOM %s started with a max of %d players\n", room_name, max_player);
	struct timespec start,end;

	multiplayer_room_shared_data_t *shared_data;
	sem_t *sem_solucao_encontrada = sem_open("sem_room_one_solucao", O_CREAT | O_RDWR, 0666, 0);
	setup_shared_memory(room_name, &shared_data);

	pthread_t soltution_checker;
	pthread_create(&soltution_checker, NULL, task_handler, (void *)shared_data);

	wait_for_full_room(&shared_data->sem_room_full, max_player);
	printf("Multiplayer room_handler: Full room - game starting\n");

	cJSON *round_board;

	//TODO
	//get new board
	srand(time(NULL));
	shared_data->board_id = rand()% num_boards;
	int random_board = shared_data->board_id;

	round_board = cJSON_GetArrayItem(boards, random_board);
	//broadcast new board
	strcpy(shared_data->starting_board,cJSON_Print(cJSON_GetObjectItem(round_board, "starting_state")));


	//Start round
	clock_gettime(CLOCK_MONOTONIC, &start);
	for(int i = 0; i < max_player; i++) {
		sem_post(&shared_data->sem_game_start);
	}
	printf("Multiplayer Room Handler: game start has been signaled \n");

	//wait for correct solutions


	for(int i = 0; i < max_player; i++) {
		sem_wait(sem_solucao_encontrada);
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("%.5f\n", (double)(end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000 );
	}

	pthread_join(soltution_checker, NULL);

	//wait until all clients are done
	//dump stats

	// Clean up (done when server shuts down)
	shared_memory_clean_up(room_name, shared_data);

	return 0;
}


