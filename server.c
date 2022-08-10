#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#define PORT 8080
#define CMD_SIZE 1024

char user[32];
char *comPort = NULL;
int port;

int lsCommand(char * dirname, char *buffer){
	DIR *dr;
   	struct dirent *en;
   	dr = opendir(dirname); 
   	if (dr) {
      	while ((en = readdir(dr)) != NULL) {
			strcat(buffer, en->d_name);
			strcat(buffer, "\n");
    }
      closedir(dr); 
   }
   return strlen(buffer);
}

void exitHandler(){
	free(comPort);
	printf("Exiting pid %d\n",getpid());
	exit(0);
}

int incPort(){
	char temp[4];
	int currentPort = atoi(comPort);
	port = currentPort;
	currentPort++;
	sprintf(temp, "%d", currentPort);
	strcpy(comPort, temp);
	return 0;
}

int deleCommand(int new_socket){
	int valread;
	char buffer[CMD_SIZE];
	struct stat st = {0};

	strcpy(buffer, "200");
	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);

	if (stat(buffer, &st) == 0){
    	if(remove(buffer)>-1){
			strcpy(buffer, "200");
			send(new_socket, buffer, strlen(buffer), 0);
			memset(&buffer[0], 0, sizeof(buffer));
		}else{
			strcpy(buffer, "400");
			send(new_socket, buffer, strlen(buffer), 0);
			memset(&buffer[0], 0, sizeof(buffer));
		}	
	}else{
		memset(&buffer[0], 0, sizeof(buffer));
		strcpy(buffer, "400");
		send(new_socket, buffer, strlen(buffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));
	}

	return 0;
}

int rmdCommand(int new_socket){
	int valread;
	char buffer[CMD_SIZE];
	struct stat st = {0};

	strcpy(buffer, "200");
	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);

	if (stat(buffer, &st) == 0){
    	if(rmdir(buffer)>-1){
			strcpy(buffer, "200");
			send(new_socket, buffer, strlen(buffer), 0);
			memset(&buffer[0], 0, sizeof(buffer));
		}else{
			strcpy(buffer, "400");
			send(new_socket, buffer, strlen(buffer), 0);
			memset(&buffer[0], 0, sizeof(buffer));
		}	
	}else{
		memset(&buffer[0], 0, sizeof(buffer));
		strcpy(buffer, "400");
		send(new_socket, buffer, strlen(buffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));
	}

	return 0;
}

int mkdCommand(int new_socket){
	int valread;
	char buffer[CMD_SIZE];
	struct stat st = {0};

	strcpy(buffer, "200");
	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);

	if (stat(buffer, &st) == -1){
    	if(mkdir(buffer, 0700)>-1){
			strcpy(buffer, "200");
			send(new_socket, buffer, strlen(buffer), 0);
			memset(&buffer[0], 0, sizeof(buffer));
		}else{
			fprintf(stderr,"error code is %d\n",mkdir(buffer, 0700));
			strcpy(buffer, "400");
			send(new_socket, buffer, strlen(buffer), 0);
			memset(&buffer[0], 0, sizeof(buffer));
		}	
	}else{
		memset(&buffer[0], 0, sizeof(buffer));
		strcpy(buffer, "400");
		send(new_socket, buffer, strlen(buffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));
	}

	return 0;
}

int pwdCommand(int new_socket){
	int valread;
	char buffer[CMD_SIZE];

	getcwd(buffer,sizeof(buffer));
	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));
	
	return 0;
}

int listCommand(int new_socket){
	int valread;
	char buffer[CMD_SIZE];
	char *tempBuffer = malloc(sizeof(char)*CMD_SIZE);

	strcpy(buffer, "200");
	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);

	if(strcmp(buffer,"0")==0){
		lsCommand(".", tempBuffer);
		send(new_socket, tempBuffer, strlen(tempBuffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));
	}
	else{
		strcpy(buffer, "200");
		send(new_socket, buffer, strlen(buffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));
		valread = read(new_socket, buffer, CMD_SIZE);

		lsCommand(buffer, tempBuffer);
		send(new_socket, tempBuffer, strlen(tempBuffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));
	}
	free(tempBuffer);

	return 0;
}

int changeDIR(int new_socket){
	int valread;
	char buffer[CMD_SIZE];
	
	strcpy(buffer, "200");
	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);
	
	if(chdir(buffer)==0){
		printf("changed to %s\n",buffer);
		memset(&buffer[0], 0, sizeof(buffer));
		strcpy(buffer, "200");
	}else{
		memset(&buffer[0], 0, sizeof(buffer));
		strcpy(buffer, "400");
	}

	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));

	return 0;
}

int downloadFile(int new_socket){
	int valread;
	char buffer[CMD_SIZE];
	long int n, size;

	strcpy(buffer, "200");
	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);

	int fd = open(buffer,O_RDONLY);
		if(fd==-1){
			printf("\n The operation was not successful with file %s\n",buffer);
			strcpy(buffer, "400");
			send(new_socket, buffer, strlen(buffer), 0);
			memset(&buffer[0], 0, sizeof(buffer));
		}
		else{
			strcpy(buffer, "200");
			send(new_socket, buffer, strlen(buffer), 0);
			memset(&buffer[0], 0, sizeof(buffer));
			valread = read(new_socket, buffer, CMD_SIZE);
			
			if(strcmp(buffer, "200")==0){
				
				memset(&buffer[0], 0, sizeof(buffer));
				size = lseek(fd,0, SEEK_END);
				lseek(fd, 0, SEEK_SET);
				sprintf(buffer,"%ld",size);
				fprintf(stderr, "size is %ld\n",size);
				send(new_socket, buffer, strlen(buffer), 0);
				memset(&buffer[0], 0, sizeof(buffer));
				valread = read(new_socket, buffer, CMD_SIZE);

				if(strcmp(buffer, "200")==0){

					memset(&buffer[0], 0, sizeof(buffer));
					while((n=read(fd,buffer,CMD_SIZE))>0){
						fprintf(stderr,"sending %ld bytes, sending %ld bytes\n",n,strlen(buffer));
						send(new_socket, buffer, n, 0);
						memset(&buffer[0], 0, sizeof(buffer));
					}

				}

		}
	
	}
	close(fd);
	return 0;
}

int uploadFile(int new_socket){
	int valread;
	long int size,curSize=0;
	char buffer[CMD_SIZE];
	
	strcpy(buffer, "200");
	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);

	int fd = open(buffer,O_WRONLY|O_CREAT,0777);
	if(fd==-1){
		printf("\n The operation was not successful --%s\n",buffer);
		memset(&buffer[0], 0, sizeof(buffer));
		strcpy(buffer, "400");
		send(new_socket, buffer, strlen(buffer), 0);
	}
	else{
		memset(&buffer[0], 0, sizeof(buffer));
		strcpy(buffer, "200");
		send(new_socket, buffer, strlen(buffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));
		valread = read(new_socket, buffer, CMD_SIZE);
		// fprintf(stderr,"buffer is %s bytes\n",buffer);
		size = atoi(buffer);
		memset(&buffer[0], 0, sizeof(buffer));
		strcpy(buffer, "200");
		send(new_socket, buffer, strlen(buffer), 0);
		perror("waiting for file");
		while((valread = read(new_socket, buffer, CMD_SIZE))>0){
			fprintf(stderr,"Read %d bytes\n",valread);
			write(fd, buffer,valread);
			memset(&buffer[0], 0, sizeof(buffer));
			curSize+=valread;
			fprintf(stderr,"Size is %ld and cursize is %ld\n",size,curSize);
			if(curSize==size)
				break;
		}
		perror("done");
		memset(&buffer[0], 0, sizeof(buffer));
		strcpy(buffer, "200");
		send(new_socket, buffer, strlen(buffer), 0);
		
	}

	close(fd);
	return 0;
}

int userCommand(int new_socket){
	int valread;
	char buffer[CMD_SIZE];
	
	strcpy(buffer, "200");
	send(new_socket, buffer, strlen(buffer), 0);

	valread = read(new_socket, buffer, CMD_SIZE);
	strcpy(user, buffer);
	printf("User %s login registered\n",buffer);
	sprintf(buffer,"230 %s logged in",user);
	send(new_socket, buffer, strlen(buffer), 0);
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

	printf("Server listening on %d\n",port);

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
	printf("Accepted connection on %d\n",port);

	valread = read(new_socket, buffer, 1024);
	
	if(strcmp(buffer, "USER")==0){	//Check each command here with strcmp and buffer
		perror("USER COMMAND TRIGGERED");
		userCommand(new_socket);
	}
	else if(strcmp(buffer, "STOR")==0)
		uploadFile(new_socket);
	else if(strcmp(buffer,"RETR")==0)
		downloadFile(new_socket);
	else if(strcmp(buffer,"CWD")==0)
		changeDIR(new_socket);
	else if(strcmp(buffer,"LIST")==0)
		listCommand(new_socket);
	else if(strcmp(buffer,"PWD")==0)
		pwdCommand(new_socket);
	else if(strcmp(buffer,"MKD")==0)
		mkdCommand(new_socket);
	else if(strcmp(buffer,"RMD")==0)
		rmdCommand(new_socket);
	else if(strcmp(buffer,"DELE")==0)
		deleCommand(new_socket);

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
		incPort();
		pid_t pid = fork();
		if(pid==0){
			send(new_socket, comPort, sizeof(comPort), 0);
			close(new_socket);
			shutdown(server_fd, SHUT_RDWR);
			initFTP();
			exit(0);
		}
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

	signal(SIGINT, exitHandler);

	while(1)
		initializeConn();

	
	free(comPort);
	return 0;
}
