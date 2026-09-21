CC=gcc
CFLAGS=-Wall
#LDFLAGS=-lpthread
all: client server

%.o: %.c
       $(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f client server