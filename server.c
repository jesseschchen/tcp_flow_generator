#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


typedef struct receiv_info {
	int num_connections;
	int* connection_fds;
	char* fd_validity;
	char* keep_alive;
} receiv_info;


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

	exit(0);

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
		new_socket = accept(server_fd, NULL, NULL);
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
	int buffer_size = 8192;
	char* msg_buffer = malloc(buffer_size);
	long total_read = 0;

	while(*keep_alive == 1) {
		// recvfrom() is used when want to know src of messages
		//n = recvfrom(server_fd, msg_buffer, buffer_size, 0, (struct sockaddr*)&address, (socklen_t*)&addrlen);
		// udp will drop the rest of packets taht hre not fully read
		//n = recv(server_fd, msg_buffer, buffer_size, MSG_DONTWAIT);
		n = recv(server_fd, msg_buffer, buffer_size, 0);

		// avoids adding -1 to total_read since recv() is non-blocking
		if (n >= 0) 
			total_read += n;
		printf("tota_read: %li\n", total_read);

		msg_buffer[8] = '\0';
		//printf("read: %i\n", n);
		//printf("msg: %s\n", msg_buffer);
	}


	printf("server closed: >%li bytes received\n", total_read);

	free(msg_buffer);
	pthread_exit(NULL);
}

int check_valid_connections(char* fd_validity, int num_connections) {
	int valid_connections = 0;
	for (int i = 0; i < num_connections; i++) {
		if (fd_validity[i] == 1) {
			valid_connections += 1;
		}
	}
	return valid_connections;
}

void* receiv_thread(void* ri) {
	receiv_info* rec_inf = (receiv_info*) ri;
	int num_connections = rec_inf->num_connections;
	int* connection_fds = rec_inf->connection_fds;
	char* fd_validity = rec_inf->fd_validity;
	char* keep_alive = rec_inf->keep_alive;

	int buffer_size = 4096;
	char* buffer = malloc(buffer_size);

	int read_val;
	int valid_connections;
	while(*keep_alive == 1) {
		for (int i = 0; i < num_connections; i++) {
			// if connection is still open
			if (fd_validity[i]) {
				read_val = recv(connection_fds[i], buffer, buffer_size, MSG_DONTWAIT);
				if (read_val == 0) {
					if(shutdown(connection_fds[i], SHUT_RD) != 0) {
						printf("failed shutdown\n");
					}
					else {
						close(connection_fds[i]);
						fd_validity[i] = (char) 0;
						valid_connections = check_valid_connections(fd_validity, num_connections);
						if (!valid_connections) {
							exit(0);
						}
					}
				}
				printf("read_val: %i\n", read_val);
			}
		}
	}
}

// prep server sockets for sequentially connection handling
int do_tcp_server_prep(char* ip_addr, int port) {
	struct sockaddr_in address;
	int server_fd;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip_addr);
	address.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 65) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	printf("TCP server on port %i prepped\n", port);
	return server_fd;
}

// starts sequential servers
// avoids cpu scheduling problems/starvations
void start_n_seq_servers(int num_servers, int start_port, int runtime, char* ip_addr, int message_size, int tcp) {
	char keep_alive = (char) 1;
	int server_fds[num_servers];
	int num_connections = 0;
	int connection_fds[num_servers];

	pthread_t receiv_thread;
	struct receiv_info ri;
	ri.num_connections = num_connections;
	ri.connection_fds = (int*)&connection_fds;




	int port;
	for (int i = 0; i < num_servers; i++) {
		port = start_port + i;
		if (tcp) {
			server_fds[i] = do_tcp_server_prep(ip_addr, port);
		}
		else {
			// do_udp_server_prep(ip_addr, port);
		}
	}

	//listen/accept() forever loop
	int new_socket;
	//while(keep_alive == 1) {
		for (int i = 0; i < num_servers; i++) {
			if (tcp) {
				if ((new_socket = accept(server_fds[i], NULL, NULL)) > 0) {
					//add new_socket to list of open connections to be read()/recv()s from 
					// if accept() works, add new_socket to a structure that can be read by receiv_thread

					//close old server socket
					close(server_fds[i]);

					connection_fds[num_connections] = new_socket;
					num_connections += 1;
				}
				else {
					//keep looping i guess
				}
			}
			else { // UDP SERVERS
				// UDP SERVER PLACEHOLDERS
			}
		}
	//}
}


//mulithreading 
void start_n_servers(int num_servers, int start_port, int runtime, char* ip_addr, int message_size, int tcp) {
	pthread_t threads[num_servers+1];
	struct server_info si[num_servers];

	char keep_alive = (char) 1;




	for(int i = 0; i < num_servers; i++) {
		// server info struct
		//si[i].port = start_port + i; // start unique servers
		si[i].port = start_port; // start on one port
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
	struct timer_info ti;
	if(runtime != 0) {
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


// successor to start_n_seq_servers 
// USE THIS ONE
void start_one_server_n_receiv(int num_threads, int num_connections, int start_port, int runtime, char* ip_addr, int message_size, int tcp) {
	char keep_alive = (char) 1;
	char fd_validity[num_connections];
	int connection_fds[num_connections];
	// prep fd_validity
	for (int i = 0; i < num_connections; i++) {
		fd_validity[i] = (char) 0;
	}


	// start num_threads receiv_threads
	pthread_t receiv_threads[num_threads];
	struct receiv_info ri;
	ri.num_connections = num_connections;
	ri.connection_fds = (int*)&connection_fds;
	ri.fd_validity = (char*)&fd_validity;
	ri.keep_alive = &keep_alive;
	for (int i = 0; i < num_threads; i++) {
		pthread_create(&receiv_threads[i], NULL, receiv_thread, (void*)&ri);
		printf("created receiv_thread:%i\n", i);
	}



	// timer thread
	pthread_t t_thread;
	struct timer_info ti;
	if(runtime != 0) {
		ti.time = runtime;
		ti.keep_alive = &keep_alive;
		pthread_create(&t_thread, NULL, timer_thread, (void*)&ti);
	}
	// opens server
	int server_fd;
	if (tcp) {
		//server_fd = do_tcp_server_prep(ip_addr, 26001); // LOCAL TESTING ONLY
		server_fd = do_tcp_server_prep(ip_addr, start_port);
	}


	// accept tcp connections loop
	struct sockaddr_in client_address;
	int len = sizeof(client_address);
	int new_socket;
	int connection_id;
	while (keep_alive == 1) {
		new_socket = accept(server_fd, (struct sockaddr*)&client_address, &len);
		printf("connection accepted!\n");
		connection_id = ntohs(client_address.sin_port) - start_port;
		printf("got connection id: %i\n", connection_id);

		connection_fds[connection_id] = new_socket;
		fd_validity[connection_id] = (char)1;
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
	int num_threads = 2;

	//start_n_servers(num_servers, start_port, runtime, ip_addr, message_size, tcp);

	start_one_server_n_receiv(num_threads, num_servers, start_port, runtime, ip_addr, message_size, tcp);

	//start_one_server(num_servers, start_port, runtime, ip_addr);

	return 0;

	
}
 
