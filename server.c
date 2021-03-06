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
#define DEFAULT_MAX_USERS   3
#define MAX_FILE_SIZE       4194304 //4MB
#define SUCCESS             0
#define FAILURE             1
#define TIMEOUT             2
#define RATE_LIMIT_EXCEEDED 3
#define USER_LIMIT          4
#define MAX_URL_LENGTH      2048

int receiveBytes(int sockfd, size_t numbytes, void* saveptr);
int sendBytes(int sockfd, size_t numbytes, void* sendptr);
int writetofile(char* buffer, size_t size, int counter);
int sendInt(int sockfd, int toSend);
int sendString(int sockfd, char *toSend);
void *handleclient(void *socketfd);
FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);
int close(int fd);
int processImage(char *str, int counter);
int pthread_yield(void);
uint32_t htonl(uint32_t hostlong);
void exit(int status);
void checkPort(char *port);
void writetolog(char *message, char addr[14]);
struct to_thread {
	int fd;
	char *ip;
};
struct tm *thetime;

long numclients = 0;
int ratenum, ratetime, timeout;
pthread_mutex_t logmutex;

int main(int argc, char** argv){
    int socketfd;
    char *port; //Range from 0-65535 so five digits is always sufficient
    int maxusers, o;
    pthread_mutex_init(&logmutex, NULL);
    port = DEFAULT_PORT;
    //TODO use these numbers
    ratenum = DEFAULT_RATE_NUM;
    ratetime = DEFAULT_RATE_TIME;
    timeout = DEFAULT_TIMEOUT;
    maxusers = DEFAULT_MAX_USERS;
    while((o = getopt(argc, argv, "p:r:s:u:t:h")) != -1) {
		switch(o) {
			case 'p':
				port = optarg;
				checkPort(port);
				break;
			case 'r':
				ratenum = atoi(optarg);
				break;
			case 's':
				ratetime = atoi(optarg);
				break;
			case 'u':
				maxusers = atoi(optarg);
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			case 'h':
				printf("Usage: server [-p PORT -r RATE_MSGS -s RATE_TIME -u MAX_USERS -t TIMEOUT -h]\n");
				printf("Defaults: PORT = %s; RATE_MSGS = %d; RATE_TIME = %d; MAX_USERS = %d; TIMEOUT = %d\n", DEFAULT_PORT, DEFAULT_RATE_NUM, DEFAULT_RATE_TIME, DEFAULT_MAX_USERS, DEFAULT_TIMEOUT);
				exit(0);
				break;
			case '?':
				if(optopt == 'p' || optopt == 'r' || optopt == 's' || optopt == 'u' || optopt == 't') {
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
					exit(0);
				}
				else {
					fprintf(stderr, "Unknown option -%c.\n", optopt);
					printf("Usage: server -p [PORT] -r [RATE] -s [RATE_TIME] -u [maxusers] -t [TIMEOUT] -h\n");
					exit(0);
				}
				break;
		}
	}
	pthread_t threads[maxusers];
	struct to_thread toThreads[maxusers];
    struct addrinfo knowninfo, *serverinfo;
    //We wouldn't want crazy stack garbage ruining our sockets
    memset(&knowninfo, 0, sizeof(knowninfo));
    knowninfo.ai_family=AF_UNSPEC;
    knowninfo.ai_socktype=SOCK_STREAM;
    knowninfo.ai_flags=AI_PASSIVE;
    if(getaddrinfo(NULL, port, &knowninfo, &serverinfo) != 0){
        fprintf(stderr, "ERROR: getaddrinfo() returned with an error.\n");
        return 1;
    }
    socketfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
    if(bind(socketfd, serverinfo->ai_addr, serverinfo->ai_addrlen)!=0){
        fprintf(stderr, "ERROR: bind() returned with an error.\n");
        return 1;
    }
    #ifdef DEBUG
    printf("Bound socket.\n");
    #endif
    if(listen(socketfd, maxusers)!=0){
        fprintf(stderr, "ERROR: listen() returned with an error.\n");
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
		char ipaddr[14];
	    sprintf(ipaddr,"%u.%u.%u.%u",((struct sockaddr*)&newaddr)->sa_data[2],((struct sockaddr*)&newaddr)->sa_data[3],((struct sockaddr*)&newaddr)->sa_data[4],((struct sockaddr*)&newaddr)->sa_data[5]);
	    writetolog("Connected.\n", ipaddr);
		if((numclients)>=maxusers){
		    //Too many connected users
		    writetolog("Too many users connected, refusing connection.\n", ipaddr);
		    sendInt(acceptedfd, USER_LIMIT);
		    sendString(acceptedfd, "Too many users connected.");
		    close(acceptedfd);
		    pthread_yield();
		}else{
		    #ifdef DEBUG
		    printf("Accepted a socket.\n");
		    #endif
		    toThreads[numclients].fd = acceptedfd;
		    toThreads[numclients].ip = (char *)ipaddr;
		    if(pthread_create(&threads[numclients], NULL, handleclient, (void *) &toThreads[numclients])) {
			    printf("There was an error creating the thread.");
			    exit(-1);
		    }
		    else printf("Created thread ID %ld.\n",(long)numclients);
		    numclients++;
		}
	}
    close(socketfd);
    pthread_mutex_destroy(&logmutex);
    pthread_exit(NULL);
}
void *handleclient(void *fromMain){
	#ifdef DEBUG
	printf("Hello, I'm a thread!\n");
	#endif
    int timedout = FALSE;
    struct to_thread *data = (struct to_thread *) fromMain;
    int sockfd = data->fd;
    char *addr;
    addr = data->ip;
    fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);
	struct timeval timeoutval;
	timeoutval.tv_sec = timeout;
	timeoutval.tv_usec = 0;
    int imagenum = 0;
    time_t lastconnection[ratenum];
    memset(lastconnection, 0 , ratenum*sizeof(time_t));
    while(!timedout){
        //TODO enforce rate rules
        //set timeout
        select(sockfd+1, &readfds, NULL, NULL, &timeoutval);
        if(FD_ISSET(sockfd, &readfds)){
	        int imgsize;
            //receive the size of the image
            int rcvstatus = receiveBytes(sockfd, sizeof(int), (void *)&imgsize);
            time_t currenttime = time(NULL);
            if(!rcvstatus){
				#ifdef DEBUG
                printf("Client closed connection.\n");
                #endif
                timedout=TRUE;
                break;
            }
            #ifdef DEBUG
            printf("Client is sending a file of size %d bytes.\n",imgsize);
            #endif
            char *imgbuf;
            if(imgsize>=MAX_FILE_SIZE){
                imgsize=MAX_FILE_SIZE;
            }
            imgbuf = (char*)malloc(imgsize);
            //recieve the image
            rcvstatus = receiveBytes(sockfd, imgsize, (void *)imgbuf);
            #ifdef DEBUG
            printf("Recieve status: %d\n", rcvstatus);
            #endif
            imagenum++;
            int i;
            int isfull = TRUE;
            for(i=0; i<ratenum; i++){
                if(difftime(currenttime, lastconnection[i])>ratetime){
                    lastconnection[i]=currenttime;
                    #ifdef DEBUG
                    printf("found a time: %f\n", difftime(currenttime, lastconnection[i]));
                    #endif
                    isfull = FALSE;
                    break;
                }
            }
            if(isfull){
                free(imgbuf);
                writetolog("Rate limit exceeded.\n", addr);
                sendInt(sockfd, RATE_LIMIT_EXCEEDED);
                sendString(sockfd, "Rate limit exceeded.");
            } else {
                //write to temporary file
                writetofile(imgbuf, imgsize, imagenum);
                #ifdef DEBUG
                printf("Wrote an image of size %d to file.\n", imgsize);
                #endif
                free(imgbuf); //don't need this in memory anymore because we saved it to a file
            
                char url[MAX_URL_LENGTH];
                processImage(url, imagenum);
                //don't need to waste disk space by keeping file
                char remove[MAX_URL_LENGTH];
                sprintf(remove, "rm tmp-%u-%d.png", (unsigned int)pthread_self(), imagenum);
                system(remove);
                printf("Parsed URL: %s\n", url);
                sendInt(sockfd, SUCCESS);
                sendString(sockfd, url);
            }
        }
        else {
            writetolog("Connection timed out.\n", addr);
            sendInt(sockfd, TIMEOUT);
            sendString(sockfd, "Connection timed out.");
            timedout=TRUE;
        }
    }
    #ifdef DEBUG
    printf("Closing connection to host.\n");
    #endif
    numclients--;
    close(sockfd); 
    pthread_exit(NULL);
}
int writetofile(char* buffer, size_t size, int counter){
    FILE *fp;
    char tmp[MAX_URL_LENGTH];
    sprintf(tmp, "tmp-%u-%d.png", (unsigned int)pthread_self(), counter);
    #ifdef DEBUG
    printf("The string is now %s.\n",tmp);
    #endif
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
    sendInt(sockfd, length);
    return send(sockfd, toSend, length, 0);
}
int processImage(char *str, int counter){
	char command[MAX_URL_LENGTH];
	sprintf(command, "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner tmp-%u-%d.png", (unsigned int)pthread_self(), counter);
    FILE *process = popen(command, "r");
    int i;
    #ifdef DEBUG
    printf("Started processing image.\n");
    #endif
    for(i=0; i<5; i++){
        fgets(str,MAX_URL_LENGTH,process);
    }
    int pclose(FILE *stream); 
    return 0;
}
void checkPort(char *port) {
	int portInt = atoi(port);
	if((portInt < 2000) || (portInt > 3000)) {
		fprintf(stderr, "Port %d is out of range. Please choose a port between 2000 and 3000.\n", portInt);
		exit(1);
	}
	return;
}
void writetolog(char *message, char addr[14]){
    char ftime[256];
    char fullmsg[256];
    time_t ctime = time(NULL);
    thetime = localtime(&ctime);
    strftime(ftime, 256, "%d %b %Y %I:%M:%S %p", thetime);
    sprintf(fullmsg, "%s from %s: %s", ftime, addr, message);
    pthread_mutex_lock(&logmutex);
    FILE *fp;
    fp=fopen("server.log", "a");
    fputs(fullmsg, fp);
    fclose(fp);
    pthread_mutex_unlock(&logmutex);
}
