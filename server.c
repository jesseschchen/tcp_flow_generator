#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


typedef struct server_info {
	int port;
	char* ip_addr;
} server_info;

typedef struct timer_info {
	int time;
	char* keep_alive;
} timer_info;


void accept_connection(int new_socket) {
	int buffer_size = 2048;
	//char buffer[buffer_size];
	char* buffer = malloc(buffer_size);
	buffer[0] = '\0';

	//printf("preread buffer: %s\n", buffer);

	int read_size = 0;
	while(read_size = read(new_socket, buffer, buffer_size)) {
		//printf("read: %i\n", read_size);
	}

	if(shutdown(new_socket, SHUT_RD) != 0) {
		printf("failed shutdown\n");
	}
	else {
		close(new_socket);
		printf("successful shutdown\n");
	}

	//printf("message: %s\n\n", buffer);

	free(buffer);
	// what to do once connection is accepted
}

void* timer_thread(void* ti) {
	timer_info* tim_inf = (timer_info*) ti;
	int t = tim_inf->time;
	char* keep_alive = tim_inf->keep_alive;

	sleep(t);
	*keep_alive = 0;

	pthread_exit(NULL);
}


//int start_server(int port, char* ip_addr) {  
void* start_server(void* si) {
	// parse server info out of void* variable
	server_info* ser_inf = (server_info*) si;
	int port = ser_inf->port;
	char* ip_addr = ser_inf->ip_addr;
	char* keep_alive = ser_inf->keep_alive;


	int server_fd, new_socket;


	struct sockaddr_in address;
	int addrlen = sizeof(address);

	// we want a IPv4(AF_INET), TCP(SOCK_STREAM) socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//if (setsockopt(server_fd, SOL_SOCKET, SO))

	// set IPv4, IP address, and port for address
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip_addr);
	address.sin_port = htons(port);

	// bind socket to particular address
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	// listen on that socket
	if (listen(server_fd, 65) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	printf("server started on %i\n", port);

	int num_connections = 0;

	//accept new connections
	while ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) && (*keep_alive == 1)) {
		if (new_socket < 0) {
			perror("accept failed");
			exit(EXIT_FAILURE);			
		}
		else {
			// accept is successful
			num_connections += 1;
			printf("connection accepted:%i: %i\n", port, num_connections);
			accept_connection(new_socket);
		}
	}
	pthread_exit(NULL);
	//return server_fd;
}


void start_n_servers(int num_servers, int start_port, char* ip_addr) {
	pthread_t threads[num_servers+1];
	struct server_info si[num_servers];

	char keep_alive = (char)1;


	for(int i = 0; i < num_servers; i++) {
		// server info struct
		si[i].port = start_port + i;
		si[i].ip_addr = ip_addr;
		si[i].keep_alive = &keep_alive;

		// create a server thread
		int thread_val = pthread_create(&threads[i], NULL, start_server, (void*)&si[i]);
		// start the server thread
	}


	struct timer_info ti;
	ti.time = time;
	ti.keep_alive = &keep_alive;
	pthread_create(&threads[num_servers], NULL, timer_thread, (void*)&ti);


	pthread_exit(NULL);
}

//git comment


// ./server <num_servers> <runtime>
int main(int argc, char const *argv[]) {


	int num_servers = atoi(argv[1]);
	int runtime = atoi(argv[2]);

	int start_port = 26000;
	char* ip_addr = "10.16.224.68";

	//start_server(start_port, ip_addr);

	start_n_servers(num_servers, start_port, ip_addr);

	for (int i = 0; i < num_servers; i++) {
		int a = 0;//start_server(start_port + i, ip_addr);
	}
	
}
 
