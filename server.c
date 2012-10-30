#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#define DEFAULT_PORT        "2012"
#define DEFAULT_RATE_NUM    3
#define DEFAULT_RATE_TIME   60
#define DEFAULT_TIMEOUT     80
#define DEFAULT_BACKLOG     20
#define MAX_FILE_SIZE       4194304 //4MB
#define SUCCESS             0
#define FAILURE             1
#define TIMEOUT             2
#define RATE_LIMIT_EXCEEDED 3
int main(int argc, char** argv){
    char *port; //Range from 0-65535 so five digits is always sufficient
    int ratenum, ratetime, timeout, backlog;
    //TODO handle arguments;
    //if not specified, set defaults
    port = DEFAULT_PORT;
    ratenum = DEFAULT_RATE_NUM;
    ratetime = DEFAULT_RATE_TIME;
    timeout = DEFAULT_TIMEOUT;
    backlog = DEFAULT_BACKLOG;
    int socketfd, acceptedfd;
    struct sockaddr_storage newaddr;
    socklen_t addrsize;
    struct addrinfo knowninfo, *serverinfo;
    //We wouldn't want crazy stack garbage ruining our sockets
    memset(&knowninfo, 0, sizeof(knowninfo));
    knowninfo.ai_family=AF_UNSPEC;
    knowninfo.ai_socktype=SOCK_STREAM;
    knowninfo.ai_flags=AI_PASSIVE;
    if(getaddrinfo(NULL, port, &knowninfo, &serverinfo) != 0){
        fprintf(stderr, "ERROR: getaddrinfo() returned with an error\n");
        return 1;
    }
    socketfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
    if(bind(socketfd, serverinfo->ai_addr, serverinfo->ai_addrlen)!=0){
        fprintf(stderr, "ERROR: bind() returned with an error\n");
        return 1;
    }
    if(listen(socketfd, backlog)!=0){
        fprintf(stderr, "ERROR: listen() returned with an error\n");
        return 1;
    }
    freeaddrinfo(serverinfo); // free up memory occupied by linked list
    //TODO determine if we actually want to accept the connection
    //accept incoming connection
    //TODO pop up a thread to handle the connection
    addrsize=sizeof(newaddr);
    acceptedfd = accept(socketfd, (struct sockaddr *)&newaddr, &addrsize);
    int imgsize;
    int rcvstatus = recv(acceptedfd, (void *)&imgsize, sizeof(int), 0);
    printf("Client is sending a file of size %d bytes\n",imgsize);
    char *imgbuf, *url;
    if(imgsize>=MAX_FILE_SIZE){
        imgsize=MAX_FILE_SIZE;
    }
    imgbuf = (char*)malloc(imgsize);
    rcvstatus = recv(acceptedfd, (void *)imgbuf, imgsize, 0);
    printf("%s\n", imgbuf);
    if(rcvstatus){
        sendInt(acceptedfd, FAILURE);
        close(acceptedfd);
        //exit the thread
        return 1;
    }
    
    //TODO process the image (if failed then send failure) and assign url to its variable
    sendInt(acceptedfd, SUCCESS);
    sendString(acceptedfd, url);
    close(acceptedfd);
}
int sendInt(int sockfd, int toSend){
    return send(sockfd, &toSend, sizeof(toSend), 0);
}
int sendString(int sockfd, char *toSend){
    int length = strlen(toSend);
    return send(sockfd, toSend, length, 0);
}
