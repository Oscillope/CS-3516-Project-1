#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#define DEFAULT_PORT "2012"
#define SERVER_ADDRESS "localhost"
#define MAX_SIZE 4096
#define MAX_URL_LENGTH 2048
#define TRUE 1
#define FALSE 0
int receiveBytes(int sockfd, size_t numbytes, void* saveptr);
int sendBytes(int sockfd, size_t numbytes, void* sendptr);
int receiveString(int sockfd, char *saveptr);

int main(int argc, char** argv){
    char *port, *message, *imgpath; //Range from 0-65535 so five digits is always sufficient
    char *server;
    char data[MAX_SIZE];
    int o, i;
    port = DEFAULT_PORT;
    server = SERVER_ADDRESS;
    imgpath = NULL;
    while((o = getopt(argc, argv, "p:a:h")) != -1) {
		switch(o) {
			case 'p':
				port = optarg;
				break;
			case 'a':
				server = optarg;
				break;
			case 'h':
				printf("Usage: client [-p PORT -a SERVER_ADDRESS -h] IMAGE\n");
				printf("Defaults: PORT = %s; SERVER_ADDRESS = %s\n", DEFAULT_PORT, SERVER_ADDRESS);
				exit(0);
			case '?':
				if(optopt == 'p' || optopt == 'a') {
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
					exit(1);
				}
		}
	}
	for(i = optind; i < argc; i++) {
		//printf("Current optind %d\n", optind);
		if(argv[i])
			imgpath = argv[i];
	}
	if(!imgpath) {
		printf("You must provide a filename.\n");
		printf("Usage: client [-p PORT -a SERVER_ADDRESS -h] IMAGE\n");
		exit(1);
	}
		
    FILE *fp;
    fp=fopen(imgpath, "rb");
    size_t imgsize = fread(data, sizeof(char), MAX_SIZE, fp);
    fclose(fp); //done reading from file
    //printf("Read an image of size %d into memory\n", imgsize);
    message = data;
    struct addrinfo knowninfo;
    struct addrinfo *clientinfo;  // will point to the results
    int socketfd;


    memset(&knowninfo, 0, sizeof knowninfo); // make sure the struct is empty
    knowninfo.ai_family = AF_UNSPEC;
    knowninfo.ai_socktype = SOCK_STREAM;
    knowninfo.ai_flags = AI_PASSIVE;		// Use the host's IP
    knowninfo.ai_addr = server;		// Use localhost for now. TODO: change this to accept an argument
    if(getaddrinfo(server, port, &knowninfo, &clientinfo) != 0) {
		fprintf(stderr, "FATAL: getaddrinfo() returned an error\n");
		return 1;
	}
    //printf("Socket: %u\n",(unsigned int)clientinfo->ai_addr);
	socketfd = socket(clientinfo->ai_family, clientinfo->ai_socktype, clientinfo->ai_protocol);
    if(connect(socketfd, clientinfo->ai_addr, clientinfo->ai_addrlen) != 0) {
        fprintf(stderr, "FATAL: connect() returned an error\n");
        return 1;
    }
    else printf("Connected. Uploading file.\n");
    send(socketfd, (void *)&imgsize, sizeof(size_t), 0);
    size_t sentbytes=0;
    while(sentbytes<imgsize){
        sentbytes += send(socketfd, message, imgsize, 0);
    }
    printf("Sent %d bytes.\n", sentbytes);
    int status;
    char url[MAX_SIZE];
    receiveBytes(socketfd, sizeof(int), (void *)&status);
    receiveString(socketfd, url);
    if(status) printf("An error occurred while uploading the file.\n");
    printf("URL: %s\n", url);
    return 0;
}
int receiveBytes(int sockfd, size_t numbytes, void* saveptr){
    size_t rcvdbytes = 0;
    int status=0;
    while((rcvdbytes<numbytes) && (status != -1)){
        status = recv(sockfd, saveptr, numbytes, 0);
        rcvdbytes += status;
    }
    return rcvdbytes==numbytes;
}
int receiveString(int sockfd, char *saveptr){
    size_t rcvdbytes = 0;
    int bytesread=0;
    int ended = FALSE;
    while((!ended) && (bytesread != -1) && rcvdbytes<MAX_URL_LENGTH){
        bytesread = recv(sockfd, (void*)saveptr, MAX_URL_LENGTH, 0);
        rcvdbytes += bytesread;
        printf("Got %d bytes from server (%d total)\n", bytesread, rcvdbytes);
        ended=(saveptr[rcvdbytes]=='\0');
    }
    return rcvdbytes;
}
int sendBytes(int sockfd, size_t numbytes, void* sendptr){
    size_t sentbytes=0;
    int status=0;
    while((sentbytes<numbytes) && (status!=-1)){
        sentbytes += send(sockfd, sendptr, numbytes, 0);
    }
    return sentbytes==numbytes;
}
