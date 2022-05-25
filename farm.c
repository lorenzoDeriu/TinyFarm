#include "xerrori.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#define INFO __LINE__,__FILE__
#define SHARED 1

#define END_OF_TASK "__END___OF____TASK__"

#define DEFAULT_PORT 65123
#define DEFAULT_HOST "127.0.0.197"

volatile bool _interrupt = false;

typedef struct {
	char **buffer;
	int buffer_size;
	pthread_mutex_t *buffer_mutex;
	sem_t *sem_free_slot;
	sem_t *sem_data_items;
	int *index;
	int port;
	char *host_address;
} Thread_worker_args;

char **remove_option(int *, char **);
bool is_option(char *);
void *thread_worker_body(void *);
int socket_create();
void send_to(int, void *, size_t);
void close_server();

void handler(int);

int main(int argc, char **argv) {
	int num_thread = 4;
	int buffer_size = 8;
	int delay = 0;
	char *host_address = DEFAULT_HOST;
	int port = DEFAULT_PORT;
	char opt;

	struct sigaction sa;
	sigaction(SIGINT, NULL, &sa); 
	sa.sa_handler = handler;
	sigaction(SIGINT, &sa, NULL); 

	while ((opt = getopt(argc, argv, "n:t:q:p:h:")) != -1) {
		switch (opt) {
			case 'n':
				if (num_thread < 1) {
					fprintf(stderr, "Il numero di thread deve essere maggiore di 1, verrà usato il numero di thread di default\n");
				} else num_thread = atoi(optarg);
				break;
			case 'q':
				if (buffer_size < 1) {
					fprintf(stderr, "The size of the buffer must be higher than 1");
				} else buffer_size = atoi(optarg);
				break;
			case 't':
				if (atoi(optarg) < 0) {
					fprintf(stderr, "il tempo di delay non può essere negativo, verrà usato il valore di default\n");
				} else delay = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'h':
				host_address = optarg;
				break;
		}
	}

	if (argc - optind < 1) {
		fprintf(stderr, "Usage: %s file [file ...] [-n num_thread] [-q buffer_length] [-t milliseconds_delay] [-h new_host_address] [-p new_port]\n", argv[0]);
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
		arg[i].port = port;
		arg[i].host_address = host_address;

		xpthread_create(&thread_worker[i], NULL, thread_worker_body, arg+i, INFO);
	}

	for (int i = optind; i < argc && !_interrupt; i++) {
		xsem_wait(&sem_free_slot, INFO);
		
		buffer[master_index % buffer_size] = argv[i];
		master_index++;
		
		xsem_post(&sem_data_items, INFO);
		usleep(delay);
	}

	for (int i = 0; i < num_thread; i++) {
		xsem_wait(&sem_free_slot, INFO);
		
		buffer[master_index % buffer_size] = END_OF_TASK;
		master_index++;
		
		xsem_post(&sem_data_items, INFO);
	}

	for (int i = 0; i < num_thread; i++) xpthread_join(thread_worker[i], NULL, INFO);
	
	free(buffer);

	close_server(port, host_address);

	return 0;
}

void *thread_worker_body(void *arguments) {
	Thread_worker_args *args = (Thread_worker_args *)arguments;

	char *file_name = NULL;

	while (true) {
		xsem_wait(args->sem_data_items, INFO);
		xpthread_mutex_lock(args->buffer_mutex, INFO);
		
		file_name = strdup(args->buffer[*(args->index) % args->buffer_size]);
		*args->index += 1;

		xpthread_mutex_unlock(args->buffer_mutex, INFO);
		xsem_post(args->sem_free_slot, INFO);

		if (file_name == NULL) xtermina("invalid read from buffer", INFO);
		if (strcmp(file_name, END_OF_TASK) == 0) {
			free(file_name);
			break;
		}

		FILE *file_desriptor = fopen(file_name, "r");
		if (file_desriptor == NULL) {
			fprintf(stderr, "Errore: il file %s non esiste\n", file_name);
			free(file_name);
			continue;
		}

		long number = 0;
		long result = 0;
		long i = 0;

		while (fread(&number, sizeof(long), 1, file_desriptor) != 0) {
			result += i * number;
			i++;
		}

		fclose(file_desriptor);
 
		int socket_file_descriptor = socket_create();
		if (socket_file_descriptor == -1) {
			fprintf(stderr, "Il file descriptor del socket non è stato creato correttamente\n");
			free(file_name);
			continue;
		}

		struct sockaddr_in server_address;

		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(args->port);
		server_address.sin_addr.s_addr = inet_addr(args->host_address);

		if (connect(socket_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
			close(socket_file_descriptor);
			fprintf(stderr, "L'invio dei risultati del file %s è fallito.\n", file_name);
			free(file_name);
			continue;
		}

		char *result_str = malloc(255 * sizeof(char));
		int result_length;

		if ((result_length = sprintf(result_str, "%ld", result)) < 0) xtermina("sprintf error", INFO);

		result_length = htonl(result_length);
		send_to(socket_file_descriptor, (void *)&result_length, sizeof(result_length));
		send_to(socket_file_descriptor, (void *)result_str, strlen(result_str));
		
		free(result_str);

		int file_name_length = htonl(strlen(file_name));
		send_to(socket_file_descriptor, (void *)&file_name_length, sizeof(file_name_length));
		send_to(socket_file_descriptor, (void *)file_name, strlen(file_name));
	
		free(file_name);
		
		close(socket_file_descriptor);
	}

	pthread_exit(NULL);
}

int socket_create() {
	int file_descriptor = 0;

	if ((file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

	return file_descriptor;
}

void send_to(int socket_file_descriptor, void *data, size_t data_size) {
	int writen_return_value = writen(socket_file_descriptor, data, data_size);
	if (writen_return_value != data_size) {
		xtermina("writen error", INFO);
	}
}

void close_server(int port, char *host_address) {
	int socket_file_descriptor = socket_create();
	struct sockaddr_in server_address;

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = inet_addr(host_address);

	if (connect(socket_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
		close(socket_file_descriptor);
		fprintf(stderr, "Il server non risulta aperto.\n");
		return;
	}

	char *result_str = "0";
	int result_length;

	result_length = htonl(strlen(result_str));
	send_to(socket_file_descriptor, (void *)&result_length, sizeof(result_length));
	send_to(socket_file_descriptor, (void *)result_str, strlen(result_str));
	
	int length = htonl(strlen(END_OF_TASK));
	send_to(socket_file_descriptor, (void *)&length, sizeof(length));
	send_to(socket_file_descriptor, (void *)END_OF_TASK, strlen(END_OF_TASK));

	close(socket_file_descriptor);
}

void handler(int signal) {
	if (signal == SIGINT) _interrupt = true;
}
