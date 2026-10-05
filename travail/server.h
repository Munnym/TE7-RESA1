#ifndef SERVER_H
#define SERVER_H

struct server;
struct user;
struct message;

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

#endif
