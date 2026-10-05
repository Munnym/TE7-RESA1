#ifndef CHANNEL_LIST_H
#define CHANNEL_LIST_H

// Structure pour stocker les informations des utilisateurs connectés
struct user
{
    char nick[NICK_LEN];
    time_t connect_time;
    struct sockaddr_in client; 
    int fd;
    struct user *next;
};   
//========================================================FONCTIONS UTILISATEURS ET LISTE CHAINEE===========================================================================================================================
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

#endif
