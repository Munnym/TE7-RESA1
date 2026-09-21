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


clean:
	rm -f client server *.o