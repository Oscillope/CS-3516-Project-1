#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>
#define TRUE                1
#define FALSE               0
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
#define MAX_URL_LENGTH      2048

int receiveBytes(int sockfd, size_t numbytes, void* saveptr);
int sendBytes(int sockfd, size_t numbytes, void* sendptr);
int writetofile(char* buffer, size_t size);
int sendInt(int sockfd, int toSend);
int sendString(int sockfd, char *toSend);
void *handleclient(void *socketfd);
FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);
int close(int fd);
int processImage(char *str);
uint32_t htonl(uint32_t hostlong);
void exit(int status);

long threadid = 0;
int ratenum, ratetime, timeout;

int main(int argc, char** argv){
    char *port; //Range from 0-65535 so five digits is always sufficient
    int backlog, o;
    pthread_t threads[10];
    port = DEFAULT_PORT;
    //TODO use these numbers
    ratenum = DEFAULT_RATE_NUM;
    ratetime = DEFAULT_RATE_TIME;
    timeout = DEFAULT_TIMEOUT;
    backlog = DEFAULT_BACKLOG;
    while((o = getopt(argc, argv, "p:r:s:u:t:h")) != -1) {
		switch(o) {
			case 'p':
				port = optarg;
				break;
			case 'r':
				ratenum = optarg;
				break;
			case 's':
				ratetime = optarg;
				break;
			case 'u':
				backlog = optarg;
				break;
			case 't':
				timeout = optarg;
				break;
			case 'h':
				printf("Usage: server [-p PORT -r RATE_MSGS -s RATE_TIME -u MAX_USERS -t TIMEOUT -h]\n");
				printf("Defaults: PORT = %s; RATE_MSGS = %d; RATE_TIME = %d; MAX_USERS = %d; TIMEOUT = %d\n", DEFAULT_PORT, DEFAULT_RATE_NUM, DEFAULT_RATE_TIME, DEFAULT_BACKLOG, DEFAULT_TIMEOUT);
				exit(0);
				break;
			case '?':
				if(optopt == 'p' || optopt == 'r' || optopt == 's' || optopt == 'u' || optopt == 't') {
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
					exit(0);
				}
				else {
					fprintf(stderr, "Unknown option -%c.\n", optopt);
					printf("Usage: server -p [PORT] -r [RATE] -s [RATE_TIME] -u [MAX_USERS] -t [TIMEOUT] -h\n");
					exit(0);
				}
				break;
		}
	}
    int socketfd;

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
    printf("Bound socket.\n");
    if(listen(socketfd, backlog)!=0){
        fprintf(stderr, "ERROR: listen() returned with an error\n");
        return 1;
    }

    freeaddrinfo(serverinfo); // free up memory occupied by linked list
    while(1) {
		printf("I'm listening...\n");
		struct sockaddr_storage newaddr;
		socklen_t addrsize;
		addrsize=sizeof(newaddr);
		int acceptedfd;
		acceptedfd = accept((int)socketfd, (struct sockaddr *)&newaddr, &addrsize);
		printf("Accepted a socket.");
		if(pthread_create(&threads[threadid], NULL, handleclient, (void *)acceptedfd)) {
			printf("There was an error creating the thread");
			exit(-1);
		}
		else printf("Created thread ID %ld.\n",(long)threadid);
		threadid++;
	}
    close(socketfd);
    pthread_exit(NULL);
}
void *handleclient(void *sockfd){
	printf("Hello, I'm a thread!\n");
    int timedout = FALSE;
    fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET((int)sockfd, &readfds);
	struct timeval timeoutval;
	timeoutval.tv_sec = 2;
	timeoutval.tv_usec = 0;
    
    while(!timedout){
        select((int)sockfd+1, &readfds, NULL, NULL, &timeoutval);
        if(FD_ISSET((int)sockfd, &readfds)){
	        int imgsize;
            //receive the size of the image
            int rcvstatus = receiveBytes((int)sockfd, sizeof(int), (void *)&imgsize);
            if(!rcvstatus){
                printf("Client closed connection\n");
                timedout=TRUE;
                break;
            }
            printf("Client is sending a file of size %d bytes.\n",imgsize);
            char *imgbuf;
            if(imgsize>=MAX_FILE_SIZE){
                imgsize=MAX_FILE_SIZE;
            }
            imgbuf = (char*)malloc(imgsize);
            //recieve the image
            rcvstatus = receiveBytes((int)sockfd, imgsize, (void *)imgbuf);
            
            //write to temporary file
            writetofile(imgbuf, imgsize);
            printf("Wrote an image of size %d to file\n", imgsize);
            free(imgbuf); //don't need this in memory anymore because we saved it to a file
            
            //TODO process the image (if failed then send failure) and assign url to its variable
            char url[MAX_URL_LENGTH];
            processImage(url);
            //don't need to waste disk space by keeping file
            char remove[MAX_URL_LENGTH];
            sprintf(remove, "rm tmp-%u.png", (unsigned int)pthread_self());
            system(remove);
            printf("Parsed URL: %s\n", url);
            sendInt((int)sockfd, SUCCESS);
            sendString((int)sockfd, url);
        } else {
            printf("Connection timed out.\n");
            timedout=TRUE;
        }
    }
    printf("Closing connection to host.\n");
    threadid--;
    close((int)sockfd); 
    pthread_exit(NULL);
}
int writetofile(char* buffer, size_t size){
    FILE *fp;
    char tmp[MAX_URL_LENGTH];
    sprintf(tmp, "tmp-%u.png", pthread_self());
    printf("The string is now %s\n",tmp);
    char *name = tmp;
    fp=fopen(name, "wb");
    size_t written = fwrite(buffer, sizeof(char), size, fp);
    fclose(fp); //we're done writing to the file
    return written!=size;
}
int receiveBytes(int sockfd, size_t numbytes, void* saveptr){
    int status = recv(sockfd, saveptr, numbytes, 0);
    if(!status){
        return status;
    }
    size_t rcvdbytes = status;
    while((rcvdbytes<numbytes) && (status != -1)){
        status = recv(sockfd, saveptr, numbytes, 0);
        rcvdbytes += status;
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
int receiveInt(int sockfd){
    int toReturn;
    receiveBytes(sockfd, sizeof(int), &toReturn);
    toReturn = ntohl(toReturn);
    return toReturn;
}
int sendInt(int sockfd, int toSend){
    int converted = htonl(toSend);
    return send(sockfd, &converted, sizeof(int), 0);
}
int sendString(int sockfd, char *toSend){
    int length = strlen(toSend);
    return send(sockfd, toSend, length, 0);
}
int processImage(char *str){
	char command[MAX_URL_LENGTH];
	sprintf(command, "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner tmp-%u.png", pthread_self());
    FILE *process = popen(command, "r");
    int i;
    printf("Started processing image\n");
    for(i=0; i<5; i++){
        fgets(str,MAX_URL_LENGTH,process);
    }
    int pclose(FILE *stream); 
    return 0;
}
