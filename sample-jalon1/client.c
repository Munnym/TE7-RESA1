#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>

#include "common.h"

void echo_client(int sockfd) {
	char buff[MSG_LEN];
	int n;
	struct pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;
    fds[1].revents = 0;
	int size_message;
	int ret_value = -1;
	printf("Please, enter your message :\n");
	while (1) {
		ret_value = poll(fds, 2, -1);
		assert(ret_value != -1);
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		if(fds[0].revents & POLLIN)
        {
			// Getting message from client
			n = 0;
			while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
			// Sending message size (ECHO)
			size_message = strlen(buff);
			if (send(sockfd, &size_message, sizeof(int), 0) <= 0) {
				break;
			}
			printf("Message length sent!\n");
			// Sending message (ECHO)
			if (send(sockfd, buff, strlen(buff), 0) <= 0) {
				break;
			}
			printf("Message sent!\n");
			if(strncmp(buff, "/quit",5) == 0)
			{
				printf("Deconnected !\n");
				close(sockfd);
				exit(EXIT_SUCCESS);
			}
        }
		if (fds[1].revents & POLLIN)
		{
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
			printf("Please, enter your message :\n");
		}
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

