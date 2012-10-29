#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define DEFAULT_PORT "2012"
#define SERVER_ADDRESS "localhost"

int main(int argc, char** argv){
    char *port, *server; //Range from 0-65535 so five digits is always sufficient
    port = DEFAULT_PORT;
    server = SERVER_ADDRESS;
    int yes=1;
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
    else printf("SUCCESS! Connected.\n");
    
}
