#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define DEFAULT_PORT "2012"
#define SERVER_ADDRESS "localhost"
#define MAX_SIZE 4096
int receiveBytes(int sockfd, size_t numbytes, void* saveptr);
int sendBytes(int sockfd, size_t numbytes, void* sendptr);

int main(int argc, char** argv){
    char *port, *server, *message, *imgpath; //Range from 0-65535 so five digits is always sufficient
    char data[MAX_SIZE];
    imgpath = "testfiles/wpi.png";
    FILE *fp;
    fp=fopen(imgpath, "rb");
    size_t imgsize = fread(data, sizeof(char), MAX_SIZE, fp);
    fclose(fp); //done reading from file
    printf("Read an image of size %d into memory\n", imgsize);
    port = DEFAULT_PORT;
    server = SERVER_ADDRESS;
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
    printf("%u",clientinfo->ai_addr);
	socketfd = socket(clientinfo->ai_family, clientinfo->ai_socktype, clientinfo->ai_protocol);
    /*if(bind(socketfd, clientinfo->ai_addr, clientinfo->ai_addrlen) != 0) {
        fprintf(stderr, "FATAL: bind() returned an error\n");
        return 1;
    }*/
    if(connect(socketfd, clientinfo->ai_addr, clientinfo->ai_addrlen) != 0) {
        fprintf(stderr, "FATAL: connect() returned an error\n");
        return 1;
    }
    else printf("SUCCESS! Connected. Uploading secret message.\n");
    send(socketfd, (void *)&imgsize, sizeof(size_t), 0);
    size_t sentbytes=0;
    while(sentbytes<imgsize){
        sentbytes += send(socketfd, message, imgsize, 0);
    }
    printf("Sent %d bytes.\n", sentbytes);
    int status;
    int rcvd = recv(socketfd, (void *)&status, sizeof(int), 0);
    printf("Success? %d\n", status);
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
int sendBytes(int sockfd, size_t numbytes, void* sendptr){
    size_t sentbytes=0;
    int status=0;
    while((sentbytes<numbytes) && (status!=-1)){
        sentbytes += send(sockfd, sendptr, numbytes, 0);
    }
    return sentbytes==numbytes;
}
