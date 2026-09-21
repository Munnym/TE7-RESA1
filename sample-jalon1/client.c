#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

void echo_client(int sockfd) {
	char buff[MSG_LEN];
	int n;
	while (1) {
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		printf("Message: ");
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
		// Sending message size (ECHO)
		int size_message = strlen(buff);
		if (send(sockfd, &size_message, sizeof(int), 0) <= 0) {
			break;
		}
		printf("Message length sent!\n");
		// Sending message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			break;
		}
		printf("Message sent!\n");
		if (recv(sockfd, &size_message, sizeof(int), 0) <= 0) {
			break;
		}
		printf("Received: %d\n", size_message);
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Receiving message
		if (recv(sockfd, buff, size_message, 0) <= 0) {
			break;
		}
		printf("Received: %s\n", buff);
	}
}

int handle_connect(char * domain_name, char * port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(domain_name, port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char * argv[]) {
	if(argc != 3)
	{
		printf("Nécessite 3 arguments\n");
		return EXIT_FAILURE;
	}
	int sfd;
	sfd = handle_connect(argv[1], argv[2]);
	echo_client(sfd);
	close(sfd);
	return EXIT_SUCCESS;
}

