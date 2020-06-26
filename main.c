#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

char* ip_addr = "10.16.224.68";

void accept_connection(int new_socket) {
	int a = 5;
	// what to do once connection is accepted
}

int main(int argc, char const *argv[]) {
	printf("hello asshole\n");

	int server_fd, new_socket, val_read;

	int start_port = 26000;

	struct sockaddr_in address;
	int addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//if (setsockopt(server_fd, SOL_SOCKET, SO))

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip_addr);
	address.sin_port = htons(8080);

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 65) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	while (new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) {
		if (new_socket < 0) {
			perror("accept failed");
			exit(EXIT_FAILURE);			
		}
		else {
			accept_connection(new_socket);
			// accept is successful


		}
	}
	
}
 
