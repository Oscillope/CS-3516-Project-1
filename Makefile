CC = gcc
CFLAGS = -Wall

all: server client
server: server.o
	$(CC) $(CFLAGS) -pthread server.o -o server
client: client.o
	$(CC) $(CFLAGS) client.o -o client
server.o:
	$(CC) $(CFLAGS) -lpthread -c server.c
client.o:
	$(CC) $(CFLAGS) -c client.c
clean:
	rm -f *~ *.o
