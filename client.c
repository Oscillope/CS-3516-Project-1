#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define DEFAULT_PORT "2012"
int main(int argc, char** argv){
    char *port; //Range from 0-65535 so five digits is always sufficient
    port = DEFAULT_PORT;
    struct addrinfo knowninfo;
    struct addrinfo *servinfo;  // will point to the results

    memset(&knowninfo, 0, sizeof knowninfo); // make sure the struct is empty
    knowninfo.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    knowninfo.ai_socktype = SOCK_STREAM; // TCP stream sockets
    knowninfo.ai_flags = AI_PASSIVE;     // fill in my IP for me
}
