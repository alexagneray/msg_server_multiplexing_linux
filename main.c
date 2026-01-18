#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <stdio.h>
#include <string.h>

#include <fcntl.h>

#define USERID_LEN 4
#define USERNAME_LEN 64
#define USERMSG_MAXLEN 4096
#define SRVMSG_MAXLEN 8192

#define USER_WELCOME_MSG "Bienvenue sur le serveur de discussion, quel est votre nom ?\n"
#define USER_WELCOME_MSG_LEN sizeof(USER_WELCOME_MSG)


typedef struct
{
    int used; // 0 si inutilisé, 1 si user actif
    int authentified; // 0 si non authentifié, 1 si authentifié
    char username[USERNAME_LEN];
    int speakto; // indice de l'interlocuteur
} user_info_t;

/**
 * @brief Renvoie l'indice de la première valeur -1 du tableau.
 * Renvoie -1 si la valeur -1 n'est pas dans le tableau
 * 
 * @param tab 
 * @param tabSize 
 * @return int 
 */
int find_empty_slot(int tab[], unsigned int tabSize)
{
    for(unsigned int i=0;i<tabSize;++i)
    {
        if(tab[i]==-1)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Génère un string contenant la liste des utilisateurs actifs. 
 * La fonction alloue dynamiquement et mets à jour le pointeur ppzUserList.
 * La mémoire allouée devra être libérée avec free_user_list_string()
 * 
 * @param atUserInfo 
 * @param nUserInfoLen 
 * @param ppzUserList 
 */
int get_user_list_string(user_info_t atUserInfo[], unsigned int nUserInfoLen, char **ppzUserList)
{
    const unsigned int USER_LABEL_SIZE = USERID_LEN + USERNAME_LEN + 3; // userid - USERNAME
    *ppzUserList = malloc(USER_LABEL_SIZE * nUserInfoLen);
    int offset = 0;
    for(unsigned int i=0;i<nUserInfoLen;++i)
    {
        if(atUserInfo[i].used)
        {
            offset += sprintf((*ppzUserList)+offset, "%d - %s\n", i,atUserInfo[i].username);
        }
    }
    return offset;
}

/**
 * @brief Libère la mémoire allouée pour la liste des utilisateurs.
 * Cette fonction doit impérativement être appelée avec get_user_list_string
 * pour libérer la mémoire allouée dynamiquement !
 * 
 * @param ppzUserList 
 */
void free_user_list_string(char **ppzUserList)
{
    if(*ppzUserList)
    {
        free(*ppzUserList);
        *ppzUserList = NULL;
    }
    
}

/**
 * @brief Renvoie l'indice du fd de socket.
 * 
 * @param sockfd socket recherchée
 * @param aSockfd liste des sockets
 * @param sockfdLen taille de la liste 
 * @return int 
 */
int get_idx_from_sockfd(int sockfd, int aSockfd[], int sockfdLen)
{
    for(int i=0; i<sockfdLen;++i)
    {
        if(aSockfd[i]==sockfd)
        {
            return i;
        }
    }
    return -1;
}


int main(int argc, char **argv)
{

    // ------------------------------------------------------------------------
    int opt;
    char *pzPort = NULL;
    int sockfd;
    int err;

    struct addrinfo hints;
    struct addrinfo *pRes;

    struct sockaddr cltSockAddr;
    socklen_t cltSockAddrLen;

    const unsigned int USER_COUNT = 10;
    int csockfd[USER_COUNT];
    for(unsigned int i=0;i<USER_COUNT;++i)
    {
        csockfd[i] = -1;
    }

    user_info_t atUserInfo[USER_COUNT];
    memset(atUserInfo, 0, sizeof(atUserInfo));
    

    struct epoll_event ev;

    struct epoll_event events[USER_COUNT+1];


    const unsigned int BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE]; 
    ssize_t nBytes;

    char bufferToSend[SRVMSG_MAXLEN];
    memset(bufferToSend,0,SRVMSG_MAXLEN);

    const char * QUIT = "QUIT";

    int epfd = epoll_create(11);
    if(epfd < 0)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    int n;
    // ------------------------------------------------------------------------


    while((opt = getopt(argc,argv,"p:h"))!=-1)
    {
        switch (opt)
        {
        case 'p':
            pzPort = optarg;
            break;
        case 'h':
            printf("%s -a address -p port\n", argv[0]);
            exit(EXIT_SUCCESS);
        default:
            break;
        }
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    err = getaddrinfo(NULL, pzPort, &hints, &pRes);
    if(err == -1)
    {
        fprintf(stderr, "%s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    ev.data.fd = sockfd;
    ev.events = EPOLLIN;
    err = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    if(err == -1)
    {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    err = bind(sockfd, pRes->ai_addr, pRes->ai_addrlen);
    if(err == -1)
    {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        perror("bind");
        exit(EXIT_FAILURE);
    }

    err = listen(sockfd, 10);
    if(err == -1)
    {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Initialisation du serveur sur le port : %s Achevée\n", pzPort);

    while(1)
    {   
        n = epoll_wait(epfd,events,USER_COUNT+1,-1);

        if(n==-1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for(int i=0;i<n;i++)
        {
            if(events[i].data.fd==sockfd)
            {
                printf("Un nouvel utiisateur veut se connecter !\n");
                int newsock;
                newsock = accept(sockfd,&cltSockAddr,&cltSockAddrLen);
                int idx = find_empty_slot(csockfd, USER_COUNT);
                if(idx==-1) // Il n'y a plus de slot dispo, on ferme la connexion
                {
                    close(newsock);
                }
                else // Un nouvel utilisateur se connecte
                {
                    printf("Ajout d'un nouvel utiisateur à l'emplacement %d\n",
                             idx);
                    csockfd[idx] = newsock;
                    ev.data.fd = newsock;
                    ev.events = EPOLLIN;
                    memset(atUserInfo + idx,0,sizeof(user_info_t));
                    atUserInfo[idx].used = 1;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, csockfd[idx], &ev);

                    send(newsock, USER_WELCOME_MSG, USER_WELCOME_MSG_LEN,0);
                }
            }
            else // On parle avec un user connecté
            {
                memset(buffer, 0, BUFFER_SIZE);
                nBytes = recv(events[i].data.fd,buffer,BUFFER_SIZE,0);
                if(nBytes==0)
                {
                    continue;
                }
                printf("%lu octets recus\n", nBytes);

                int idx = get_idx_from_sockfd(events[i].data.fd, csockfd, USER_COUNT);

                if(!atUserInfo[idx].authentified) // authentification
                {
                    strncpy(atUserInfo[idx].username, buffer, USERNAME_LEN);
                    atUserInfo[idx].authentified = 1;
                    atUserInfo[idx].speakto = -1;
                    char *pzUserList;
                    int nUserListLen = 0;
                    nUserListLen = get_user_list_string(atUserInfo, USER_COUNT, &pzUserList);
                    send(events[i].data.fd, pzUserList, nUserListLen,0);
                    free_user_list_string(&pzUserList);
                }
                else if(atUserInfo[idx].authentified &&
                    atUserInfo[idx].speakto == -1) // connexion à un autre user
                {
                    unsigned int speakToIdx;
                    int ret;
                    ret = sscanf(buffer,"%u",&speakToIdx);
                    if(ret!=EOF)
                    {
                        if(speakToIdx < USER_COUNT)
                        {
                            memset(buffer, 0, BUFFER_SIZE);
                            nBytes = snprintf(buffer, BUFFER_SIZE,
                                                "Vous parlez maintenant à %s\n",
                                                atUserInfo[speakToIdx].username);
                            atUserInfo[idx].speakto = speakToIdx;
                            send(csockfd[idx],buffer,nBytes,0);
                        }
                    }
                }
                else // user authentifié et connecté à un autre user
                {
                    const unsigned int speakto = atUserInfo[idx].speakto;
                    send(csockfd[speakto], buffer, nBytes,0);

                    if(!memcmp(QUIT, buffer, strlen(QUIT)-1))
                    {
                        printf("ON QUITTE\n");
                    }
                }

            }
        }
    }

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    
    exit(EXIT_SUCCESS);
}