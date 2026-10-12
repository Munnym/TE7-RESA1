CC=gcc
CFLAGS=-Wall
#LDFLAGS=-lpthread
all: client server 
client : client.o
	$(CC) $(CFLAGS) $< -o $@

server : server.o
	$(CC) $(CFLAGS) $< -o $@


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

client.o: client.c common.h 
server.o: server.c server.h common.h types.h channel_list.h


clean:
	rm -f client server *.o