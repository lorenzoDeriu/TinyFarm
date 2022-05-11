#include "xerrori.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#define INFO __LINE__,__FILE__
#define DEFAULT_STRING_SIZE 255
#define SHARED 1

#define END_OF_TASK "__END___OF____TASK__" // TODO trovare soluzione migliore

#define PORT 65001
#define HOST "127.0.0.1"

typedef struct {
	char **buffer;
	int buffer_size;
	pthread_mutex_t *buffer_mutex;
	sem_t *sem_free_slot;
	sem_t *sem_data_items;
	int *index;
} Thread_worker_args;

typedef struct {
	char *file_name;
	long result;
} Result;

char **remove_option(int*, char**);
bool is_option(char *);
void *thread_worker_body(void*);
int socket_create();

int main(int argc, char **argv) {
	int num_thread = 4;
	int buffer_size = 8;
	int delay = 0;
	char opt;

	while ((opt = getopt(argc, argv, "n:t:q:")) != -1) {
		switch (opt) {
			case 'n':
				num_thread = atoi(optarg);
				break;
			case 'q':
				buffer_size = atoi(optarg);
				break;
			case 't':
				delay = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s file [file ...] [-t nsecs] [-n num_thread] [-q buffer_size] \n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if (argc - optind < 2) {
		printf("Usage: %s file [file ...] [-n num_thread] [-q buffer_length] [-t delay] \n", argv[0]);
		return 1;
	}

	char **buffer = malloc(buffer_size * sizeof(char *));
	pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
	int worker_index = 0;
	int master_index = 0;

	sem_t sem_free_slot;
	sem_t sem_data_items;
	if (sem_init(&sem_free_slot, SHARED, buffer_size) != 0) termina("sem_init error");
	if (sem_init(&sem_data_items, SHARED, 0) != 0) termina("sem_init error");

	Thread_worker_args arg[num_thread];
	pthread_t thread_worker[num_thread];

	for (int i = 0;  i < num_thread; i++) {
		arg[i].buffer = buffer;
		arg[i].buffer_size = buffer_size;
		arg[i].buffer_mutex = &buffer_mutex;
		arg[i].sem_data_items = &sem_data_items;
		arg[i].sem_free_slot = &sem_free_slot;
		arg[i].index = &worker_index;

		xpthread_create(&thread_worker[i], NULL, thread_worker_body, arg+i, INFO);
	}

	for (int i = optind; i < argc; i++) {
		xsem_wait(&sem_free_slot, INFO);
		
		buffer[master_index % buffer_size] = strdup(argv[i]);
		master_index++;
		
		xsem_post(&sem_data_items, INFO);
		usleep(delay);
	}

	for (int i = 0; i < num_thread; i++) {
		xsem_wait(&sem_free_slot, INFO);
		
		buffer[master_index % buffer_size] = strdup(END_OF_TASK);
		master_index++;
		
		xsem_post(&sem_data_items, INFO);
	}

	for (int i = 0; i < num_thread; i++) pthread_join(thread_worker[i], NULL);

	// TODO: Chiudere il server

	// debug
	// printf("num_thread = %d\tbuffer_size = %d\tdelay = %d\n", num_thread, buffer_size, delay);
	// for (int i = 0; i < argc; i++) printf("%s\n", argv[i]);
	// end debug
	

	// TODO:
	//	-	buffer free
	// 	-	argv file_name

	return 0;
}


bool is_option(char *str) {
	return str[0] == '-';
}


void *thread_worker_body(void *arguments) {
	fprintf(stderr, "Thread partito\n");
	Thread_worker_args *args = (Thread_worker_args *)arguments;

	char *file_name = NULL;

	bool finished = false;

	while (!finished) {
		xsem_wait(args->sem_data_items, INFO);
		xpthread_mutex_lock(args->buffer_mutex, INFO);
		
		file_name = strdup(args->buffer[*(args->index) % args->buffer_size]);
		free(args->buffer[*(args->index) % args->buffer_size]);
		*args->index += 1;

		xpthread_mutex_unlock(args->buffer_mutex, INFO);
		xsem_post(args->sem_free_slot, INFO);

		if (file_name == NULL) xtermina("invalid read from buffer", INFO);
		if (strcmp(file_name, END_OF_TASK) == 0) {
			free(file_name);
			finished = true;
			continue;
		}

		FILE *file_desriptor = fopen(file_name, "r");
		if (file_desriptor == NULL) xtermina("fopen error", INFO);

		long number = 0;
		long result = 0;
		long i = 0;

		while (fread(&number, sizeof(long), 1, file_desriptor) != 0) {
			result += i * number;
			i++;
		}

		fclose(file_desriptor);

		int socket_file_descriptor = socket_create();
		struct sockaddr_in server_address;

		server_address.sin_family = AF_INET;

		server_address.sin_port = htons(PORT);
		server_address.sin_addr.s_addr = inet_addr(HOST);

		if (connect(socket_file_descriptor, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
			xtermina("Socket connection error", INFO);
		}

		char result_str[255];
		int result_length;

		if ((result_length = sprintf(result_str, "%ld", result)) < 0) xtermina("sprintf error", INFO);

		result_length = htonl(result_length);
		int writen_return_value = writen(socket_file_descriptor, &result_length, sizeof(result_length));
		if (writen_return_value != sizeof(result_length)) {
			xtermina("write error", INFO);
		}

		writen_return_value = writen(socket_file_descriptor, result_str, strlen(result_str));
		if (writen_return_value != strlen(result_str)) {
			xtermina("write error", INFO);
		}

		int file_name_length = htonl(strlen(file_name));
		writen_return_value = writen(socket_file_descriptor, &file_name_length, sizeof(file_name_length));
		if (writen_return_value != sizeof(file_name_length)) {
			xtermina("write error", INFO);
		}

		writen_return_value = writen(socket_file_descriptor, file_name, strlen(file_name));
		if (writen_return_value != (strlen(file_name))) {
			xtermina("write error", INFO);
		}

		free(file_name);
	}

	pthread_exit(NULL);
}

int socket_create() {
	int file_descriptor = 0;

	if ((file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		xtermina("socket creation erro", INFO);
	}

	return file_descriptor;
}