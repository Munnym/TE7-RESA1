CC=gcc
CFLAGS=-Wall
#LDFLAGS=-lpthread
all: client server server_test
client : client.o
	$(CC) $(CFLAGS) $< -o $@

server : server.o
	$(CC) $(CFLAGS) $< -o $@

server_test : server_test.o
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f client server server_test *.o