#ifndef TYPES_H
#define TYPES_H

struct user;
struct message;

//==================================================================FONCTIONS TYPE MESSAGE=============================================================================================================================

int Nickname_new(int fd, struct user **sockets, struct message msgstruct)
{
    struct user *me = find_by_fd(*sockets,fd);
    if (me == NULL) 
    { 
        return 0;
    }
    else if (nick_is_taken(*sockets, msgstruct.infos,fd))
    {
        if (send_message(fd, "Server", NICKNAME_NEW,"","Nickname already taken, please choose another one") < 0)
        {
            return 0;
        }
    }
    else
    {
        char old[NICK_LEN];
        strncpy(old, me->nick, NICK_LEN - 1);
        old[NICK_LEN - 1] = '\0';

        add_pseudo_user(sockets, msgstruct.infos,fd);   // écrase l'ancien 

        char txt[2 * NICK_LEN + 64];
        if (old[0])
        {

            snprintf(txt, sizeof(txt), "Your nickname is now %s (was %s)",msgstruct.infos, old);
        }
        else
        {
            snprintf(txt, sizeof(txt), "Welcome on the chat %s", msgstruct.infos);
        }
        if (send_message(fd, "Server", NICKNAME_NEW, msgstruct.infos,txt) < 0)
        {
            return 0;
        } 
    }
    return 1;
}

int Nickname_list(int fd, struct user **sockets)
{
    char list[MSG_LEN];
    int off = snprintf(list, sizeof list, "Online users are");

    for (struct user *cur = *sockets; cur != NULL; cur = cur->next)
    {
        if (cur->nick[0] == '\0')          // pas encore de pseudo   
        {
            continue;
        }
        if (off >= (int)sizeof list)     //plein
        {
            return -1;
        }
        off += snprintf(list + off, sizeof list-off, "\n- %s", cur->nick);
    }

    if (send_message(fd, "Server", NICKNAME_LIST, "", list) < 0)
    {
        return 0;
    }
    return 1;

}

int Nickname_infos(int fd, struct user **sockets, struct message msgstruct)
{
    struct user *user = find_by_nick(*sockets, msgstruct.infos);
    char txt[MSG_LEN];
    if (user == NULL)
    {
        snprintf(txt, sizeof txt, "User %s does not exist", msgstruct.infos);
    }
    else
    {
        char date[32];
        strftime(date, sizeof date, "%Y/%m/%d@%H:%M", localtime(&user->connect_time));
        snprintf(txt, sizeof txt,"%s connected since %s with IP address %s and port number %d",user->nick, date,inet_ntoa(user->client.sin_addr),ntohs(user->client.sin_port));
    }

    if (send_message(fd, "Server", NICKNAME_INFOS, "", txt) < 0)
    {
        return 0;
    }
    return 1;
}


int Echo_send(int fd, struct message msgstruct, char *playload)
{
    playload[msgstruct.pld_len] = '\0';
    printf("Message received : %s\n", playload);
    printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
    
    if (0 == strcmp(playload, "/quit")) // deconnexion du client si /quit
    {
        return 0;
    }
    if (send_message(fd, msgstruct.nick_sender, ECHO_SEND, msgstruct.infos, playload) < 0)
    {
        return 0;
    }
    printf("Message sent!\n");
    return 1;
}

int Unicast_send(int fd, struct user **sockets, struct message msgstruct, char *playload)
{
    struct user *destination = find_by_nick(*sockets, msgstruct.infos);
    if (destination == NULL)
    {
        send_message(fd, "Server", UNICAST_SEND,"","Server error: user not found");
        return 0;
    }
    if (send_message(destination->fd, msgstruct.nick_sender, UNICAST_SEND, msgstruct.infos, playload) < 0)
    {
        send_message(fd, "Server", UNICAST_SEND,"","Server error: unable to send unicast message");
        return 0;
    }
    return 1;
}

int Broadcast_send(int fd, struct user **sockets, struct message msgstruct, char *playload)
{
    int socket_expediteur = fd;
    struct user *expediteur = find_by_fd(*sockets, socket_expediteur);
    for (struct user *cur = *sockets; cur; cur = cur->next)
    {
        if (cur->fd != socket_expediteur && cur->nick[0] != '\0')
        {
            if (send_message(cur->fd, expediteur->nick, BROADCAST_SEND, msgstruct.infos, playload) < 0)
            {
                send_message(cur->fd, "Server", BROADCAST_SEND,"","Server error: unable to send broadcast message");
                return 0;
            }
        }
    }
    return 1;
}

int File_request(int fd, struct user **sockets, struct message msgstruct, char *playload)
{
    struct user *destination = find_by_nick(*sockets, msgstruct.infos);
    if (destination == NULL)
    {
        send_message(fd, "Server", UNICAST_SEND,"","Server error: user not found");
        return 0;
    }
    char txt[MSG_LEN];
    snprintf(txt, sizeof txt, "User %s wants to send you a file", msgstruct.nick_sender);
    send_message(destination->fd, msgstruct.nick_sender, msgstruct.type, msgstruct.infos, txt);
    send_message(destination->fd, msgstruct.nick_sender, msgstruct.type, msgstruct.infos,playload);
    return 1;
}

int File_accept(int fd, struct user **sockets, struct message msgstruct, char *playload)
{
    struct user *emission = find_by_nick(*sockets, msgstruct.infos);
    if (emission == NULL)
    {
        send_message(fd, "Server", UNICAST_SEND,"","Server error: user not found");
        return 0;
    }
    char txt[MSG_LEN];
    snprintf(txt, sizeof txt, "User %s accepted the file transfer request", msgstruct.nick_sender);
    send_message(emission->fd, msgstruct.nick_sender, msgstruct.type, msgstruct.infos, txt);
    send_message(emission->fd, msgstruct.nick_sender, msgstruct.type, msgstruct.infos,playload);
    return 1;
}


int File_reject(int fd, struct user **sockets, struct message msgstruct, char *playload)
{
    struct user *emission = find_by_nick(*sockets, msgstruct.infos);
    if (emission == NULL)
    {
        send_message(fd, "Server", UNICAST_SEND,"","Server error: user not found");
        return 0;
    }
    char txt[MSG_LEN];
    snprintf(txt, sizeof txt, "User %s rejected the file transfer request", msgstruct.nick_sender);
    send_message(emission->fd, msgstruct.nick_sender, msgstruct.type, msgstruct.infos, txt);
    return 1;
}


int switch_message_type(int fd,struct message msgstruct, char *playload, struct user *sockets)
{
    int boole = 1;
    switch(msgstruct.type)
    {
        case NICKNAME_NEW:
        {
            return Nickname_new(fd, &sockets, msgstruct);
        }
        case NICKNAME_LIST:
        {
            boole = Nickname_list(fd, &sockets);
            if (boole < 0)
            {
                send_message(fd, "Server", NICKNAME_LIST,"","Server error: unable to send nickname list");
                return 0;
            }
            return boole;
        }
        case NICKNAME_INFOS:
        {
            return Nickname_infos(fd, &sockets, msgstruct);
        }
        case ECHO_SEND:
        {
            return Echo_send(fd, msgstruct, playload);
        }
        case UNICAST_SEND:
        {
            return Unicast_send(fd, &sockets, msgstruct, playload);
        }
        case BROADCAST_SEND:
        {
            return Broadcast_send(fd, &sockets, msgstruct, playload);
        }
        case FILE_REQUEST:
        {
            return File_request(fd, &sockets, msgstruct, playload);
        }
        case FILE_ACCEPT:
        {
            return File_accept(fd, &sockets, msgstruct, playload);
        }
        case FILE_REJECT:
        {
            return File_reject(fd, &sockets, msgstruct, playload);
        }
        case FILE_SEND:
        {   
            printf("Message type %d pear to pear server isn't include\n", msgstruct.type);
            send_message(fd, "Server", msgstruct.type,"","Server error: Connection between clients without server");
            return 0;
        }
        case FILE_ACK:
        {
            printf("Message type %d pear to pear server isn't include\n", msgstruct.type);
            send_message(fd, "Server", msgstruct.type,"","Server error: Connection between clients without server");
            return 0;
        }

        default:
        {
            printf("Message type %d not implemented\n", msgstruct.type);
            send_message(fd, "Server", msgstruct.type,"","Server error: message type not implemented");
            return 0;
        }
    }
}


#endif
