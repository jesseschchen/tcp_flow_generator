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
	char* keep_alive;
	int message_size;
} server_info;

typedef struct timer_info {
	int time;
	char* keep_alive;
} timer_info;


long accept_connection(int new_socket, int message_size) {
	int buffer_size = 4096;
	//char buffer[buffer_size];
	char* buffer = malloc(buffer_size);
	buffer[0] = '\0';

	//printf("preread buffer: %s\n", buffer);
	//printf("new_socket: %i\n", new_socket);

	int read_size = 0;
	char* message = "ok";
	long total_read = 0;
	while(read_size = read(new_socket, buffer, buffer_size)) {
		total_read += read_size;
		//printf("total_read: %i\n", total_read);
		//if (total_read >= message_size) {
		//	int send_val = send(new_socket, message, strlen(message), 0);
		//	total_read = 0;
		//}
	}

	if(shutdown(new_socket, SHUT_RD) != 0) {
		//close(new_socket);
		printf("failed shutdown\n");
	}
	else {
		close(new_socket);
		//printf("successful shutdown\n");
	}

	//printf("message: %s\n\n", buffer);

	free(buffer);
	return total_read;
}

void* timer_thread(void* ti) {
	timer_info* tim_inf = (timer_info*) ti;
	int t = tim_inf->time;
	char* keep_alive = tim_inf->keep_alive;

	sleep(t);
	*keep_alive = 0;

	pthread_exit(NULL);
}


// currently only accepts one connection and closes
void* start_tcp_server(void* si) {
	// parse server info out of void* variable
	server_info* ser_inf = (server_info*) si;
	int port = ser_inf->port;
	char* ip_addr = ser_inf->ip_addr;
	char* keep_alive = ser_inf->keep_alive;
	int message_size = ser_inf->message_size;


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

	printf("TCP server started on %i\n", port);

	int num_connections = 0;

	//accept new connections
	//while (*keep_alive == 1) {
		new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
		if (new_socket < 0) {
			//perror("accept failed");
			//exit(EXIT_FAILURE);			
		}
		else {
			// accept is successful
			num_connections += 1;
			printf("connection accepted: %i\n", port);
			long total_read = accept_connection(new_socket, message_size);
			printf("connection ended: %i: %li bytes received\n", port, total_read);
		}
	//}
	pthread_exit(NULL);
	//return server_fd;
}

void* start_udp_server(void* si) {
	server_info* ser_inf = (server_info*) si;
	int port = ser_inf->port;
	char* ip_addr = ser_inf->ip_addr;
	char* keep_alive = ser_inf->keep_alive;
	int message_size = ser_inf->message_size;


	int server_fd, new_socket;

	// we want a IPv4(AF_INET), UDP(SOCK_DGRAM) socket
	if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip_addr);
	address.sin_port = htons(port);
	int addrlen = sizeof(address);

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	printf("UDP server started on %i\n", port);

	int n;
	int buffer_size = 4096;
	char* msg_buffer = malloc(buffer_size);
	long total_read = 0;

	while(*keep_alive == 1) {
		// recvfrom() is used when want to know src of messages
		//n = recvfrom(server_fd, msg_buffer, buffer_size, 0, (struct sockaddr*)&address, (socklen_t*)&addrlen);
		// udp will drop the rest of packets taht hre not fully read
		n = recv(server_fd, msg_buffer, buffer_size, MSG_DONTWAIT);

		// avoids adding -1 to total_read since recv() is non-blocking
		if (n >= 0) 
			total_read += n;

		msg_buffer[8] = '\0';
		//printf("read: %i\n", n);
		//printf("msg: %s\n", msg_buffer);
	}


	printf("server closed: >%li bytes received\n", total_read);

	free(msg_buffer);
	pthread_exit(NULL);
}

void start_n_servers(int num_servers, int start_port, int runtime, char* ip_addr, int message_size, int tcp) {
	pthread_t threads[num_servers+1];
	struct server_info si[num_servers];

	char keep_alive = (char) 1;


	for(int i = 0; i < num_servers; i++) {
		// server info struct
		si[i].port = start_port + i;
		si[i].ip_addr = ip_addr;
		si[i].keep_alive = &keep_alive;
		si[i].message_size = message_size;

		// create a server thread
		int thread_val;
		if (tcp) {
			thread_val = pthread_create(&threads[i], NULL, start_tcp_server, (void*)&si[i]);
		}
		else {
			thread_val = pthread_create(&threads[i], NULL, start_udp_server, (void*)&si[i]);
		}
		// start the server thread
	}


	// runtime = 0 means run forever
	if(runtime != 0) {
		struct timer_info ti;
		ti.time = runtime;
		ti.keep_alive = &keep_alive;
		pthread_create(&threads[num_servers], NULL, timer_thread, (void*)&ti);
	}

	for (int i = 0; i < num_servers; i++) {
		int join_val = pthread_join(threads[i], NULL);
		printf("thread %i joined\n", i);
	}


	pthread_exit(NULL);
}

// not used
void start_one_server(int num_servers, int start_port, int runtime, char* ip_addr) {
	char keep_alive = (char) 1;

	int server_fd, new_socket;

	struct sockaddr_in address;
	int addrlen = sizeof(address);

	// we want a IPv4(AF_INET), TCP(SOCK_STREAM) socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < num_servers; i++) {
		// set IPv4, IP address, and port for address
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = inet_addr(ip_addr);
		address.sin_port = htons(start_port + i);

		// bind socket to particular address
		if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
			perror("bind failed");
			exit(EXIT_FAILURE);
		}
	}

	// listen on that socket
	if (listen(server_fd, 65) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	//printf("server started on %i\n", port);

	int num_connections = 0;


	//accept new connections
	while ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) && (keep_alive == 1)) {
		if (new_socket < 0) {
			perror("accept failed");
			exit(EXIT_FAILURE);			
		}
		else {
			// accept is successful
			num_connections += 1;
			//printf("connection accepted:%i: %i\n", port, num_connections);
			//accept_connection(new_socket, &keep_alive);
		}
	}



	// timer stuff
	if(runtime != 0) {
		struct timer_info ti;
		ti.time = runtime;
		ti.keep_alive = &keep_alive;
		pthread_t t_thread;
		pthread_create(&t_thread, NULL, timer_thread, (void*)&ti);
	}


	pthread_exit(NULL);
}


// ./server <num_servers> <runtime> <server_ip> <message_size> <start_port> <tcp>
int main(int argc, char const *argv[]) {

	int num_servers = atoi(argv[1]);
	int runtime = atoi(argv[2]);
	char* ip_addr = (char*)argv[3];
	int message_size = atoi(argv[4]);
	int start_port = atoi(argv[5]);
	int tcp = atoi(argv[6]);
	//char* ip_addr = "10.16.224.68";


	//int start_port = 26000;

	//start_server(start_port, ip_addr);

	//int tcp = 0;

	start_n_servers(num_servers, start_port, runtime, ip_addr, message_size, tcp);

	//start_one_server(num_servers, start_port, runtime, ip_addr);

	return 0;

	
}
 
