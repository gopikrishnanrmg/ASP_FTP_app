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
#include <sys/mman.h>
#include <sys/types.h>

#define PORT 8080	//Main server port
#define CMD_SIZE 1024
#define MAX_CLIENTS 100

char buff_200[] = "200", buff_400[] = "400"; //Response codes
char *comPort = NULL; //Command port
int *datPort = NULL;
int cPort, *datPortCnt;	

int dataServer(int *, int *, int *, int *, long int *,int );

int lsCommand(char * dirname, char *buffer){	//Function to list to commands in dirname and copy response to buffer
	DIR *dr;
   	struct dirent *en;
   	dr = opendir(dirname); 
   	if (dr) {
      	while ((en = readdir(dr)) != NULL) {
			strcat(buffer, en->d_name);	//Concatenate the name of the item
			strcat(buffer, "\n");	//Concatenate a newline
    }
      closedir(dr); 
   }
   return strlen(buffer);
}

void exitHandler(){	//Called when user presses SIGINT or SIGSTOP, free the allocated memory
	free(comPort);
	munmap(datPort,sizeof(int)*MAX_CLIENTS);
	munmap(datPortCnt,sizeof(int)*1);

	printf("Exiting pid %d\n",getpid());	//Print the pid of the processes killed
	exit(0);
}

int incComPort(){	//Used to increase the child server's port (starts at 9000, not 8080) 
	char temp[4];
	int currentPort = atoi(comPort);
	cPort = currentPort;
	currentPort++;
	sprintf(temp, "%d", currentPort);
	strcpy(comPort, temp);
	return 0;
}

int createDataPort(int *fd1, int *fd2, int *status, int *rest, int*dataPort, long int *progress, int port){// Used to initialize the 2 pipes needed for communication between dataserver and commandserver, further initiates the dataserver
	int i;
	if (pipe(fd1) < 0)
        return 0;
	if (pipe(fd2) < 0)
        return 0;
	
	if(*datPortCnt==MAX_CLIENTS)	//Check if 100 clients are already connected
		return 0;

	for (i=0;i<MAX_CLIENTS; i++) {	//Check if port is already registered
		if(datPort[i]==port)
			return 0;
	}
	
	for (i=0;i<MAX_CLIENTS; i++) {	//Add port to the list
		if(datPort[i]==-1){
			datPort[i] = port;
			*dataPort = port;
			(*datPortCnt)++;
			break;
		}
	}

	pid_t pid = fork();
	if(pid==0){
		dataServer(fd1,fd2,status,rest,progress,port);
	}else
		return pid;
	
	return 0;
}

int reinCommand(int *status, int new_socket){	//Used to reinitialize the child server
	while(*status==1);	//Block if any transfer is in progress
	send(new_socket,buff_200 , strlen(buff_200), 0);

	return 0;
}

int restCommand(int *rest, int new_socket){	//REST flag is enabled
	send(new_socket,buff_200 , strlen(buff_200), 0);
	*rest=1;

	return 0;
}

int statCommand(int *status, long int * progress, int new_socket){	//Lists the files in the current working directory or directory provided as argument, else shows the number of bytes transferred if transaction is in progress
	int valread;
	char buffer[CMD_SIZE] = {0};
	char *tempBuffer = malloc(sizeof(char)*CMD_SIZE);

	send(new_socket,buff_200 , strlen(buff_200), 0);
	valread = read(new_socket, buffer, CMD_SIZE);
	
	if(*status){	//Show the bytes being transferred
		memset(&buffer[0], 0, sizeof(buffer));
		sprintf(buffer,"Transferred %ld bytes\n",*progress);
		send(new_socket, buffer, strlen(buffer), 0);
	}else{	//	List the items in the given directory
		if(strcmp(buffer,buff_200)==0)
			lsCommand(".", tempBuffer);
		else
			lsCommand(buffer, tempBuffer);
		
		send(new_socket, tempBuffer, strlen(tempBuffer), 0);
	}
	free(tempBuffer);

	return 0;
}

int aborCommand(int *status, int *dataServerPID, int *dataPort, int new_socket){	//Kill the dataserver, if transfer is in progress show abnormal closing warning
	int i;
	if(kill(*dataServerPID, SIGKILL)==0 && *status==0)	//Transfer is not in progress
		send(new_socket,buff_200 , strlen(buff_200), 0);
	else if(kill(*dataServerPID, 0)==0 && *status==1)	//Transfer is in progress
		send(new_socket,buff_400 , strlen(buff_400), 0);
	
	for(i=0;i<MAX_CLIENTS;i++){	//Remove port from the tracker
		if(datPort[i]==*dataPort){
			datPort[i]=-1;
			(*datPortCnt)--;
		}
	}
	return 0;
}

int noopCommand(int new_socket){	//Send reply back
	send(new_socket,buff_200 , strlen(buff_200), 0);

	return 0;
}

int deleCommand(int new_socket){	//Delete the file specified
	int valread;
	char buffer[CMD_SIZE] = {0};
	struct stat st = {0};

	send(new_socket,buff_200 , strlen(buff_200), 0);
	valread = read(new_socket, buffer, CMD_SIZE);

	if (stat(buffer, &st) == 0){	//Check if file exists
    	if(remove(buffer)>-1){	//Check if file was sucessfully deleted
			send(new_socket,buff_200 , strlen(buff_200), 0);
		}else{
			send(new_socket,buff_400 , strlen(buff_400), 0);
		}	
	}else{
		send(new_socket,buff_400 , strlen(buff_400), 0);
	}

	return 0;
}

int rnCommand(int new_socket){	//Renames files using arguments provided by RNFR and RNTO
	int valread;
	char buffer[CMD_SIZE] = {0},file1[CMD_SIZE] = {0},file2[CMD_SIZE] = {0};
	struct stat st = {0};

	send(new_socket,buff_200 , strlen(buff_200), 0);
	valread = read(new_socket, file1, CMD_SIZE);

	if (stat(file1, &st) == 0){	//Check if file to be renamed exists
		send(new_socket,buff_200 , strlen(buff_200), 0);
		valread = read(new_socket, file2, CMD_SIZE);

		if(rename(file1, file2)==0){	//Check if renaming is successful
			printf("renamed %s to %s\n",file1,file2);
			send(new_socket,buff_200 , strlen(buff_200), 0);

		}else{
			send(new_socket,buff_400 , strlen(buff_400), 0);
		}

	}else{
		send(new_socket,buff_400 , strlen(buff_400), 0);
	}

	return 0;
}

int rmdCommand(int new_socket){	//Removes directory
	int valread;
	char buffer[CMD_SIZE] = {0};
	struct stat st = {0};

	send(new_socket,buff_200 , strlen(buff_200), 0);
	valread = read(new_socket, buffer, CMD_SIZE);

	if (stat(buffer, &st) == 0){	//Check if the directory to be removed exists
    	if(rmdir(buffer)>-1){	//Check if removing directory was successful
			send(new_socket,buff_200 , strlen(buff_200), 0);
		}else{
			send(new_socket,buff_400 , strlen(buff_400), 0);
		}	
	}else{
		send(new_socket,buff_400 , strlen(buff_400), 0);
	}

	return 0;
}

int mkdCommand(int new_socket){	//Used to create directory
	int valread;
	char buffer[CMD_SIZE] = {0};
	struct stat st = {0};

	send(new_socket,buff_200 , strlen(buff_200), 0);

	valread = read(new_socket, buffer, CMD_SIZE);
	if (stat(buffer, &st) == -1){	//Check if file already exists
    	if(mkdir(buffer, 0777)>-1){	//Check if creating directory was successful
			send(new_socket,buff_200 , strlen(buff_200), 0);
		}else{
			send(new_socket,buff_400 , strlen(buff_400), 0);
		}	
	}else{
		send(new_socket,buff_400 , strlen(buff_400), 0);
	}

	return 0;
}

int listCommand(int *fd1, int *fd2, int new_socket){// Used to list the files in the current working directory/ the directory passed in the argument. This function communicates with both dataserver and client
	int valread;
	char buffer[CMD_SIZE] = {0};
	char *tempBuffer = malloc(sizeof(char)*CMD_SIZE);

	valread = read(fd2[0], buffer, CMD_SIZE);
	send(new_socket, buffer, strlen(buffer), 0);
	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);
	write(fd1[1], buffer, strlen(buffer));

	if(strcmp(buffer,"0")!=0){	//Check if arguement is passed or not
		valread = read(fd2[0],buffer,CMD_SIZE);
		send(new_socket, buffer, strlen(buffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));
		valread = read(new_socket, buffer, CMD_SIZE);
		write(fd1[1], buffer, strlen(buffer));
	}

	return 0;
}

int changeDIR(int *fd1, int *fd2, int new_socket){// Used to change the current working directory using the directory passed in the argument. This function communicates with both dataserver and client
	int valread;
	char buffer[CMD_SIZE] = {0},buffer1[CMD_SIZE] = {0};
	
	send(new_socket,buff_200 , strlen(buff_200), 0);

	valread = read(new_socket, buffer, CMD_SIZE);
	
	write(fd1[1], buffer, strlen(buffer));
	valread = read(fd2[0], buffer1, CMD_SIZE);

	if(chdir(buffer)==0&&(strcmp(buffer1,buff_200)==0)){// Check if CWD was successful in both child server and data server
		printf("changed to %s\n",buffer);
		send(new_socket,buff_200 , strlen(buff_200), 0);
	}else{
		send(new_socket,buff_400 , strlen(buff_400), 0);
	}

	return 0;
}

int appeCommand(int *fd1, int *fd2, int new_socket){// Used to append data passed in the argument to the file passed in the argument. This function communicates with both dataserver and client 
	int valread;
	char buffer[CMD_SIZE] = {0};

	valread = read(fd2[0], buffer, CMD_SIZE);
	send(new_socket, buffer, strlen(buffer), 0); //send 200 back

	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);
	write(fd1[1], buffer, strlen(buffer));

	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(fd2[0], buffer, CMD_SIZE);

	if(strcmp(buffer,buff_400)==0)	//send 200/400
		send(new_socket, buffer, strlen(buffer), 0);
	else{
		send(new_socket, buffer, strlen(buffer), 0);
		
		memset(&buffer[0], 0, sizeof(buffer));
		valread = read(fd2[0], buffer, CMD_SIZE);
		send(new_socket, buffer, strlen(buffer), 0);
	}
	return 0;	
}

int downloadFile(int *fd1, int *fd2, int new_socket){// Used to download the files passed in the argument if it exists in the current working directory of the server. This function communicates with both dataserver and client
	int valread;
	char buffer[CMD_SIZE] = {0};
	long int n, size;

	valread = read(fd2[0], buffer, CMD_SIZE);
	send(new_socket, buffer, strlen(buffer), 0); //send 200 back

	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE);
	write(fd1[1], buffer, strlen(buffer));
	
	memset(&buffer[0], 0, sizeof(buffer));	// read filename
	valread = read(fd2[0], buffer, CMD_SIZE);

	if(strcmp(buffer,buff_400)==0)	//send 200/400
		send(new_socket, buffer, strlen(buffer), 0);
	else{
		send(new_socket, buffer, strlen(buffer), 0);
		
		memset(&buffer[0], 0, sizeof(buffer));	//client able to create file
		valread = read(new_socket, buffer, CMD_SIZE);
		write(fd1[1], buffer, strlen(buffer));

		if(strcmp(buffer, buff_200)==0){

			memset(&buffer[0], 0, sizeof(buffer)); //size of the file
			valread = read(fd2[0], buffer, CMD_SIZE);
			send(new_socket, buffer, strlen(buffer), 0);

			memset(&buffer[0], 0, sizeof(buffer));	//send 200
			valread = read(new_socket, buffer, CMD_SIZE);
			write(fd1[1], buffer, strlen(buffer));

			memset(&buffer[0], 0, sizeof(buffer)); //read the offset, if rest is enabled, send the value of the bytes already transferred
			valread = read(fd2[0], buffer, CMD_SIZE);
			send(new_socket, buffer, strlen(buffer), 0);

			memset(&buffer[0], 0, sizeof(buffer));	//initiate sending
			valread = read(new_socket, buffer, CMD_SIZE);
			write(fd1[1], buffer, strlen(buffer));

		}
	}
	return 0;
}

int uploadFile(int *fd1, int *fd2, int new_socket){
	int valread;
	long int size,curSize=0;
	char buffer[CMD_SIZE] = {0};

	valread = read(fd2[0], buffer, CMD_SIZE);
	send(new_socket, buffer, strlen(buffer), 0); //send 200 back

	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(new_socket, buffer, CMD_SIZE); // read filename
	write(fd1[1], buffer, strlen(buffer));

	memset(&buffer[0], 0, sizeof(buffer));
	valread = read(fd2[0], buffer, CMD_SIZE); //read 200/400

	if(strcmp(buffer,buff_400)==0)
		send(new_socket, buffer, strlen(buffer), 0);
	else{
		send(new_socket, buffer, strlen(buffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));

		valread = read(new_socket, buffer, CMD_SIZE); //read filesize
		write(fd1[1], buffer, strlen(buffer));
		memset(&buffer[0], 0, sizeof(buffer));
		
		valread = read(fd2[0], buffer, CMD_SIZE); //read 200
		send(new_socket, buffer, strlen(buffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));

		valread = read(new_socket, buffer, CMD_SIZE); //restval
		write(fd1[1], buffer, strlen(buffer));
		memset(&buffer[0], 0, sizeof(buffer));

		valread = read(fd2[0], buffer, CMD_SIZE); //send value
		send(new_socket, buffer, strlen(buffer), 0);
		memset(&buffer[0], 0, sizeof(buffer));

		
		pid_t pid = fork();
		if(pid==0){	//read the response asynchronously, as the transfer is happening on data socket
			valread = read(fd2[0], buffer, CMD_SIZE); //read 200 after done copying file 
			send(new_socket, buffer, strlen(buffer), 0);
			exit(0);
		}
	}	

	return 0;
}

int userCommand(char *user, int new_socket){ //Used to enable user input and registering the user
	int valread;
	char buffer[CMD_SIZE] = {0};

	send(new_socket,buff_200 , strlen(buff_200), 0);

	valread = read(new_socket, buffer, CMD_SIZE);
	strcpy(user, buffer);
	printf("User %s login registered\n",buffer);
	sprintf(buffer,"230 %s logged in",user);
	send(new_socket, buffer, strlen(buffer), 0);
	return 0;
}

int portCommand(int *fd1, int *fd2, int *status, int *rest, int *dataPort, long int *progress, int new_socket){	//Used to open the port given in the arguments part
	int valread, dataServerPID;
	char buffer[CMD_SIZE] = {0};

	send(new_socket,buff_200 , strlen(buff_200), 0);

	valread = read(new_socket, buffer, CMD_SIZE);

	if((dataServerPID=createDataPort(fd1,fd2,status,rest,dataPort,progress,atoi(buffer)))){
		send(new_socket,buff_200 , strlen(buff_200), 0);
		return dataServerPID;
	}else {
		send(new_socket,buff_400 , strlen(buff_400), 0);
	}

	return 0;
}

int dataServer(int *fd1, int *fd2, int *status, int *rest, long int *progress, int port){	//The dataserver which is reponsible for handline the data connection
	long int size,curSize,n;
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[CMD_SIZE] = {0}, commandBuf[CMD_SIZE] = {0};

	close(fd1[1]);
	close(fd2[0]);

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

	printf("Data server listening on %d\n",port);

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
	printf("Data server accepted connection on %d\n",cPort);
	
	while(1){

		valread = read(fd1[0],commandBuf,CMD_SIZE);

		if(strcmp(commandBuf, "LIST")==0){	// If input is LIST command
			char *tempBuffer = malloc(sizeof(char)*CMD_SIZE);
			memset(&tempBuffer[0], 0, sizeof(tempBuffer));

			memset(&commandBuf[0], 0, sizeof(commandBuf));

			write(fd2[1], buff_200, strlen(buff_200));

			valread = read(fd1[0],commandBuf,CMD_SIZE);	//Read the number of args
			
			if(strcmp(commandBuf,"0")==0){
				lsCommand(".", tempBuffer);	//Execute lscommand in the cwd
				send(new_socket, tempBuffer, strlen(tempBuffer), 0);
			}
			else{
				memset(&commandBuf[0], 0, sizeof(commandBuf));

				write(fd2[1], buff_200, strlen(buff_200));
	
				valread = read(fd1[0],commandBuf,CMD_SIZE);	//Read the argument, which is the target directory to execute the list command

				lsCommand(commandBuf, tempBuffer);	//Execute lscommand in the directory specified in the argument
				memset(&commandBuf[0], 0, sizeof(commandBuf));

				send(new_socket, tempBuffer, strlen(tempBuffer), 0);
			}

			free(tempBuffer);

		}

		else if(strcmp(commandBuf, "STOR")==0){	//If the commmand is STOR
			curSize = 0;	
			memset(&commandBuf[0], 0, sizeof(commandBuf));

			write(fd2[1], buff_200, strlen(buff_200));

			valread = read(fd1[0],commandBuf,CMD_SIZE);	//Read filename

			int fd = open(commandBuf,O_WRONLY|O_CREAT,0777);	//Create file to write data into
			if(fd==-1){
				printf("\n The operation was not successful %s\n",commandBuf);
				write(fd2[1], buff_400, strlen(buff_400));
			}
			else{

				memset(&commandBuf[0], 0, sizeof(commandBuf));
				write(fd2[1], buff_200, strlen(buff_200));

				valread = read(fd1[0], commandBuf, CMD_SIZE);	//Read filesize
				size = atoi(commandBuf);

				memset(&commandBuf[0], 0, sizeof(commandBuf));

				write(fd2[1], buff_200, strlen(buff_200));

				valread = read(fd1[0], commandBuf, CMD_SIZE);
				if(strcmp(commandBuf, "restval")==0){
					memset(&commandBuf[0], 0, sizeof(commandBuf));
					if(*status==1&&*rest==1&&*progress!=-1){	//If status of file transfer is incomplete and rest command is issued
						lseek(fd,*progress,SEEK_SET);	//Set to the correct offset, which is no of bytes received before
						curSize=*progress;	//Used to track file transfer progress
						sprintf(commandBuf,"%ld",*progress);
						write(fd2[1], commandBuf, strlen(commandBuf));	//Send back to the client the offset to be set when reading the file
					}else{
						sprintf(commandBuf,"%d",0);	//Send back the offset as 0
						write(fd2[1], commandBuf, strlen(commandBuf));
					}
				}

				*status = 1;
				while((valread = read(new_socket, buffer, CMD_SIZE))>0){	//Read from the client directly via data socket connection
					// fprintf(stderr,"Read %d bytes\n",valread);
					write(fd, buffer,valread);	//Write to file the bytes read
					curSize+=valread;
					*progress = curSize;
					memset(&buffer[0], 0, sizeof(buffer));
					// fprintf(stderr,"Size is %ld and cursize is %ld\n",size,curSize);
					if(curSize==size)	//Upon completion exit
						break;
				}
				*rest = 0;
				*status = 0;
				*progress = -1;
				printf("Done\n");

				write(fd2[1], buff_200, strlen(buff_200));	//Notify transfer was done
				printf("Done receiving data\n");
			}
			close(fd);
		}
		
		else if(strcmp(commandBuf, "RETR")==0){	//If the commmand is RETR
			curSize = 0;
			memset(&commandBuf[0], 0, sizeof(commandBuf));
			write(fd2[1], buff_200, strlen(buff_200));


			valread = read(fd1[0],commandBuf,CMD_SIZE);
			int fd = open(commandBuf,O_RDONLY);
			if(fd==-1){	//If the file does not exist send back 400
				printf("\n The operation was not successful with file %s\n",commandBuf);
				write(fd2[1], buff_400, strlen(buff_400));

			}
			else{
				memset(&commandBuf[0], 0, sizeof(commandBuf));
				write(fd2[1], buff_200, strlen(buff_200));

				valread = read(fd1[0],commandBuf,CMD_SIZE);

				if(strcmp(commandBuf,buff_200)==0){

					memset(&commandBuf[0], 0, sizeof(commandBuf));
					size = lseek(fd,0, SEEK_END);
					lseek(fd, 0, SEEK_SET);
					sprintf(commandBuf,"%ld",size);
					printf( "size is %ld\n",size);
					write(fd2[1], commandBuf, strlen(commandBuf));	//Send the size of the file
					memset(&commandBuf[0], 0, sizeof(commandBuf));

					valread = read(fd1[0],commandBuf,CMD_SIZE);
					if(strcmp(commandBuf,buff_200)==0){
						memset(&commandBuf[0], 0, sizeof(commandBuf));
						if(*status==1&&*rest==1&&*progress!=-1){	//If status of file transfer is incomplete and rest command is issued
							lseek(fd,*progress,SEEK_SET);	//Set to the correct offset, which is no of bytes sent before
							curSize=*progress;	//Used to track file transfer progress
							sprintf(commandBuf,"%ld",*progress);
							write(fd2[1], commandBuf, strlen(commandBuf));	//Send back to the client the offset to be set when reading the file
							
						}else{
							sprintf(commandBuf,"%d",0);	//Send back the offset as 0
							write(fd2[1], commandBuf, strlen(commandBuf));
						}

						memset(&commandBuf[0], 0, sizeof(commandBuf));
						valread = read(fd1[0],commandBuf,CMD_SIZE);
						
						if(strcmp(commandBuf,buff_200)==0){
							*status = 1;
							while((n=read(fd,buffer,CMD_SIZE))>0){	//Read from the file
								// fprintf(stderr,"sending %ld bytes, sending %ld bytes\n",n,strlen(buffer));
								send(new_socket, buffer, n, 0);		//Write to data socket the bytes read
								curSize+=n;
								*progress = curSize;
								memset(&buffer[0], 0, sizeof(buffer));
							}
							*rest = 0;
							*status = 0;
							*progress = -1;
							printf("Done sending data\n");
						}
					}
				}
			}
			close(fd);
		}

		else if(strcmp(commandBuf, "APPE")==0){

			memset(&commandBuf[0], 0, sizeof(commandBuf));
			write(fd2[1], buff_200, strlen(buff_200));

			valread = read(fd1[0],commandBuf,CMD_SIZE);
			int fd = open(commandBuf,O_WRONLY|O_CREAT|O_APPEND,0777);

			if(fd==-1){	//If file is not found
				printf("\n The operation was not successful with file %s\n",commandBuf);
				write(fd2[1], buff_400, strlen(buff_400));
			}
			else{
				lseek(fd,0,SEEK_END);	//Move the pointer to the end of the file
				memset(&commandBuf[0], 0, sizeof(commandBuf));
				write(fd2[1], buff_200, strlen(buff_200));

				valread = read(new_socket, buffer, CMD_SIZE);	//Read the data
				write(fd, buffer,valread);	//Write to the file

				write(fd2[1], buff_200, strlen(buff_200));	//Notify operation was done

			}
			close(fd);
		}

		else if(strcmp(commandBuf, "PWD")==0){
			getcwd(buffer,sizeof(buffer));	//Get the CWD
			send(new_socket, buffer, strlen(buffer), 0);	//Send back to the client
		}

		else if(strcmp(commandBuf, "CWD")==0){
			
			memset(&commandBuf[0], 0, sizeof(commandBuf));
			valread = read(fd1[0],commandBuf,CMD_SIZE);	//Read the argument value
			if(chdir(commandBuf)==0){	//Change the CWD
			write(fd2[1], buff_200, strlen(buff_200));	//Send back the acknowledgment
			}else{
				write(fd2[1], buff_400, strlen(buff_400));
			}
		}
	
	memset(&buffer[0], 0, sizeof(buffer));
	memset(&commandBuf[0], 0, sizeof(commandBuf));

	}
	return 0;
}

int initFTP(){	//Child server for data connection
	int server_fd, new_socket, valread, i;
	struct sockaddr_in address;
	int opt, addrlen = sizeof(address);
	int server,*fd1, *fd2, *status, *dataServerPID, *rest, *dataPort;
	long int *progress;
	char buffer[1024] = {0}, *user;

	user = malloc(sizeof(char)*32);
	fd1 = malloc(sizeof(int)*2);	//Pipe to communicate between child server and dataserver
	fd2 = malloc(sizeof(int)*2);	//Pipe to communicate between child server and dataserver
	dataServerPID = malloc(sizeof(int)*1);
	dataPort = malloc(sizeof(int)*1);

	//Shared memory variables to be used between the child server(control server) and dataserver
	rest = mmap(NULL, sizeof *rest, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	status = mmap(NULL, sizeof *status, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	progress = mmap(NULL, sizeof *progress, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	
	*rest = 0;
	*status = 0;
	*progress = -1;

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
	address.sin_port = htons(cPort);

	if (bind(server_fd, (struct sockaddr*)&address,
			sizeof(address))
		< 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	printf("Server listening on %d\n",cPort);

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
	printf("Accepted connection on %d\n",cPort);

	while (1){
		valread = read(new_socket, buffer, CMD_SIZE);	//Read the command sent and execute corresponding code

		if(strcmp(buffer, "USER")==0)
			userCommand(user,new_socket);
		
		else if(strcmp(buffer, "PORT")==0){
			*dataServerPID = portCommand(fd1,fd2,status,rest,dataPort,progress,new_socket);
			close(fd1[0]);
			close(fd2[1]);
		}
		else if(strcmp(buffer, "STOR")==0){
			write(fd1[1], buffer, strlen(buffer));
			uploadFile(fd1,fd2,new_socket);
		}
		else if(strcmp(buffer,"RETR")==0){
			write(fd1[1], buffer, strlen(buffer));
			downloadFile(fd1,fd2,new_socket);
		}
		else if(strcmp(buffer,"APPE")==0){
			write(fd1[1], buffer, strlen(buffer));
			appeCommand(fd1,fd2,new_socket);
		}
		else if(strcmp(buffer,"CWD")==0){
			write(fd1[1], buffer, strlen(buffer));
			changeDIR(fd1,fd2,new_socket);
		}
		else if(strcmp(buffer,"LIST")==0){
			write(fd1[1], buffer, strlen(buffer));
			listCommand(fd1,fd2,new_socket);
		}
		else if(strcmp(buffer,"PWD")==0)
			write(fd1[1], buffer, strlen(buffer));
		
		else if(strcmp(buffer,"MKD")==0)
			mkdCommand(new_socket);
		else if(strcmp(buffer,"RMD")==0)
			rmdCommand(new_socket);
		else if(strcmp(buffer,"RNFR")==0)
			rnCommand(new_socket);
		else if(strcmp(buffer,"DELE")==0)
			deleCommand(new_socket);
		else if(strcmp(buffer,"NOOP")==0)
			noopCommand(new_socket);
		else if(strcmp(buffer,"ABOR")==0)
			aborCommand(status,dataServerPID,dataPort,new_socket);
		else if(strcmp(buffer,"STAT")==0)
			statCommand(status,progress,new_socket);
		else if(strcmp(buffer,"REST")==0)
			restCommand(rest,new_socket);
		else if(strcmp(buffer,"QUIT")==0)
			break;
		else if(strcmp(buffer,"REIN")==0){
			reinCommand(status,new_socket);
			printf("Server shutting down\n");
			kill(*dataServerPID, SIGKILL);	//kill the dataserver
			for(i=0;i<MAX_CLIENTS;i++){	//Remove port from the tracker
				if(datPort[i]==*dataPort){
					datPort[i]=-1;
					(*datPortCnt)--;
				}
			}
			free(user);
			free(fd1);
			free(fd2);
			free(dataServerPID);
			free(dataPort);
			munmap(rest, sizeof *rest);
			munmap(status, sizeof *status);
			munmap(progress, sizeof *progress);
			close(new_socket);
			shutdown(server_fd, SHUT_RDWR);
			initFTP(); 
		}
	memset(&buffer[0], 0, sizeof(buffer));
	}

	printf("Server shutting down\n");
	kill(*dataServerPID, SIGKILL);
	for(i=0;i<MAX_CLIENTS;i++){
		if(datPort[i]==*dataPort){
			datPort[i]=-1;
			(*datPortCnt)--;
		}
	}
	free(user);
	free(fd1);
	free(fd2);
	free(dataServerPID);
	free(dataPort);
	munmap(rest, sizeof *rest);
	munmap(status, sizeof *status);
	munmap(progress, sizeof *progress);
	close(new_socket);
	shutdown(server_fd, SHUT_RDWR);
	close(new_socket);
	shutdown(server_fd, SHUT_RDWR);

	return 0;
}

int initializeConn(){	//Main server at 8080
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[CMD_SIZE] = { 0 };
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
	valread = read(new_socket, buffer, CMD_SIZE);
	if(strcmp(buffer,"init")==0){
		incComPort();
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
	if(argc==3 && strcmp(argv[1],"-d")==0)	//set the working dir to the argument value
		chdir(argv[2]);
		
	comPort = malloc(sizeof(char)*4);
	strcpy(comPort,"9000");

	datPort = mmap(NULL, sizeof(int)*MAX_CLIENTS, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	memset(&datPort[0],-1,sizeof(datPort));
	datPortCnt = mmap(NULL, sizeof(int)*1, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	signal(SIGINT, exitHandler);
	signal(SIGSTOP, exitHandler);

	while(1)
		initializeConn();
	
	return 0;
}
