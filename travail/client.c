#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>
#include <fcntl.h>

#include "common.h"
#include "msg_struct.h"

void die(int ret_value, const char *msg)
{
    if (ret_value < 0)
    {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

int recv_safe(int fd, void *buf, int to_recv) {
    int total_bites_recv = 0;
    while (total_bites_recv < to_recv) {
        int ret = recv(fd, (char *)buf + total_bites_recv, to_recv - total_bites_recv, 0);
        if (ret <= 0) {
			return -1;
		}     
        total_bites_recv += ret;
    }
    return total_bites_recv;
}

int send_safe(int fd, const void *buf, int to_send) {
    int total_bites_send = 0;
    while (total_bites_send < to_send) {
        ssize_t s = send(fd, (const char *)buf + total_bites_send, to_send - total_bites_send, 0);
        if (s <= 0) return -1;
        total_bites_send += s;
    }
    return total_bites_send;
}

int read_safe(int fd, void *buf, int to_read) {
    int total_bites_read = 0;
    while (total_bites_read < to_read) {
        int ret = read(fd, (char *)buf + total_bites_read, to_read- total_bites_read);
        if (ret < 0) {
			return -1;
		}
		if (ret == 0) {
			break;
		}  
        total_bites_read += ret;
    }
    return total_bites_read;
}

int write_safe(int fd, void *buf, int to_write) {
    int total_bites_wrote = 0;
    while (total_bites_wrote < to_write) {
        int ret = write(fd, (char *)buf + total_bites_wrote, to_write - total_bites_wrote);
        if (ret < 0) {
			return -1;
		}
		if (ret == 0) {
			break;
		}  
        total_bites_wrote += ret;
    }
    return total_bites_wrote;
}

int handle_connect(char *domain_name, char *port) {
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

int handle_bind(int *port_out){
	int listener_fd = socket(AF_INET, SOCK_STREAM,0);
    die(listener_fd, "socket");

    int yes=1;
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in listener_addr;
	memset(&listener_addr, 0, sizeof(listener_addr));
    listener_addr.sin_family = AF_INET;
    listener_addr.sin_port = 0;
    inet_aton("127.0.0.1", &listener_addr.sin_addr);
    
    int ret_value = bind(listener_fd, (struct sockaddr *)&listener_addr, sizeof(listener_addr));
    die(ret_value, "bind_user");
   
    ret_value = listen(listener_fd, BACKLOG);
    die(ret_value, "listen_user");

	socklen_t len = sizeof(listener_addr);
	ret_value = getsockname(listener_fd, (struct sockaddr *)&listener_addr, &len);
	die(ret_value, "port_optention");
	*port_out = ntohs(listener_addr.sin_port);

	return listener_fd;
}

void echo_client(int sockfd) {
	struct message msgstruct;
	char buff[MSG_LEN];
	char filepath[MSG_LEN] = {0};
	char filename[INFOS_LEN] = {0};
	char destfile[INFOS_LEN] = {0};
	int filecount = 0;
	int user_request = 0;
	char requester[INFOS_LEN] = {0};
	int listener_fd = -1;
	char file_rcv_path[INFOS_LEN + 13];
	char file_rcv_name[INFOS_LEN];
	char file_rcv_buff[PROTO_MAX_PAYLOAD];

	int n;
	struct pollfd fds[3];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

	fds[2].fd = listener_fd;
    fds[2].events = POLLIN;
    fds[2].revents = 0;

	int ret_value = -1;
	printf("Please, enter your message :\n");
	while (1) {
		ret_value = poll(fds, 3, -1);
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
			char *payload = buff;
			msgstruct.pld_len = strlen(buff);
			memcpy(msgstruct.nick_sender, "azertyapres", 11);
			if(user_request)
			{
				char *answer_payload = NULL;
				if (strcmp(buff, "Y") == 0 || strcmp(buff, "y") == 0)
				{
					int port_out;
					char listen_coord[MSG_LEN];
					listener_fd = handle_bind(&port_out);
					if(listener_fd == -1)
					{
						continue;
					}
					fds[2].fd = listener_fd;
					snprintf(listen_coord, sizeof(listen_coord), "127.0.0.1:%d", port_out);

					msgstruct.type = FILE_ACCEPT;
					answer_payload = listen_coord;
					msgstruct.pld_len = strlen(answer_payload);
				}
				else if (strcmp(buff, "N") == 0 || strcmp(buff, "n") == 0)
				{
					msgstruct.type = FILE_REJECT;
					msgstruct.pld_len = 0;
				}
				else
				{
					printf("Please answer Y or N\n");
        			continue;
				}
				memcpy(msgstruct.infos, requester, INFOS_LEN - 1);
				// Sending structure
				if (send_safe(sockfd, &msgstruct, sizeof(msgstruct)) <= 0) {
					break;
				}
				// Sending message (ECHO)
				if(msgstruct.pld_len > 0  && msgstruct.pld_len < MSG_LEN + 1)
				{
					if (send_safe(sockfd, answer_payload, msgstruct.pld_len) <= 0) {
						break;
					}
				}
				user_request = 0;
				continue;
			}
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
				payload = msg;
				msgstruct.pld_len = strlen(msg);
			}
			else if(strncmp(buff, "/msg ",5) == 0) {
				char *pseudo_message = buff + 5;
				char *message_with_space = strchr(pseudo_message, ' ');
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
				payload = message;
				msgstruct.pld_len = len_message;
			}
			else if(strncmp(buff, "/send ",6) == 0) {
				char *pseudo_file = buff + 6;
				char *file_with_space = strchr(pseudo_file, ' ');
				if(file_with_space == NULL || file_with_space == pseudo_file)
				{
					printf("Usage : /send <pseudo> <file>\n");
					continue;
				}
				int len_pseudo = file_with_space - pseudo_file;
				if(len_pseudo >= INFOS_LEN)
				{
					printf("pseudo's length > %d\n", INFOS_LEN);
					continue;
				}

				char *file_path = file_with_space + 1;
				int len_file_path = strlen(file_path);

				if(len_file_path >= 2 && file_path[0] == '"' && file_path[len_file_path - 1] == '"')
				{
					file_path[len_file_path - 1] = '\0';
					file_path++;
					len_file_path -= 2;
				}
				if(len_file_path == 0)
				{
					printf("Usage : /send <pseudo> <file>\n");
					continue;
				}

				struct stat st;
				if(stat(file_path, &st) < 0 || !S_ISREG(st.st_mode))
				{
					printf("File not found: %s\n", file_path);
					continue;
				}

				char *file_name = strrchr(file_path, '/');
				if(file_name == NULL)
				{
					file_name = file_path;
				}
				else
				{
					file_name++;
				}
				int len_file_name = strlen(file_name);
				if(len_file_name == 0 || len_file_name >= INFOS_LEN)
				{
					printf("Invalid file name\n");
					continue;
				}

				if(len_file_path >= (int)sizeof(filepath))
				{
					printf("Path too long\n");
					continue;
				}

				memcpy(filepath, file_path, len_file_path);
				filepath[len_file_path] = '\0';

				memcpy(filename, file_name, len_file_name);
				filename[len_file_name] = '\0';

				memcpy(destfile, pseudo_file, len_pseudo);
				destfile[len_pseudo] = '\0';
				filecount = 1;

				msgstruct.type = FILE_REQUEST;
				memcpy(msgstruct.infos, pseudo_file, len_pseudo);
				payload = file_name;
				msgstruct.pld_len = len_file_name;

			}	
			else {
				msgstruct.type = ECHO_SEND;
			}
			// Sending structure
			if (send_safe(sockfd, &msgstruct, sizeof(msgstruct)) <= 0) {
				break;
			}
			// Sending message (ECHO)
			if(msgstruct.pld_len > 0  && msgstruct.pld_len < MSG_LEN + 1)
			{
				if (send_safe(sockfd, payload, msgstruct.pld_len) <= 0) {
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
			if (recv_safe(sockfd, &msgstruct, sizeof(struct message)) <= 0) {
				break;
			}
			// Cleaning memory
			memset(buff, 0, MSG_LEN);
			// Receiving message
			if (msgstruct.pld_len > 0) {
				if (msgstruct.pld_len >= MSG_LEN) {
					break;
				}
				if (recv_safe(sockfd, buff, msgstruct.pld_len) <= 0) {
					break;
				}
				if (msgstruct.type == FILE_REQUEST)
				{
					strncpy(file_rcv_name,buff,sizeof(file_rcv_name) - 1);
					memcpy(requester, msgstruct.infos, INFOS_LEN - 1);

    				user_request = 1;
					printf("%s wants you to accept the transfer of the file named %s. Do you accept? [Y/N]\n", requester,file_rcv_name);
				}
				if (msgstruct.type == FILE_ACCEPT && filecount == 1)
				{
					char addr_user[MSG_LEN] = {0};
					char *port_user = strrchr(buff, ':');
					if(port_user == NULL)
					{
						printf("Error : invalid adress\n");
						continue;
					}
					port_user++;
					memcpy(addr_user,buff,strlen(buff) - strlen(port_user) - 1);
					int sockfd_user = handle_connect(addr_user, port_user);
					if(sockfd_user == -1)
					{
						printf("Error: user port or domain adress\n");
						continue;
					}
					filecount = 0;
					char payload[PROTO_MAX_PAYLOAD];
					int fd = open(filepath, O_RDONLY);
					if(fd<0)
					{
						perror("file opening");
						close(sockfd_user);
						continue;
					}
					int nb_bytes_read = 0;
					struct message filemsg;
					while((nb_bytes_read = read_safe(fd, payload, PROTO_MAX_PAYLOAD)) > 0){
						//Preparing structure
						memset(&filemsg, 0, sizeof(filemsg));
						filemsg.type = FILE_SEND;
						memcpy(filemsg.infos, filename, strlen(filename));
						filemsg.pld_len = nb_bytes_read;
						// Sending structure
						if (send_safe(sockfd_user, &filemsg, sizeof(filemsg)) <= 0) {
							break;
						}
						// Sending message (ECHO)
						if (send_safe(sockfd_user, payload, nb_bytes_read) <= 0) {
							break;
						}
						printf("Message sent!\n");
					}
					close(fd);
					shutdown(sockfd_user, SHUT_WR);
					struct message ack;
					if (recv_safe(sockfd_user, &ack, sizeof(ack)) > 0 && ack.type == FILE_ACK)
					{
						printf("%s has been received.\n", ack.infos);
					}
					close(sockfd_user);
				}
				if (msgstruct.type == FILE_REJECT){
					filecount = 0;
					printf("file rejected\n");
					continue;
				}
			}
			printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
			printf("Received: %s\n", buff);
		}
		if (fds[2].revents & POLLIN)
		{
			int sockfd_peer = accept(listener_fd, NULL, NULL);
			close(listener_fd);
			listener_fd = -1;
			fds[2].fd = -1;

			int nb_bytes_read = 0;
			mkdir(".resa1",0755);
			mkdir(".resa1/inbox",0755);
			memset(file_rcv_path,0,sizeof(file_rcv_path));
			snprintf(file_rcv_path, sizeof(file_rcv_path), ".resa1/inbox/%s", file_rcv_name);
			int file_fd = open(file_rcv_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			die(file_fd,"File's creation");

			int expected = 0;
			// Receiving structure
			while(recv_safe(sockfd_peer, &msgstruct, sizeof(struct message)) > 0)
			{
				// Cleaning memory
				memset(file_rcv_buff, 0, PROTO_MAX_PAYLOAD);
				// Receiving message
				if (msgstruct.pld_len > 0) 
				{
					if (msgstruct.pld_len > PROTO_MAX_PAYLOAD) {
						break;
					}
					if (recv_safe(sockfd_peer, file_rcv_buff, msgstruct.pld_len) <= 0) {
						break;
					}
					if (msgstruct.type == FILE_SEND)
					{
						expected = msgstruct.pld_len;
						nb_bytes_read = write_safe(file_fd, file_rcv_buff, msgstruct.pld_len);
						if (nb_bytes_read != expected)
						{
							break;
						}
					}
				}
			}
			close(file_fd);
			if(nb_bytes_read != expected)
			{
				close(sockfd_peer);
				continue;
			}
			else
			{
				struct message msg_ack;
				memset(&msg_ack, 0, sizeof(msg_ack));
				msg_ack.type = FILE_ACK;
				memcpy(msg_ack.infos, file_rcv_name, strlen(file_rcv_name));
				msg_ack.pld_len = 0;

				// Sending structure
				if (send_safe(sockfd_peer, &msg_ack, sizeof(msg_ack)) <= 0) {
					continue;
				}
				printf("file recieved!\n");
				close(sockfd_peer);
			}
		}
	}
}


int main(int argc, char *argv[]) {
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

