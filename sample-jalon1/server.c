#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"

#define BACKLOG 10
#define FDS_SIZE 10



struct user
{
    struct sockaddr_in client; // le premier sera tjrs la socket d'écoute
    int fd;
    struct user *next;
};    

struct message
{
    int type; 
    int size;
    long speaker_id;
};

void die(int ret_value,char* msg_err)
{
    if (ret_value < 0)
    {
        perror(msg_err);
        exit(EXIT_FAILURE);
    }
}

int read_on_socket(int sockfd, void* ptr, int size)
{
    int bits_lu=0;
    int a_lire= size;
    int ret_value=0;
    while ( bits_lu != a_lire)
    {
        ret_value = read(sockfd,(char *)(ptr) + bits_lu,a_lire - bits_lu);
        if (ret_value == 0)
        {
            // Le client s'est déconnecté
            return 0;
        }

        if (ret_value < 0)
        {
            perror("read");
            return -1;
        }

        die(ret_value,"readinng msg header");
        bits_lu+= ret_value;
    }
    return ret_value;
}
int write_on_socket(int sockfd, void* ptr, int size) // permet d'ecrire le message dans son intégralité ou de renvoyer une erreur
{
    int bits_ecris=0;
    int a_ecrire= size;
    int ret_value=0;
    while ( bits_ecris != a_ecrire) 
    {
        //ecriture des bits restant 
        ret_value = write(sockfd,(char *)(ptr) + bits_ecris,a_ecrire - bits_ecris); //  on cast pour bien incrementer le pointeur de 1 bits a chaque fois
        if (ret_value == 0)
        {
            // Le client s'est déconnecté
            return 0;
        }

        if (ret_value < 0)
        {
            perror("read");
            return -1;
        }

        die(ret_value,"readinng msg header");
        bits_ecris+= ret_value; // incremente le nombre de bits à écrire
    }
    return ret_value;
}
struct user* create_user(struct sockaddr_in data,int fd) 
{
    struct user *newuser = malloc(sizeof(struct user));
    if (newuser == NULL)
    {
        exit(EXIT_FAILURE);
    }
    newuser->client =data;
    newuser->next = NULL;
    newuser->fd = fd;
    return newuser;
}

void insert_first (struct user **pro,struct sockaddr_in data,int fd)
{
    struct user *newuser = create_user(data,fd);
    newuser->next = *pro;
    *pro = newuser;
}


void insert_last(struct user **pro, struct sockaddr_in data, int fd)
{
    struct user *newuser = create_user(data, fd);
    if (*pro == NULL)
    {
        *pro = newuser;
        return;
    }
    struct user *user = *pro;
    while (user->next != NULL)
        user = user->next;
    user->next = newuser;
}

void remove_user(struct user **pro, int fd)
{
    struct user *cur = *pro;
    struct user *prev = NULL;
    while (cur != NULL)
    {
        if (cur->fd == fd)
        {
            if (prev == NULL)
                *pro = cur->next;
            else
                prev->next = cur->next;
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}




void free_list(struct user **pro)
{
    struct user *cur = *pro;
    while (cur != NULL) 
    {
        struct user *next = cur->next;  /* sauvegarder AVANT free */
        free(cur);
        cur = next;
    }
    *pro = NULL;
}



int handle_bind() {
    struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, SERV_PORT, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}



int receive_and_echo(int connfd,struct sockaddr_in *cli ,socklen_t *len, int port)
{
    //Création de la socket d'écoute
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);
    die(listen_fd,"Erreur de socket d'écoute");

    int yes=1;
    setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));

    //Addressage de la socket
    struct sockaddr_in server_addr;
    server_addr.sin_family= AF_INET;
    server_addr.sin_port=htons(port);
    inet_aton("127.0.0.1",&server_addr.sin_addr);

    //lier la socket d"ecoute a une addresse
    int bind_value;
    bind_value=bind(listen_fd,(struct sockaddr *)&server_addr,sizeof(server_addr));
    die(bind_value,"Addressage incorrect");

    //Etape 3 listen
    int listen_value;
    listen_value=listen(listen_fd,BACKLOG);
    die(listen_value,"Erreur lors de l'écoute");

	struct user *sockets=create_user(server_addr,listen_fd);

	struct pollfd fds[FDS_SIZE];
    fds[0].fd=listen_fd;
    fds[0].events=POLLIN;
    fds[0].revents=0;
    for (int i=1;i<FDS_SIZE;i++)
    {
        fds[i].fd=-1;
        fds[i].events=0;
        fds[i].revents=0;
    }
    int size;
    char buffer[MSG_LEN + 1];
    while(1)
    {
        printf("attente des messages \n ");

        int nb_active_fd=poll(fds,FDS_SIZE,-1);
        die(nb_active_fd,"Polling error");

        printf("actif fd =%d \n",nb_active_fd);
        for (int i=0;i<FDS_SIZE;i++)
        {
			if (i == 0 && fds[0].revents & POLLIN)
            {
				//accepter la socket
                fds[0].revents=0;
                
				if ((connfd = accept(fds[0].fd, (struct sockaddr*) cli, len)) < 0) {
					perror("accept()\n");
					continue;
				}
                printf("client accepté \n");
				insert_last(&sockets,*cli,connfd);
                for (int j = 1; j < FDS_SIZE; j++) 
                {
                    if (fds[j].fd == -1) 
                    {
                        fds[j].fd = connfd;
                        fds[j].events = POLLIN;
                        fds[j].revents = 0;
                        break;
                    }
                }
            }
            else if ( fds[i].revents & POLLIN)
            {
                // Cleaning memory
                memset(buffer, 0, MSG_LEN + 1);
                int boole = 1; //boolean pour savoir si le client est encore connecté

                // Receiving message length
                if (boole && read_on_socket(fds[i].fd,&size,sizeof(size)) <= 0) 
                {
                    boole = 0;
                }
                
                if ( boole &&(size <= 0 || size > MSG_LEN) )
                {
                    fprintf(stderr, "Taille invalide : %d\n", size);
                    boole = 0;
                }
                // Receiving message
                if (boole &&read_on_socket(fds[i].fd,buffer,(size_t)size) <= 0) 
                {
                    boole = 0;
                }

                if (boole)
                {
                    buffer[size] = '\0';
                    printf("Message reçu : %s\n", buffer);

                    if (strcmp(buffer, "/quit") == 0) // deconnexion du client si /quit
                        boole = 0;
                }
                // Sending message length
                if (boole &&write_on_socket(fds[i].fd,&size,sizeof(size)) <= 0) 
                {
                    boole=0;
                }

                // Sending message (ECHO)
                if (boole && write_on_socket(fds[i].fd,buffer,(size_t)size) <= 0) 
                {
                   boole = 0;
                }
                if (!boole)
                {
                    printf("Client déconnecté (fd=%d)\n", fds[i].fd);
                    close(fds[i].fd);
                    remove_user(&sockets, fds[i].fd);
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    fds[i].revents = 0;
                }
            }
        }
    }
    free_list(&sockets);
	close(listen_fd);
    return 0;
}



int main(int argc, char* argv[]) 
{

    if (argc != 2) 
    {
        fprintf(stderr, "Usage : %s <server_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct sockaddr_in cli;
	int connfd=-1;
	socklen_t len;
	//int sfd;
	//sfd = handle_bind();
	//if ((listen(sfd, SOMAXCONN)) != 0) {
		//perror("listen()\n");
		//exit(EXIT_FAILURE);
	//}
	len = sizeof(cli);
	receive_and_echo(connfd,&cli ,&len, atoi(argv[1]));
	close(connfd);
	//close(sfd);
	return EXIT_SUCCESS;
}

