	#include <netinet/in.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <unistd.h>
	#include <sys/wait.h>
	#define PORT 8080

	char *comPort = NULL;
	int port;

	int incPort(){
		char temp[4];
		int currentPort = atoi(comPort);
		port = currentPort;
		currentPort++;
		sprintf(temp, "%d", currentPort);
		strcpy(comPort, temp);
		return 0;
	}

	int initFTP(){
		int server_fd, new_socket, valread;
		struct sockaddr_in address;
		int opt = 1;
		int addrlen = sizeof(address);
		char buffer[1024] = { 0 };
		char* invalid = "invalid request";

		if ((server_fd = socket(AF_INET, SOCK_STREAM, 0))
			== 0) {
			perror("socket failed");
			exit(EXIT_FAILURE);
		}

		if (setsockopt(server_fd, SOL_SOCKET,
					SO_REUSEADDR | SO_REUSEPORT, &opt,
					sizeof(opt))) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(port);

		if (bind(server_fd, (struct sockaddr*)&address,
				sizeof(address))
			< 0) {
			perror("bind failed");
			exit(EXIT_FAILURE);
		}
		if (listen(server_fd, 3) < 0) {
			perror("listen");
			exit(EXIT_FAILURE);
		}
		if ((new_socket
			= accept(server_fd, (struct sockaddr*)&address,
					(socklen_t*)&addrlen))
			< 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		valread = read(new_socket, buffer, 1024);
		
		//Check each command here with strcmp and buffer


		close(new_socket);
		shutdown(server_fd, SHUT_RDWR);

		return 0;
	}

	int initializeConn(){
	int server_fd, new_socket, valread;
		struct sockaddr_in address;
		int opt = 1;
		int addrlen = sizeof(address);
		char buffer[1024] = { 0 };
		char* invalid = "invalid request";

		if ((server_fd = socket(AF_INET, SOCK_STREAM, 0))
			== 0) {
			perror("socket failed");
			exit(EXIT_FAILURE);
		}

		if (setsockopt(server_fd, SOL_SOCKET,
					SO_REUSEADDR | SO_REUSEPORT, &opt,
					sizeof(opt))) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(PORT);

		if (bind(server_fd, (struct sockaddr*)&address,
				sizeof(address))
			< 0) {
			perror("bind failed");
			exit(EXIT_FAILURE);
		}
		if (listen(server_fd, 3) < 0) {
			perror("listen");
			exit(EXIT_FAILURE);
		}
		if ((new_socket
			= accept(server_fd, (struct sockaddr*)&address,
					(socklen_t*)&addrlen))
			< 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		valread = read(new_socket, buffer, 1024);
		if(strcmp(buffer,"init")==0){
			pid_t pid = fork();
			if(pid==0){
				send(new_socket, comPort, sizeof(comPort), 0);
				close(new_socket);
				shutdown(server_fd, SHUT_RDWR);
				initFTP();
				exit(0);
			}
			incPort();
		}
		else
				send(new_socket, invalid, strlen(invalid), 0);
		
		close(new_socket);
		shutdown(server_fd, SHUT_RDWR);

		return 0;
	}

	int main(int argc, char const* argv[])
	{
		comPort = malloc(sizeof(char)*4);
		strcpy(comPort,"9000");

		while(1){
			initializeConn();
		}
		
		free(comPort);
		return 0;
	}
