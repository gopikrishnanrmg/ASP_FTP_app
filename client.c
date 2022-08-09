#include <arpa/inet.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080
#define CMD_SIZE 1024

char commands[CMD_SIZE][CMD_SIZE];
int cmdLen;


int userCommand(int sock){
	int valread;
	char buffer[CMD_SIZE];
	char *login = getlogin();
	strcpy(buffer, "USER");
	// perror("IN USER COMMAND");
	send(sock, buffer, strlen(buffer), 0);
	// perror("SENT USER COMMAND");

	valread = read(sock, buffer, CMD_SIZE);
		if(strcmp(buffer, "200")){
			send(sock, login, strlen(login), 0);
			valread = read(sock, buffer, CMD_SIZE);
			printf("%s \n",buffer);
		}

	perror("EXITING USER COMMAND");
	
	return 0;
}

int initFTP(int port){
	perror("ENTERING initFTP");

	int sock = 0, valread, client_fd;
	struct sockaddr_in serv_addr;
	char* init = "init";
	char buffer[1024];
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
		<= 0) {
		printf(
			"\nInvalid address/ Address not supported \n");
		return -1;
	}

	RETRY:
	if ((client_fd
		= connect(sock, (struct sockaddr*)&serv_addr,
				sizeof(serv_addr)))
		< 0) {
		printf("\nConnection Failed, attempting reconnection... \n\n");
		goto RETRY;
	}else 
		printf("Connection Established\n\n");
	
	if(strcmp(commands[0],"USER")==0)
		userCommand(sock);
	

	close(client_fd);
	perror("CLOSE THE CONN AND RETURN");
	return 0;
}

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

	RETRY:
	if ((client_fd
		= connect(sock, (struct sockaddr*)&serv_addr,
				sizeof(serv_addr)))
		< 0) {
		printf("\nConnection Failed, attempting reconnection... \n\n");
		// goto RETRY;
	}else 
		printf("Connection Established\n\n");

	send(sock, init, strlen(init), 0);
	valread = read(sock, buffer, 1024);

	if(strcmp(buffer, "invalid request")!=0){
		close(client_fd);
		printf("%d\n", atoi(buffer)-1);
		initFTP(atoi(buffer)-1);
		perror("FINISHED initFTP");

	}
	else
		close(client_fd);
	
	perror("EXITING initconn");
	return 0;
}

int main(int argc, char const* argv[])
{
	// while(1){
	int cmdLen = 0;
	char commandBuf[CMD_SIZE], *rest = NULL, *command = NULL;
	// fflush(stdin);
	// fflush(stdout);
	scanf("%s",commandBuf);
	// fprintf(stderr,"buf is %s",commandBuf);
	// sleep(20);
	// fgets(commandBuf,CMD_SIZE, stdin);

	// if(strcmp(commandBuf,"exit\n")==0)  
	// 	break;

	rest = commandBuf;
	while ((command = strtok_r(rest, " ", &rest))){ 
		command[strcspn(command, "\n")] = 0;
		strcpy(commands[cmdLen], command);
		cmdLen++;
	}

	initializeConn();
	// }
	return 0;
}
