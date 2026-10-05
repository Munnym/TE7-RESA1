#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <ctype.h>
#include <stddef.h>

#include "common.h"
#include "msg_struct.h"
#include "channel_list.h"
#include "server.h"
#include "types.h"

//==================================================================================================================================================================================================
//=======================================================================FONCTIONS===========================================================================================================================
//==================================================================================================================================================================================================


// Fonction pour gérer les erreurs
void die(int ret_value,char* msg_err)
{
    if (ret_value < 0)
    {
        perror(msg_err);
        exit(EXIT_FAILURE);
    }
}


//====================================================================FONCTION SERVEUR===========================================================================================================================


int handle_bind(int port)
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
    return listen_fd;
}


//Fonction connexion principale du serveur
int receive_and_echo(int connfd,struct sockaddr_in *cli ,socklen_t *len, int port)
{
    //Création de la socket d'écoute
    int listen_fd = handle_bind(port);

	struct user *sockets=NULL;

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
    struct message msgstruct;
    char playload[MSG_LEN + 1];
    while(1)
    {
        printf("Wait messages \n ");

        int nb_active_fd=poll(fds,FDS_SIZE,-1);
        die(nb_active_fd,"Polling error");

        printf("active fd =%d \n",nb_active_fd);
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
                printf("Accepted  \n");
				insert_last(&sockets,*cli,connfd,NULL,time(NULL));
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
                memset(&msgstruct, 0, sizeof(struct message));
                memset(playload, 0, MSG_LEN + 1);
                int boole = 1; //boolean pour savoir si le client est encore connecté

                // Receiving structure
                if (boole && (read_on_socket(fds[i].fd, &msgstruct, sizeof(struct message)) <= 0))
                {
			        boole = 0;
		        }
                if (msgstruct.pld_len < 0 || msgstruct.pld_len > MSG_LEN)
                {
                    printf("Payload length invalide : %d\n", msgstruct.pld_len);
                    boole = 0;
                }

                // Receiving message
                if (boole && msgstruct.pld_len > 0 && read_on_socket(fds[i].fd, playload, msgstruct.pld_len) <= 0)
                {
                    boole = 0;
                }
                boole=switch_message_type(fds[i].fd, msgstruct, playload, sockets);
                if (!boole)
                {
                    printf("Deconnected (fd=%d)\n", fds[i].fd);
                    close(fds[i].fd);
                    remove_user(&sockets, fds[i].fd);
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    fds[i].revents = 0;
                }
            }
        }
    }
    close(connfd);
    free_list(&sockets);
	close(listen_fd);
    return 0;
}

//==================================================================================================================================================================================================
//========================================================================MAIN==========================================================================================================================
//==================================================================================================================================================================================================

int main(int argc, char* argv[]) 
{
    int port;
    if (argc > 2) 
    {
        fprintf(stderr, "Usage : %s <server_port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    else if (argc == 1) 
    {
        port = 12345; // Default port
    }
    else 
    {
        port = atoi(argv[1]);
    }
    printf("Server is running on port %d\n", port);
    struct sockaddr_in cli;
	int connfd=-1;
	socklen_t len;
	len = sizeof(cli);
	receive_and_echo(connfd,&cli ,&len, port);
	return EXIT_SUCCESS;
}
