#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>



typedef struct message_info {
	int port;
	char* ip_addr;
	char* message;
} message_info;

typedef struct message_sender_info {
	int port;
	int message_size;
	char* ip_addr;
	char* keep_alive;
} message_sender_info;

typedef struct timer_info {
	int time;
	char* keep_alive;
} timer_info;


char* read_random_bytes(int num_bytes) {
	char* buffer = malloc(num_bytes);
	FILE* fp = fopen("/dev/urandom", "r");
	int bytes_read = fread(buffer, 1, num_bytes, fp);


	fclose(fp);
	return buffer;
}

// message_id = exactly 4 bytes long 
// random_bytes = exactly num_bytes long
char* create_message(int num_bytes, char* message_id, char* random_bytes) {
	char* message = random_bytes;

	//char* syn = "SYN\0";
	//char* fin = "FIN\0";
	char* syn = "SYN*";
	char* fin = "FIN*";

	memcpy(message, syn, 4);
	memcpy(&message[4], message_id, 4);

	memcpy(&message[num_bytes-8], fin, 4);
	memcpy(&message[num_bytes-4], message_id, 4);

	//printf("%s\n", message);

	return message;
}

int open_connection(char* ip_addr, int port) {
	int client_fd;
	struct sockaddr_in address;


	// open a socket
	if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket creation error");
		exit(EXIT_FAILURE);
	}

	// initialize the tcp:<ip_addr>:<port_num>
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip_addr);
	address.sin_port = htons(port);

	// connect to server
	if (connect(client_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		printf("failed port: %i\n", port);
		perror("connect failed");
		exit(EXIT_FAILURE);
	}

	return client_fd;
}

// 
//void* send_message(void* mi) {
void send_message(int port, char* ip_addr, char* message) {
	// parse message info out of void* variable
	/*message_info* mes_inf = (message_info*) mi;
	int port = mes_inf->port;
	char* ip_addr = mes_inf->ip_addr;
	char* message = mes_inf->message; */


	//printf("ip address: %s\n", ip_addr);
	//printf("message: %s\n", message);
	//printf("port: %i\n", port);

	int client_fd;
	struct sockaddr_in address;

	// open a socket
	if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket creation error");
		//exit(EXIT_FAILURE);
	}

	// initialize the tcp:<ip_addr>:<port_num>
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip_addr);
	address.sin_port = htons(port);

	// connect to server
	if (connect(client_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		printf("failed port: %i\n", port);
		perror("connect failed");
		//exit(EXIT_FAILURE);
	}

	// send message
	send(client_fd, message, strlen(message)+1, 0);

	
	printf("message sent: \'%s\'\n", message);
	printf("message length: %li\n", strlen(message));

	shutdown(client_fd, SHUT_WR);

	//pthread_exit(NULL);
}




// gets called num_threads amount of times
void* send_message_loop(void* msi) {
	message_sender_info* m_s_i = (message_sender_info*) msi;
	int port = m_s_i->port;
	int message_size = m_s_i->message_size;
	char* ip_addr = m_s_i->ip_addr;
	char* keep_alive = m_s_i->keep_alive;

	int num_sent = 0;
	char* random_bytes = read_random_bytes(message_size);

	// tcp SYN packet
	int client_fd = open_connection(ip_addr, port);
	char message_id_0 = (char)0; //read_random_bytes(4);
	//char* message_id_0 = "0000";
	
	char* recv_buffer = malloc(2048);
	// repeatedly send message
	while (*keep_alive == 1) {
		//create message
		int message_id = (int)message_id_0 + num_sent;
		char* message = create_message(message_size, (char*)&message_id, random_bytes);

		// send message
		if (send(client_fd, message, message_size, 0) < 0) {
			printf("failed to send %i\n", num_sent);
		}
		else {
			int bytes_read = read(client_fd, recv_buffer, 2048);
		}

		num_sent += 1;
		printf("num_sent:%i: %i\n", port, num_sent);
		//sleep(0.01);
	}

	// tcp FIN packet
	if (shutdown(client_fd, SHUT_WR) != 0) {
		close(client_fd);
		printf("unable to shutdown properly\n");
	}
	else {
		close(client_fd);
		printf("proper shutdown\n");
	}
	
	//free(message_id_0);
	free(random_bytes);

	pthread_exit(NULL);
}

void* timer_thread(void* ti) {
	timer_info* tim_inf = (timer_info*) ti;
	int t = tim_inf->time;
	char* keep_alive = tim_inf->keep_alive;

	sleep(t);
	*keep_alive = 0;

	pthread_exit(NULL);
}

// called once
// this creates all the threads
void send_n_messages(int num_messages, int start_port, int message_size, int time, char* ip_addr) {

	pthread_t threads[num_messages+1];
	//struct message_info mi[num_messages];
	struct message_sender_info msi[num_messages];

	char keep_alive = (char) 1;


	
	for (int i = 0; i < num_messages; i++) {
		//struct message_info mi;
		//mi[i].port = start_port + i;
		//mi[i].ip_addr = ip_addr;
		//mi[i].message = (char*)malloc(1024);
		//sprintf(mi[i].message, "hello message: %i", i);

		msi[i].port = start_port + i;
		msi[i].message_size = message_size;
		msi[i].ip_addr = ip_addr;
		msi[i].keep_alive = &keep_alive;


		//printf("mi.port: %i\n", mi.port);
		//printf("mi.ip-addr: %s\n", mi.ip_addr);
		//printf("mi.message: %s\n", mi.message);
		//mi.message = "hello message\0";

		int thread_val = pthread_create(&threads[i], NULL, send_message_loop, (void*)&msi[i]);

		//int thread_val = pthread_create(&threads[i], NULL, send_message, (void*)&mi[i]);

		//free(&mi);
	}

	if (time != 0) {
		struct timer_info ti;
		ti.time = time;
		ti.keep_alive = &keep_alive;
		pthread_t t_thread;
		pthread_create(&t_thread, NULL, timer_thread, (void*)&ti);
	}

	for (int i = 0; i < num_messages; i++){
		printf("prejoin %i\n", i);
		pthread_join(threads[i], NULL);
		printf("postjoin %i\n", i);
	}

	pthread_exit(NULL);
}

void single_thread_send_messages(int num_messages, int start_port, int message_size, int time, char* ip_addr) {

	char keep_alive = (char) 1;

	struct timer_info ti;
	ti.time = time;
	ti.keep_alive = &keep_alive;
	pthread_t t_thread;
	pthread_create(&t_thread, NULL, timer_thread, (void*)&ti);

	int num_sent = 0;
	char* rand_message = read_random_bytes(message_size);
	while (keep_alive == 1) {
		for (int i = 0; i < num_messages; i++) {

			num_sent += 1;
			int port = start_port + i;


			printf("num_sent:%i: %i\n", port, num_sent);


			int client_fd = open_connection(ip_addr, port);

			if (send(client_fd, rand_message, message_size+1, 0) < 0) {
				printf("failed to send %i\n", num_sent);
			}

			if (shutdown(client_fd, SHUT_WR) != 0) {
				close(client_fd);
				printf("unable to shutdown properly\n");
			}
			else {
				close(client_fd);
				printf("proper shutdown\n");
			}
		}
	}
	free(rand_message);

	pthread_exit(NULL);
}




// ./client <num_connections> <runtime> <server_ip> <message_size>
int main(int argc, char const* argv[]) {

	// parse arguments
	
	int num_servers = atoi(argv[1]);
	int runtime = atoi(argv[2]);
	char* ip_addr = (char*)argv[3];
	int message_size = atoi(argv[4]);
	//char* ip_addr = "10.16.224.68";


	int start_port = 26000;

	
	//int message_size = 20000;
	//int message_size = 100000;


	//int runtime = 2;





	char* message_id = "abcd";
	char* random_bytes = read_random_bytes(40);
	char* message = create_message(38, message_id, random_bytes);

	send_n_messages(num_servers, start_port, message_size, runtime, ip_addr);

}
