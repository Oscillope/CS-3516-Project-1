#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#define DEFAULT_PORT "2012"
#define SERVER_ADDRESS "localhost"
#define MAX_SIZE 4096
#define MAX_URL_LENGTH 2048
#define TRUE 1
#define FALSE 0
int receiveBytes(int sockfd, size_t numbytes, void* saveptr);
int sendBytes(int sockfd, size_t numbytes, void* sendptr);
int receiveString(int sockfd, char *saveptr);
uint32_t ntohl(uint32_t netlong);
int receiveInt(int sockfd);
void exit(int status);
void checkPort(char *port);

int main(int argc, char** argv){
    char *port, *message; //Range from 0-65535 so five digits is always sufficient
    char *server;
    char data[MAX_SIZE];
    int o, i, status;
    status = 0;
    port = DEFAULT_PORT;
    server = SERVER_ADDRESS;
    while((o = getopt(argc, argv, "p:a:h")) != -1) {
		switch(o) {
			case 'p':
				port = optarg;
				checkPort(port);
				break;
			case 'a':
				server = optarg;
				break;
			case 'h':
				printf("Usage: client [-p PORT -a SERVER_ADDRESS -h] IMAGE\n");
				printf("Defaults: PORT = %s; SERVER_ADDRESS = %s\n", DEFAULT_PORT, SERVER_ADDRESS);
				exit(0);
			case '?':
				if(optopt == 'p' || optopt == 'a')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				exit(1);
		}
	}
	char *imgpath[(argc-optind)];
	memset(imgpath, 0, sizeof(imgpath));
	for(i = optind; i < argc; i++) {
		if(argv[i])
			imgpath[i-optind] = argv[i];
	}
	if(!imgpath[0]) {
		printf("You must provide a filename.\n");
		printf("Usage: client [-p PORT -a SERVER_ADDRESS -h] IMAGE\n");
		exit(1);
	}
    struct addrinfo knowninfo;
    struct addrinfo *clientinfo;  // will point to the results
    int socketfd;
	
    memset(&knowninfo, 0, sizeof knowninfo); // make sure the struct is empty
    knowninfo.ai_family = AF_UNSPEC;
    knowninfo.ai_socktype = SOCK_STREAM;
    knowninfo.ai_flags = AI_PASSIVE;		// Use the host's IP
    knowninfo.ai_addr = (struct sockaddr *)server;		
    if(getaddrinfo(server, port, &knowninfo, &clientinfo) != 0) {
		fprintf(stderr, "FATAL: getaddrinfo() returned an error\n");
		return 1;
	}
	socketfd = socket(clientinfo->ai_family, clientinfo->ai_socktype, clientinfo->ai_protocol);
    if(connect(socketfd, clientinfo->ai_addr, clientinfo->ai_addrlen) != 0) {
        fprintf(stderr, "FATAL: connect() returned an error\n");
        return 1;
    }
    else printf("SUCCESS! Connected. Uploading file(s).\n");
	for(i = 0; i < (argc-optind); i++) {
	    FILE *fp;
	    fp=fopen(imgpath[i], "rb");
	    size_t imgsize = fread(data, sizeof(char), MAX_SIZE, fp);
	    fclose(fp); //done reading from file
	    #ifdef DEBUG
	    printf("Read an image of size %d into memory\n", imgsize);
	    #endif
	    message = data;
	    send(socketfd, (void *)&imgsize, sizeof(size_t), 0);
	    size_t sentbytes=0;
	    while(sentbytes<imgsize){
	        sentbytes += send(socketfd, message, imgsize, 0);
	    }
	    #ifdef DEBUG
	    printf("Sent %d bytes.\n", sentbytes);
	    #endif
	    char url[MAX_SIZE];
	    status=receiveInt(socketfd);
	    receiveString(socketfd, url);
	    switch(status) {
			case 0:	
				printf("URL %d: %s\n", i, url);
				break;
			case 1:
				fprintf(stderr, "Something went wrong! Error code 1. Check to make sure your QR code is valid.\n");
				break;
			case 2:
				fprintf(stderr, "%s\n", url);
				break;
			case 3:
				fprintf(stderr, "%s\n", url);
				break;
		}
	}
    return status;
}
int receiveInt(int sockfd){
    int toReturn;
    receiveBytes(sockfd, sizeof(int), &toReturn);
    toReturn = ntohl(toReturn);
    return toReturn;
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
    int bytesread = 0;
    int size = receiveInt(sockfd);
    while((bytesread != -1) && rcvdbytes<size){
        bytesread = recv(sockfd, (void*)saveptr, MAX_URL_LENGTH, 0);
        rcvdbytes += bytesread;
        #ifdef DEBUG
        printf("Got %d bytes from server (%d total).\n", bytesread, rcvdbytes);
        #endif
    }
    saveptr[rcvdbytes]='\0';
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
void checkPort(char *port) {
	int portInt = atoi(port);
	if((portInt < 2000) || (portInt > 3000)) {
		fprintf(stderr, "Port %d is out of range. Please choose a port between 2000 and 3000.\n", portInt);
		exit(1);
	}
	return;
}
