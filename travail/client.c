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
#include "msg_struct.h"

void echo_client(int sockfd) {
	struct message msgstruct;
	char buff[MSG_LEN];
	int n;
	struct pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;
    fds[1].revents = 0;
	int ret_value = -1;
	printf("Please, enter your message :\n");
	while (1) {
		ret_value = poll(fds, 2, -1);
		assert(ret_value != -1);
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		memset(&msgstruct, 0, sizeof(struct message));
		if(fds[0].revents & POLLIN)
        {
			// Getting message from client
			n = 0;
			while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
			buff[strcspn(buff, "\n")] = '\0';
			// Filling structure
			char *playload = buff;
			msgstruct.pld_len = strlen(buff);
			memcpy(msgstruct.nick_sender, "azertyapres", 11);
			if (strcmp(buff, "/nick") == 0) {
				printf("Usage : /nick <pseudo>\n");
				continue;
			}
			if (strcmp(buff, "/whois") == 0) { 
				printf("Usage : /whois <pseudo>\n"); 
				continue;
			}
			if (strcmp(buff, "/msg") == 0) {
				printf("Usage : /msg <pseudo> <message>\n");
				continue;
			}
			if (strcmp(buff, "/msgall") == 0) {
				printf("Usage : /msgall <message>\n");
				continue;
			}
			if(strncmp(buff, "/nick ",6) == 0) {
				char *pseudo = buff + 6;
				int len = strlen(pseudo);
				if (len == 0) {
					printf("Usage : /nick <pseudo>\n");
					continue;
				}
				if(len >= INFOS_LEN) {
					printf("pseudo's length >= %d\n", INFOS_LEN);
					continue;
				}
				msgstruct.type = NICKNAME_NEW;
				memcpy(msgstruct.infos, pseudo, len);
				msgstruct.pld_len = 0;
			}
			else if(strncmp(buff, "/whois ",7) == 0 ) {
				msgstruct.type = NICKNAME_INFOS;
				int len = strlen(buff + 7); 
				if(0 < len && len < INFOS_LEN) {
					memcpy(msgstruct.infos, buff + 7, len);
					msgstruct.pld_len = 0;
				}
				else if(len == 0) {
					printf("Usage : /whois <pseudo>\n");
					continue;
				}
				else if(len > INFOS_LEN - 1) {
					printf("pseudo's length > %d\n", INFOS_LEN);
					continue;
				}
			}
			else if(strcmp(buff, "/who") == 0) {
				msgstruct.type = NICKNAME_LIST;
				msgstruct.pld_len = 0;
			}
			else if(strncmp(buff, "/msgall ",8) == 0) {
				char *msg = buff + 8;
				if (*msg == '\0'){
					printf("Usage : /msgall <message>\n");
        			continue;
				}
				msgstruct.type = BROADCAST_SEND;
				playload = msg;
				msgstruct.pld_len = strlen(msg);
			}
			else if(strncmp(buff, "/msg ",5) == 0) {
				char * pseudo_message = buff + 5;
				char * message_with_space = strchr(pseudo_message, ' ');
				if(message_with_space == NULL || message_with_space == pseudo_message)
				{
					printf("Usage : /msg <pseudo> <message>\n");
					continue;
				}
				int len_pseudo = strlen(pseudo_message) - strlen(message_with_space);
				if(len_pseudo >= INFOS_LEN)
				{
					printf("pseudo's length > %d\n", INFOS_LEN);
					continue;
				}

				char* message = message_with_space + 1;
				int len_message = strlen(message);
				if(len_message == 0)
				{
					printf("Usage : /msg <pseudo> <message>\n");
					continue;
				}
				msgstruct.type = UNICAST_SEND;
				memcpy(msgstruct.infos, pseudo_message, len_pseudo);
				playload = message;
				msgstruct.pld_len = len_message;
			}
			else {
				msgstruct.type = ECHO_SEND;
			}
			// Sending structure
			if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				break;
			}
			// Sending message (ECHO)
			if(msgstruct.pld_len > 0  && msgstruct.pld_len < MSG_LEN + 1)
			{
				if (send(sockfd, playload, msgstruct.pld_len, 0) <= 0) {
					break;
				}
				printf("Message sent!\n");
			}
			if(strncmp(buff, "/quit",5) == 0)
			{
				printf("Deconnected !\n");
				close(sockfd);
				exit(EXIT_SUCCESS);
			}
        }
		if (fds[1].revents & POLLIN)
		{
			// Receiving structure
			if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
				break;
			}
			// Cleaning memory
			memset(buff, 0, MSG_LEN);
			// Receiving message
			if (msgstruct.pld_len > 0) {
				if (msgstruct.pld_len >= MSG_LEN) {
					break;
				}
				if (recv(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
					break;
				}
			}
			printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
			printf("Received: %s", buff);
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
		printf("Usage : ./client <server_domain_name> <server_port> \n");
		return EXIT_FAILURE;
	}
	int sfd;
	sfd = handle_connect(argv[1], argv[2]);
	echo_client(sfd);
	close(sfd);
	return EXIT_SUCCESS;
}

