#include <arpa/inet.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 8080
#define CMD_SIZE 1024

char commands[CMD_SIZE][CMD_SIZE], buff_200[]="200", buff_400[]="400";
int cmdLen, *dataSock = NULL, *dataClient_fd = NULL, *dataPort = NULL;
int notAuth, dataDisConn;

int initializeConn();

void exitHandler(){	//Called when user presses SIGINT or SIGSTOP, free the allocated memory
	free(dataSock);
	free(dataClient_fd);
	free(dataPort);

	printf("Exiting pid %d\n",getpid());
	exit(0);
}

//Function used to reinitialise the entire connection
int reinCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "REIN", strlen("REIN"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0){
		printf("Reinitializing..\n");
		return 0;
	}else
		printf("Error in reinitializing\n");
	
	return 1;
}

//Function used to quit the server
int quitCommand(int sock){
	int valread;
	send(sock, "QUIT", strlen("QUIT"), 0);
	return 0;
}

//Function used to resume file transfers happened before
int restCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "REST", strlen("REST"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0){
		printf("REST command successful\n");
	}else{
		printf("Rest command failed\n");
	}
	return 0;
}

//Function used to state of the transaction or behave like an ls command
int statCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "STAT", strlen("STAT"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0){
		memset(&buffer[0], 0, sizeof(buffer));
		//check if any parameters passed
		if(cmdLen>1)
			send(sock, commands[1], strlen(commands[1]), 0);
		else
		 	send(sock, buff_200, strlen(buff_200), 0);
		
		valread = read(sock, buffer, CMD_SIZE);
		printf("\n%s\n",buffer);
	}else
		printf("Error in showing stat\n");
	

	return 0;
}

//Function used to abort the tasks taking place
int aborCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "ABOR", strlen("ABOR"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0){
		printf("ABOR command successful\n");
		dataDisConn=1;
	}
	else{
		printf("ABOR command successful, but abnormal closing\n");
		dataDisConn=1;
	}

	return 0;
}

//Function used to retrieve the acknowledgement from the server
int noopCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "NOOP", strlen("NOOP"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0)
		printf("Server sent OK\n");
	else
	 	printf("Server not responding\n");

	return 0;
}

//Function used to delete the files
int deleCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "DELE", strlen("DELE"), 0);

	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0){
		memset(&buffer[0], 0, sizeof(buffer));
		send(sock, commands[1], strlen(commands[1]), 0);	//Send file name
		valread = read(sock, buffer, CMD_SIZE);
		
		if(strcmp(buffer,buff_200)==0)
			printf("File %s deletion successful\n",commands[1]);
		else
			printf("File %s deletion failed\n",commands[1]);
	}else 
		printf("Error deleting files\n");

	return 0;
}

//Function used to rename the files
int RNcommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "RNFR", strlen("RNFR"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0){
		memset(&buffer[0], 0, sizeof(buffer));
		send(sock, commands[1], strlen(commands[1]), 0);	//Send file name

		valread = read(sock, buffer, CMD_SIZE);

		if(strcmp(buffer,buff_200)==0){
			memset(&buffer[0], 0, sizeof(buffer));
			send(sock, commands[3], strlen(commands[3]), 0);	//Send new file name
			valread = read(sock, buffer, CMD_SIZE);

			if(strcmp(buffer,buff_200)==0)
				printf("Successfully renamed %s to %s\n",commands[1],commands[3]);
			else 
				printf("Failed to rename %s to %s\n",commands[1],commands[3]);
		}else 
			printf("Error renaming files, check if file exists\n");

	}else 
		printf("Error renaming files\n");

	return 0;
}

//Function to delete a directory
int rmdCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "RMD", strlen("RMD"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0){
		memset(&buffer[0], 0, sizeof(buffer));
		send(sock, commands[1], strlen(commands[1]), 0);	//Send name of dir to be deleted
		valread = read(sock, buffer, CMD_SIZE);
		
		if(strcmp(buffer,buff_200)==0)
			printf("Directory %s deletion successful\n",commands[1]);
		else
			printf("Directory %s deletion failed\n",commands[1]);
	}else 
		printf("Error removing directory\n");

	return 0;
}

//Function to make a new directory in the current working directory
int mkdCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "MKD", strlen("MKD"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0){
		memset(&buffer[0], 0, sizeof(buffer));
		//sending the name of the directory
		send(sock, commands[1], strlen(commands[1]), 0);	//Send name of dir to be created
		valread = read(sock, buffer, CMD_SIZE);
		
		if(strcmp(buffer,buff_200)==0)
			printf("Directory %s creation succesful\n",commands[1]);
		else
			printf("Directory %s creation failed\n",commands[1]);
	}
	else 
		printf("Error creating directory\n");

	return 0;
}

//Function to printing the current working directory
int pwdCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "PWD", strlen("PWD"), 0);
	valread = read(*dataSock, buffer, CMD_SIZE);
	printf("Current working directory is\n%s\n",buffer);

	return 0;
}

//Function to list directories and files present in the current working directory
//ls
int listCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "LIST", strlen("LIST"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer,buff_200)==0){
		memset(&buffer[0], 0, sizeof(buffer));
		//checks if the command has an argument
		if(cmdLen<2){	//If no argument
			send(sock, "0", strlen("0"), 0);
			valread = read(*dataSock, buffer, CMD_SIZE);
		}else{	//If there is an argument
			send(sock, "1", strlen("1"), 0);		
			valread = read(sock, buffer, CMD_SIZE);
			
			if(strcmp(buffer,buff_200)==0){
				memset(&buffer[0], 0, sizeof(buffer));
				send(sock, commands[1], strlen(commands[1]), 0);	//send the argument
				valread = read(*dataSock, buffer, CMD_SIZE);
			}else 
				printf("Error sending multiple directory argument\n");
		}
			printf("\n%s\n",buffer);
	}else 
		printf("Error listing directory\n");

	return 0;
}

//Function to change the current directory
int changeDIR(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};
	
	send(sock, "CWD", strlen("CWD"), 0);
	valread = read(sock, buffer, CMD_SIZE);
	
	if(strcmp(buffer, buff_200)==0){
		memset(&buffer[0], 0, sizeof(buffer));
		//sending the directiory name

		send(sock, commands[1], strlen(commands[1]), 0);
		valread = read(sock, buffer, CMD_SIZE);

		if(strcmp(buffer, buff_200)==0)
			printf("Changed to %s\n",commands[1]);
		else
			printf("Unable to change to %s\n",commands[1]);
		
	}else 
		printf("Error changing directory\n");

	return 0;
}

//Function to append a particular data to file
int appeCommand(int sock){
	int valread, i;
	char buffer[CMD_SIZE] = {0};
	long int n, size;

	send(sock, "APPE", strlen("APPE"), 0);
	valread = read(sock, buffer, CMD_SIZE);	

	if(strcmp(buffer, buff_200)==0){

		memset(&buffer[0], 0, sizeof(buffer));
		//sends the file name
		send(sock, commands[1], strlen(commands[1]), 0);
		valread = read(sock, buffer, CMD_SIZE);

		if(strcmp(buffer, buff_200)==0){
			memset(&buffer[0], 0, sizeof(buffer));
			//concatenates the data to be sent to append in the file
			for(i=2;i<cmdLen;i++){
				strcat(buffer,commands[i]);
				if(i!=cmdLen-1)
					strcat(buffer," ");
			}

			pid_t pid = fork();
			if(pid==0){
				//sending the data to append to the server
				send(*dataSock, buffer, strlen(buffer), 0);
				memset(&buffer[0], 0, sizeof(buffer));
				valread = read(sock, buffer, CMD_SIZE);
				
				if(strcmp(buffer, buff_200)==0)
					printf("The data has been appened\n");
				else
					printf("The data has not been appened\n");
				exit(0);
			}
		}else
			printf("No file found (remote)\n");
	}else
		printf("Error appending file\n");

	return 0;	
}

//Function for downloading the file
int downloadFile(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};
	long int n, size, curSize=0;

	send(sock, "RETR", strlen("RETR"), 0);
	valread = read(sock, buffer, CMD_SIZE);
	
	if(strcmp(buffer, buff_200)==0){
		memset(&buffer[0], 0, sizeof(buffer));

		//sends the file name
		send(sock, commands[1], strlen(commands[1]), 0);
		valread = read(sock, buffer, CMD_SIZE);

		//getting acknowledgment
		if(strcmp(buffer, buff_200)==0){
			memset(&buffer[0], 0, sizeof(buffer));
			int fd = open(commands[1],O_WRONLY|O_CREAT,0777);
			if(fd==-1)
				printf("\n The operation was not successful with file %s\n",commands[1]);
			else{
				send(sock, buff_200, strlen(buff_200), 0);

				//retrieve the filesize
				valread = read(sock, buffer, CMD_SIZE);
				size = atoi(buffer);
				memset(&buffer[0], 0, sizeof(buffer));

				send(sock, buff_200, strlen(buff_200), 0);

				//retrieve the offset
				valread = read(sock, buffer, CMD_SIZE);
				lseek(fd,atoi(buffer),SEEK_SET);
				curSize=atoi(buffer);

				if(atoi(buffer))
					printf("Resuming at %s\n",buffer);

				memset(&buffer[0], 0, sizeof(buffer));
				send(sock, buff_200, strlen(buff_200), 0);

				pid_t pid = fork();
				//child processes asynchronously recieves the data
				if(pid==0){
					while((valread = read(*dataSock, buffer, CMD_SIZE))>0){
						// fprintf(stderr,"Read %d bytes\n",valread);
						write(fd, buffer,valread);
						memset(&buffer[0], 0, sizeof(buffer));
						curSize+=valread;
						// fprintf(stderr,"Size is %ld and cursize is %ld\n",size,curSize);
						if(curSize==size){
							printf("Done downloading file\n");
							break;
						}
					}
					close(fd);
					exit(0);
				}
				close(fd);
			}
		}else 
			printf("No file found (remote)\n");
		
	}else
		printf("Error downloading file\n");
	
	return 0;
}

//Function to upload a file
int uploadFile(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};
	long int n, size;
	int fd = open(commands[1],O_RDONLY);
	if(fd==-1)
		printf("\n The operation was not successful with file %s\n",commands[1]);
	else{
		send(sock, "STOR", strlen("STOR"), 0);

		valread = read(sock, buffer, CMD_SIZE);

		if(strcmp(buffer, buff_200)==0){

			memset(&buffer[0], 0, sizeof(buffer));
			//sending the filename
			send(sock, commands[1], strlen(commands[1]), 0);
			valread = read(sock, buffer, CMD_SIZE);

			if(strcmp(buffer, buff_200)==0){

				memset(&buffer[0], 0, sizeof(buffer));
				//measuring an sending the filesize
				size = lseek(fd,0, SEEK_END);
				sprintf(buffer,"%ld",size);
				printf("size is %ld\n",size);
				send(sock, buffer, strlen(buffer), 0);
				memset(&buffer[0], 0, sizeof(buffer));
				valread = read(sock, buffer, CMD_SIZE);
				
				memset(&buffer[0], 0, sizeof(buffer));
				send(sock, "restval", strlen("restval"), 0);
				valread = read(sock, buffer, CMD_SIZE);
				lseek(fd, atoi(buffer), SEEK_SET);
				
				if(atoi(buffer))
					printf("Resuming at %s\n",buffer);

				pid_t pid = fork();//data transfer can be done asynchronously

				//child procees keeps sending the data
				if(pid==0){
					memset(&buffer[0], 0, sizeof(buffer));
					while((n=read(fd,buffer,CMD_SIZE))>0){
						// fprintf(stderr,"sending %ld bytes, sending %ld bytes\n",n,strlen(buffer));
						send(*dataSock, buffer, n, 0);
						memset(&buffer[0], 0, sizeof(buffer));
					}
					memset(&buffer[0], 0, sizeof(buffer));
					valread = read(sock, buffer, CMD_SIZE);

					if(strcmp(buffer, buff_200)==0)
						printf("Done uploading file\n");
					
					close(fd);
					exit(0);
				}
			}else 
				printf("Error at file creation (remote)\n");
			
		}else
			printf("Error uploading file\n");

	}
	close(fd);
	return 0;
}

//Function for authenticating the user
int userCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};
	char *login = getlogin();

	send(sock, "USER", strlen("USER"), 0);

	valread = read(sock, buffer, CMD_SIZE);
		if(strcmp(buffer, buff_200)==0){
			memset(&buffer[0],0,sizeof(buffer));
			send(sock, login, strlen(login), 0);
			valread = read(sock, buffer, CMD_SIZE);
			notAuth=0;
			printf("%s \n",buffer);
		}else
			printf("Error registering the user %s\n",login);
	
	return 0;
}

//For opening the data port - connects to the data connection channel of the server
int portCommand(int sock){
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(sock, "PORT", strlen("PORT"), 0);
	valread = read(sock, buffer, CMD_SIZE);

	if(strcmp(buffer, buff_200)==0){
		memset(&buffer[0], 0, sizeof(buffer));
		send(sock, commands[1], strlen(commands[1]), 0);
		valread = read(sock, buffer, CMD_SIZE);

		if(strcmp(buffer, buff_200)==0){
			dataClient_fd = malloc(sizeof(int)*1);
			dataSock = malloc(sizeof(int)*1);
			dataPort = malloc(sizeof(int)*1);
			*dataSock = 0;
			*dataPort = atoi(commands[1]);

			//making another socket connection
			struct sockaddr_in serv_addr;
			if ((*dataSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				printf("\n Socket creation error \n");
				return -1;
			}

			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(*dataPort);
			
			if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<= 0) {
				printf("\nInvalid address/ Address not supported \n");
				return -1;
			}

			RETRY:
			if ((*dataClient_fd = connect(*dataSock, (struct sockaddr*)&serv_addr,sizeof(serv_addr)))< 0) {
				printf("\nConnection Failed, attempting reconnection... \n\n");
				goto RETRY;
			}else 
				printf("Connection Established\n\n");
			
			printf("The port %s was opened for data transfer\n",commands[1]);
			dataDisConn = 0;
			return 0;
		}
		else
			printf("The port %s was not opened for data transfer, port could be bound\n",commands[1]);
	}else 
		printf("The port %s was not opened for data transfer\n",commands[1]);	

	return 0;
}

int initFTP(int port){

	//for authetication- to make USER as first command
	notAuth=1;

	//for check the opening of data connection                                                          
	dataDisConn=1;

	int sock = 0, valread, client_fd, status;
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
	
	//Used for user to run the commands
	while(1){

	INIT:
		cmdLen = 0;
		char commandBuf[CMD_SIZE], *rest = NULL, *command = NULL;
	RNFR: //read the command from the user
		printf("\nEnter the command:\n\n");
		fgets(commandBuf,CMD_SIZE, stdin);
		
		// tokenization of commands 
		rest = commandBuf;
		while ((command = strtok_r(rest, " ", &rest))){ 
			command[strcspn(command, "\n")] = 0;
			strcpy(commands[cmdLen], command);
			cmdLen++;
		}

		/*checking what command is sent by the user*/

		//Until you authenticate , it will run and then proceed
		if(strcmp(commands[0],"USER")!=0&&notAuth){
			printf("You need to authenticate yourself\n");
			goto INIT;
		}

		//Unless a port is open, user cannot move further
		if(((strcmp(commands[0],"STOR")==0)||(strcmp(commands[0],"RETR")==0)||(strcmp(commands[0],"APPE")==0)||(strcmp(commands[0],"CWD")==0)||(strcmp(commands[0],"CDUP")==0)||(strcmp(commands[0],"LIST")==0)||(strcmp(commands[0],"PWD")==0)||(strcmp(commands[0],"REST")==0)||(strcmp(commands[0],"STOR")==0)||(strcmp(commands[0],"STOR")==0)||(strcmp(commands[0],"STOR")==0))&&dataDisConn){
			printf("You need to open a port\n");
			goto INIT;
		}

		if(((strcmp(commands[0],"STOR")==0)||(strcmp(commands[0],"RETR")==0)||(strcmp(commands[0],"APPE")==0)||(strcmp(commands[0],"CWD")==0)||(strcmp(commands[0],"CDUP")==0)||(strcmp(commands[0],"LIST")==0)||(strcmp(commands[0],"PWD")==0)||(strcmp(commands[0],"REST")==0)||(strcmp(commands[0],"STOR")==0)||(strcmp(commands[0],"STOR")==0)||(strcmp(commands[0],"STOR")==0)))
			wait(&status);


		if(strcmp(commands[0],"USER")==0)
			userCommand(sock);
		else if(strcmp(commands[0],"PORT")==0)
			portCommand(sock);
		else if(strcmp(commands[0],"STOR")==0)
			uploadFile(sock);
		else if(strcmp(commands[0],"RETR")==0)
			downloadFile(sock);
		else if(strcmp(commands[0],"APPE")==0)
			appeCommand(sock);
		else if(strcmp(commands[0],"CWD")==0)
			changeDIR(sock);
		else if(strcmp(commands[0],"CDUP")==0){
			memset(&commands[0][0],0,sizeof(commandBuf[0]));
			memset(&commands[1][0],0,sizeof(commandBuf[1]));
			cmdLen=2;
			strcpy(commands[0],"CWD");
			strcpy(commands[1],"..");
			//change to the parent directory
			changeDIR(sock);
		}
		else if(strcmp(commands[0],"LIST")==0)
			listCommand(sock);
		else if(strcmp(commands[0],"PWD")==0)
			pwdCommand(sock);
		else if(strcmp(commands[0],"MKD")==0)
			mkdCommand(sock);
		else if(strcmp(commands[0],"RMD")==0)
			rmdCommand(sock);
		else if(strcmp(commands[0],"RNFR")==0)
			if(cmdLen<4)
				goto RNFR;
			else
			 	RNcommand(sock);
		else if(strcmp(commands[0],"RNTO")==0){
			printf("Must be preceeded by RNFR\n");
			continue;
		}
		else if(strcmp(commands[0],"DELE")==0)
			deleCommand(sock);
		else if(strcmp(commands[0],"NOOP")==0)
			noopCommand(sock);
		else if(strcmp(commands[0],"ABOR")==0)
			aborCommand(sock);
		else if(strcmp(commands[0],"STAT")==0)
			statCommand(sock);
		else if(strcmp(commands[0],"REST")==0)
			restCommand(sock);
		else if(strcmp(commands[0],"QUIT")==0){
			quitCommand(sock);
			break;
		}
		else if(strcmp(commands[0],"REIN")==0){
			if(!reinCommand(sock)){
				initFTP(port);
			}
		}
	}

	close(client_fd);
	return 0;
}

//initialize connection for client to server
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
		goto RETRY;
	}else 
		printf("Connection Established\n\n");

	send(sock, init, strlen(init), 0);
	//client recieves the current port
	valread = read(sock, buffer, 1024);

	if(strcmp(buffer, "invalid request")!=0)
		initFTP(atoi(buffer)-1);
	else
		printf("error with: %s\n", buffer);
	return 0;
}

int main(int argc, char const* argv[])
{
	//calls exit handler when ctrl-c or ctrl-z pressed
	signal(SIGINT,exitHandler);
	signal(SIGSTOP,exitHandler);

	initializeConn();
	return 0;
}
