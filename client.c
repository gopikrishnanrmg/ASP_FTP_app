		#include <arpa/inet.h>
		#include <stdio.h>
		#include <stdlib.h>
		#include <string.h>
		#include <sys/socket.h>
		#include <unistd.h>
		#define PORT 8080

		int initializeConn(){
		int sock = 0, valread, client_fd;
			struct sockaddr_in serv_addr;
			char* init = "init";
			char buffer[1024] = { 0 };
			if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				printf("\n Socket creation error \n");
				return -1;
			}

			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(PORT);

			
			if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
				<= 0) {
				printf(
					"\nInvalid address/ Address not supported \n");
				return -1;
			}

			if ((client_fd
				= connect(sock, (struct sockaddr*)&serv_addr,
						sizeof(serv_addr)))
				< 0) {
				printf("\nConnection Failed \n");
				return -1;
			}
			send(sock, init, strlen(init), 0);
			valread = read(sock, buffer, 1024);

			if(strcmp(buffer, "invalid request")!=0){
				close(client_fd);
				printf("%d\n", atoi(buffer));

			}
			else 
				close(client_fd);

			close(client_fd);
			return 0;
		}

		int main(int argc, char const* argv[])
		{
			initializeConn();
			return 0;
		}
