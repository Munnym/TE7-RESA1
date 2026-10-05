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

//==================================================================================================================================================================================================
//=======================================================================STRUCTURES======================================================================================================================
//==================================================================================================================================================================================================

// Structure pour stocker les informations des utilisateurs connectés
struct user
{
    char nick[NICK_LEN];
    time_t connect_time;
    struct sockaddr_in client; 
    int fd;
    struct user *next;
};   
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

//=======================================================================FONCTION TEST ECRITURE/LECTURE===========================================================================================================================
// Fonction limite sur la lecture des sockets
int read_on_socket(int sockfd, void* ptr, int size)
{
    if (size <= 0)
    {
        return 1;
    }
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
        bits_lu+= ret_value;
    }
    return size;
}
// Fonction limite sur l ecriture des sockets
int write_on_socket(int sockfd, void* ptr, int size) // permet d'ecrire le message dans son intégralité ou de renvoyer une erreur
{
    if (size <= 0)
    {
        return 1;
    }
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
            perror("write");
            return -1;
        }
        bits_ecris+= ret_value; // incremente le nombre de bits à écrire
    }
    return size;
}

//======================================================================UTILISATEURS ET LISTE CHAINEE===========================================================================================================================
// Fonction pour créer un nouvel utilisateur
struct user* create_user(struct sockaddr_in data,int fd,char nick[NICK_LEN],time_t time) 
{
    struct user *newuser = malloc(sizeof(struct user));
    if (newuser == NULL)
    {
        exit(EXIT_FAILURE);
    }
    if (nick != NULL)
        memcpy(newuser->nick, nick, strlen(nick) + 1);
    else
        newuser->nick[0] = '\0';
    newuser->connect_time=time;
    newuser->client =data;
    newuser->next = NULL;
    newuser->fd = fd;
    return newuser;
}

// Fonction pour insérer un utilisateur au début de la liste
void insert_first (struct user **pro,struct sockaddr_in data,int fd,char nick[NICK_LEN],time_t time)
{
    struct user *newuser = create_user(data,fd,nick,time);
    newuser->next = *pro;
    *pro = newuser;
}

//  Fonction pour insérer un utilisateur à la fin de la liste
void insert_last(struct user **pro, struct sockaddr_in data, int fd,char nick[NICK_LEN],time_t time)
{
    struct user *newuser = create_user(data, fd,nick,time);
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

// Fonction pour supprimer un utilisateur de la liste
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


// Fonction pour compter le nombre d'utilisateurs dans la liste
int count_users(struct user *pro)
{
    int count = 0;
    while (pro != NULL)
    {
        count++;
        pro = pro->next;
    }
    return count;
}


//  Fonction pour libérer la liste chainée
void free_list(struct user **pro)
{
    struct user *cur = *pro;
    while (cur != NULL) 
    {
        struct user *next = cur->next;  
        free(cur);
        cur = next;
    }
    *pro = NULL;
}
//=====================================================================FONCTIONS SOCKETS=============================================================================================================================

// Fonction pour tester si un pseudo est valide
int test_nickname(char nick[NICK_LEN])
{
    int length=strlen(nick);
    if (length == 0 || length>=NICK_LEN)
    {
        return -1;
    }
    for (int i=0;i<length;i++)
    {
        if (!isalnum((unsigned char)nick[i]))
        {
            
            return -1;
        }
    }
    return 0;
}


// Fonction pour vérifier si un pseudo est déjà pris par un autre utilisateur
int nick_is_taken(struct user *pro,char *nick, int selfsocket)
{
    int test_name = test_nickname(nick);
    if (test_name == 0)
    {

        for (struct user *cur = pro; cur; cur = cur->next) 
        {
            if (cur->fd == selfsocket)
            { 
                continue;
            }      
            if (cur->nick[0] == '\0')       
            { 
                continue;
            }    
            if (strcasecmp(cur->nick, nick) == 0)
            { 
                return 1;
            }    
        }
    }
    return 0;
}

//Fonction pour ajouter un pseudo à un utilisateur existant
void add_pseudo_user(struct user **pro, char *pseudo, int fd)
{
    struct user *cur = *pro;

    while (cur != NULL)
    {
        if (cur->fd == fd)
        {
            memcpy(cur->nick, pseudo, strlen(pseudo) + 1);
            break;
        }

        cur = cur->next;
    }
}

// Fonction pour trouver un utilisateur par son descripteur de socket
struct user *find_by_fd(struct user *pro, int fd)
{
    for (; pro; pro = pro->next)
    {

        if (pro->fd == fd)
        {
            return pro;
        }
    }
    return NULL;
}

// Fonction pour trouver un utilisateur par son pseudo
struct user *find_by_nick(struct user *pro,char *nick)
{
    for (; pro; pro = pro->next)
    {
        if (pro->nick[0] != '\0' && strcasecmp(pro->nick, nick) == 0)
        { 
            return pro;
        }
    }
    return NULL;
}

// Fonction pour envoyer un message à un utilisateur
int send_message(int fd, char *sender, enum msg_type type,char *infos,char *txt)
{
    struct message resp;
    memset(&resp, 0, sizeof resp);

    strncpy(resp.nick_sender, sender ? sender : "", NICK_LEN - 1);
    strncpy(resp.infos, infos ? infos : "", INFOS_LEN - 1);
    resp.type = type;
    resp.pld_len = txt ? (int)strlen(txt) + 1 : 0;

    // Envoi de la structure 
    if (write_on_socket(fd, &resp, sizeof resp) <= 0)
        return -1;

    // Envoi du payload, seulement s'il y en a un 
    if (resp.pld_len > 0 && write_on_socket(fd, (void *)txt, resp.pld_len) <= 0)
        return -1;

    return 0;
}

//====================================================================FONCTION SERVEUR===========================================================================================================================
//Fonction connexion principale du serveur
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
                printf("Accepted A \n");
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
<<<<<<< HEAD
                if (boole && (read_on_socket(fds[i].fd, playload, msgstruct.pld_len) <= 0))
=======
                if (boole && msgstruct.pld_len > 0 && read_on_socket(fds[i].fd, buffer, msgstruct.pld_len) <= 0)
>>>>>>> c98ae0ba19cf3dc0bcf5738374b94944ec13138b
                {
                    boole = 0;
                }
                switch(msgstruct.type)
                {
                    case NICKNAME_NEW:
                    {
                        struct user *me = find_by_fd(sockets, fds[i].fd);
                        if (me == NULL) 
                        { 
                            boole = 0; 
                            break; 
                        }
                        else if (nick_is_taken(sockets, msgstruct.infos, fds[i].fd))
                        {
                            if (send_message(fds[i].fd, "Server", NICKNAME_NEW,"","Nickname already taken, please choose another one") < 0)
                            {
                                boole = 0;
                            }
                        }
                        else
                        {
                            char old[NICK_LEN];
                            strncpy(old, me->nick, NICK_LEN - 1);
                            old[NICK_LEN - 1] = '\0';

                            add_pseudo_user(&sockets, msgstruct.infos, fds[i].fd);   // écrase l'ancien 

                            char txt[2 * NICK_LEN + 64];
                            if (old[0])
                            {

                                snprintf(txt, sizeof(txt), "Your nickname is now %s (was %s)",msgstruct.infos, old);
                            }
                            else
                            {
                                snprintf(txt, sizeof(txt), "Welcome on the chat %s", msgstruct.infos);
                            }
                            if (send_message(fds[i].fd, "Server", NICKNAME_NEW, msgstruct.infos,txt) < 0)
                            {
                                boole = 0;                                  
                            } 
                        }
                        break;
                    }
                
                    case NICKNAME_LIST:
<<<<<<< HEAD
                    { 
                        int count = count_users(sockets);
                        char *nicknames = malloc(count * NICK_LEN);
                        if (nicknames == NULL)
=======
                    {
                        char list[MSG_LEN];
                        int off = snprintf(list, sizeof list, "Online users are");

                        for (struct user *c = sockets; c != NULL; c = c->next)
>>>>>>> c98ae0ba19cf3dc0bcf5738374b94944ec13138b
                        {
                            if (c->nick[0] == '\0')          // pas encore de pseudo 
                                continue;
                            if (off >= (int)sizeof list)     //plein
                                break;
                            off += snprintf(list + off, sizeof list - off, "\n- %s", c->nick);
                        }

                        if (send_message(fds[i].fd, "Server", NICKNAME_LIST, "", list) < 0)
                            boole = 0;
                        break;
                    }
                    case NICKNAME_INFOS:
                    {
                        struct user *t = find_by_nick(sockets, msgstruct.infos);
                        char txt[MSG_LEN];
                        if (t == NULL)
                        {
                            snprintf(txt, sizeof txt, "User %s does not exist", msgstruct.infos);
                        }
                        else
                        {
                            char date[32];
                            strftime(date, sizeof date, "%Y/%m/%d@%H:%M", localtime(&t->connect_time));
                            snprintf(txt, sizeof txt,
                                    "%s connected since %s with IP address %s and port number %d",
                                    t->nick, date,
                                    inet_ntoa(t->client.sin_addr),
                                    ntohs(t->client.sin_port));
                        }

                        if (send_message(fds[i].fd, "Server", NICKNAME_INFOS, "", txt) < 0)
                            boole = 0;
                        break;
                    }
                    case ECHO_SEND:
                    {
                        playload[msgstruct.pld_len] = '\0';
                        printf("Messsage receved : %s\n", playload);
                        printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);

                        if (0 == strcmp(playload, "/quit")) // deconnexion du client si /quit
                        {
                            boole = 0;
                            break;
                        }
                        if (send_message(fds[i].fd, msgstruct.nick_sender, ECHO_SEND, msgstruct.infos, playload) < 0)
                        {
                            boole = 0;
                        }
                        printf("Message sent!\n");
                        break;
                    }
                    case UNICAST_SEND:
                    {
                        struct user *destination = find_by_nick(sockets, msgstruct.infos);
                        if (destination == NULL)
                        {
                            send_message(fds[i].fd, "Server", UNICAST_SEND,"","Server error: user not found");
                            break;
                        }
                        if (send_message(destination->fd, msgstruct.nick_sender, UNICAST_SEND, msgstruct.infos, playload) < 0)
                        {
                            send_message(fds[i].fd, "Server", UNICAST_SEND,"","Server error: unable to send unicast message");
                        }
                        break;
                    }
                    case BROADCAST_SEND:
                    {
                        int socket_expediteur = fds[i].fd;
                        struct user *expediteur = find_by_fd(sockets, socket_expediteur);
                        for (struct user *cur = sockets; cur; cur = cur->next)
                        {
                            if (cur->fd != socket_expediteur && cur->nick[0] != '\0')
                            {
                                if (send_message(cur->fd, expediteur->nick, BROADCAST_SEND, msgstruct.infos, playload) < 0)
                                {
                                    send_message(cur->fd, "Server", BROADCAST_SEND,"","Server error: unable to send broadcast message");
                                }
                            }
                        }
                        break;
                    }
                    case FILE_REQUEST:
                    {
                        struct user *destination = find_by_nick(sockets, msgstruct.infos);
                        if (destination == NULL)
                        {
                            send_message(fds[i].fd, "Server", UNICAST_SEND,"","Server error: user not found");
                            break;
                        }
                        send_message(destination->fd, msgstruct.nick_sender, msgstruct.type, msgstruct.infos,playload);
                        break;
                    }
                    case FILE_ACCEPT:
                    {
                        struct user *emission = find_by_nick(sockets, msgstruct.infos);
                        if (emission == NULL)
                        {
                            send_message(fds[i].fd, "Server", UNICAST_SEND,"","Server error: user not found");
                            break;
                        }
                        send_message(emission->fd, msgstruct.nick_sender, msgstruct.type, msgstruct.infos,playload);
                        break;

                    }
                    case FILE_REJECT:
                    {
                        struct user *emission = find_by_nick(sockets, msgstruct.infos);
                        if (emission == NULL)
                        {
                            send_message(fds[i].fd, "Server", UNICAST_SEND,"","Server error: user not found");
                            break;
                        }
                        send_message(emission->fd, msgstruct.nick_sender, msgstruct.type, msgstruct.infos,"Client rejected the file transfer request");
                        break;
                    }
                    case FILE_SEND:
                    {
                        break;
                    }
                    case FILE_ACK:
                    {
                        break;
                    }

                    default:
                    {
                        printf("Message type %d not implemented\n", msgstruct.type);
                        send_message(fds[i].fd, "Server", msgstruct.type,"","Server error: message type not implemented");
                        break;
                    }
                }
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
    if (argc != 2) 
    {
        fprintf(stderr, "Usage : %s <server_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct sockaddr_in cli;
	int connfd=-1;
	socklen_t len;
	len = sizeof(cli);
	receive_and_echo(connfd,&cli ,&len, atoi(argv[1]));
	return EXIT_SUCCESS;
}
