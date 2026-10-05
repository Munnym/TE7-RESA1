#define MSG_LEN 1024
#define SERV_PORT "8080"
#define SERV_ADDR "127.0.0.1"
#define PROTO_MAX_PAYLOAD 65536
#define BACKLOG 10
#define FDS_SIZE 10

#ifndef COMMON_H
#define COMMON_H
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

#endif
