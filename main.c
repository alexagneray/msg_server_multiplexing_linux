#include <stdlib.h>

#include <assert.h>

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
    int b_used; // 0 si inutilisé, 1 si user actif
    int b_authentified; // 0 si non authentifié, 1 si authentifié
    char ac_username[USERNAME_LEN];
    int idx_speakto; // indice de l'interlocuteur
} user_info_t;

/**
 * @brief Renvoie l'indice de la première valeur -1 du tableau.
 * Renvoie -1 si la valeur -1 n'est pas dans le tableau
 * 
 * @param ac_tocheck 
 * @param u_tab_size 
 * @return int 
 */
int find_empty_slot(int ac_tocheck[], unsigned int u_tab_size)
{
    for(unsigned int i=0;i<u_tab_size;++i)
    {
        if(ac_tocheck[i]==-1)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Génère un string contenant la liste des utilisateurs actifs. 
 * La fonction alloue dynamiquement et mets à jour le pointeur ppz_user_list.
 * La mémoire allouée devra être libérée avec free_user_list_string()
 * 
 * @param at_userinfo 
 * @param cnt_userinfo 
 * @param ppz_user_list 
 */
int get_user_list_string(user_info_t at_userinfo[], unsigned int cnt_userinfo, char **ppz_user_list)
{
    const unsigned int USER_LABEL_SIZE = USERID_LEN + USERNAME_LEN + 3; // userid - USERNAME
    *ppz_user_list = malloc(USER_LABEL_SIZE * cnt_userinfo);
    int offset = 0;
    for(unsigned int i=0;i<cnt_userinfo;++i)
    {
        if(at_userinfo[i].b_used)
        {
            offset += sprintf((*ppz_user_list)+offset, "%d - %s\n", i,at_userinfo[i].ac_username);
        }
    }
    return offset;
}

/**
 * @brief Libère la mémoire allouée pour la liste des utilisateurs.
 * Cette fonction doit impérativement être appelée avec get_user_list_string
 * pour libérer la mémoire allouée dynamiquement !
 * 
 * @param ppz_user_list 
 */
void free_user_list_string(char **ppz_user_list)
{
    if(*ppz_user_list)
    {
        free(*ppz_user_list);
        *ppz_user_list = NULL;
    }
    
}

/**
 * @brief Renvoie l'indice du fd de socket.
 * 
 * @param fd_sock socket recherchée
 * @param afd_sock liste des sockets
 * @param cnt_fd_sock nombre de sockets
 * @return int 
 */
int get_idx_from_sockfd(int fd_sock, int afd_sock[], unsigned int cnt_fd_sock)
{
    for(unsigned int i=0; i<cnt_fd_sock;++i)
    {
        if(afd_sock[i]==fd_sock)
        {
            return i;
        }
    }
    return -1;
}

void close_user_connection(unsigned int idx_user, int afd_sock[], user_info_t at_userinfo[], unsigned int cnt_fd_sock)
{
    assert(idx_user < cnt_fd_sock);

    close(afd_sock[idx_user]);
    afd_sock[idx_user] = -1;
    memset(&at_userinfo[idx_user], 0, sizeof(user_info_t));
    at_userinfo[idx_user].idx_speakto = -1;
}

int main(int argc, char **argv)
{

    // ------------------------------------------------------------------------
    int i_opt;
    char *pz_port = NULL;
    int fd_sock;
    int i_err;

    struct addrinfo t_hints;
    struct addrinfo *pt_res;

    struct sockaddr t_client_sockaddr;
    socklen_t cnt_client_sockaddr;

    const unsigned int USER_COUNT = 10;
    int afd_user_sock[USER_COUNT];
    for(unsigned int i=0;i<USER_COUNT;++i)
    {
        afd_user_sock[i] = -1;
    }

    user_info_t at_userinfo[USER_COUNT];
    memset(at_userinfo, 0, sizeof(at_userinfo));
    

    struct epoll_event t_event;

    struct epoll_event at_user_events[USER_COUNT+1];


    const unsigned int BUFFER_SIZE = 256;
    char ac_buffer[BUFFER_SIZE]; 
    ssize_t cnt_bytes_transfered;

    char ac_tosend[SRVMSG_MAXLEN];
    memset(ac_tosend,0,SRVMSG_MAXLEN);

    const char *QUIT = "/QUIT";

    int fd_ep = epoll_create(11);
    if(fd_ep < 0)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    int cnt_user_events;
    // ------------------------------------------------------------------------


    while((i_opt = getopt(argc,argv,"p:h"))!=-1)
    {
        switch (i_opt)
        {
        case 'p':
            pz_port = optarg;
            break;
        case 'h':
            printf("%s -a address -p port\n", argv[0]);
            exit(EXIT_SUCCESS);
        default:
            break;
        }
    }

    memset(&t_hints, 0, sizeof(t_hints));
    t_hints.ai_family = AF_INET;
    t_hints.ai_socktype = SOCK_STREAM;
    t_hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    i_err = getaddrinfo(NULL, pz_port, &t_hints, &pt_res);
    if(i_err == -1)
    {
        fprintf(stderr, "%s\n", gai_strerror(i_err));
        exit(EXIT_FAILURE);
    }

    fd_sock = socket(AF_INET, SOCK_STREAM, 0);

    t_event.data.fd = fd_sock;
    t_event.events = EPOLLIN;
    i_err = epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd_sock, &t_event);
    fcntl(fd_sock, F_SETFL, O_NONBLOCK);

    if(i_err == -1)
    {
        shutdown(fd_sock, SHUT_RDWR);
        close(fd_sock);
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    i_err = bind(fd_sock, pt_res->ai_addr, pt_res->ai_addrlen);
    if(i_err == -1)
    {
        shutdown(fd_sock, SHUT_RDWR);
        close(fd_sock);
        perror("bind");
        exit(EXIT_FAILURE);
    }

    i_err = listen(fd_sock, 10);
    if(i_err == -1)
    {
        shutdown(fd_sock, SHUT_RDWR);
        close(fd_sock);
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Initialisation du serveur sur le port : %s Achevée\n", pz_port);

    while(1)
    {   
        cnt_user_events = epoll_wait(fd_ep,at_user_events,USER_COUNT+1,-1);

        if(cnt_user_events==-1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for(int i=0;i<cnt_user_events;i++)
        {
            if(at_user_events[i].data.fd==fd_sock)
            {
                printf("Un nouvel utilisateur veut se connecter !\n");
                int newsock;
                newsock = accept(fd_sock,&t_client_sockaddr,&cnt_client_sockaddr);
                int idx = find_empty_slot(afd_user_sock, USER_COUNT);
                if(idx==-1) // Il n'y a plus de slot dispo, on ferme la connexion
                {
                    close(newsock);
                }
                else // Un nouvel utilisateur se connecte
                {
                    printf("Ajout d'un nouvel utiisateur à l'emplacement %d\n",
                             idx);
                    afd_user_sock[idx] = newsock;
                    t_event.data.fd = newsock;
                    t_event.events = EPOLLIN;
                    memset(at_userinfo + idx,0,sizeof(user_info_t));
                    at_userinfo[idx].b_used = 1;
                    epoll_ctl(fd_ep, EPOLL_CTL_ADD, afd_user_sock[idx], &t_event);

                    send(newsock, USER_WELCOME_MSG, USER_WELCOME_MSG_LEN,0);
                }
            }
            else // On parle avec un user connecté
            {
                memset(ac_buffer, 0, BUFFER_SIZE);
                cnt_bytes_transfered = recv(at_user_events[i].data.fd,ac_buffer,BUFFER_SIZE,0);
                if(cnt_bytes_transfered==0)
                {
                    continue;
                }
                char *pc_found = strpbrk(ac_buffer,"\r\n");
                if(pc_found)
                {
                    *pc_found = '\0';
                }

                printf("%lu octets recus\n", cnt_bytes_transfered);

                int idx = get_idx_from_sockfd(at_user_events[i].data.fd, afd_user_sock, USER_COUNT);

                if(!at_userinfo[idx].b_authentified) // authentification
                {
                    strncpy(at_userinfo[idx].ac_username, ac_buffer, USERNAME_LEN);
                    at_userinfo[idx].b_authentified = 1;
                    at_userinfo[idx].idx_speakto = -1;
                    char *pzUserList;
                    int nUserListLen = 0;
                    nUserListLen = get_user_list_string(at_userinfo, USER_COUNT, &pzUserList);
                    send(at_user_events[i].data.fd, pzUserList, nUserListLen,0);
                    free_user_list_string(&pzUserList);
                }
                else if(at_userinfo[idx].b_authentified &&
                    at_userinfo[idx].idx_speakto == -1) // connexion à un autre user
                {
                    unsigned int idx_speakto;
                    int ret;
                    ret = sscanf(ac_buffer,"%u",&idx_speakto);
                    if(ret!=EOF)
                    {
                        if(idx_speakto < USER_COUNT)
                        {
                            memset(ac_buffer, 0, BUFFER_SIZE);
                            cnt_bytes_transfered = snprintf(ac_buffer, BUFFER_SIZE,
                                                "Vous parlez maintenant à %s\n",
                                                at_userinfo[idx_speakto].ac_username);
                            at_userinfo[idx].idx_speakto = idx_speakto;
                            send(afd_user_sock[idx],ac_buffer,cnt_bytes_transfered,0);
                        }
                    }
                }
                else // user authentifié et connecté à un autre user
                {
                    if(!memcmp(QUIT, ac_buffer, strlen(QUIT)-1))
                    {
                        close_user_connection(idx, afd_user_sock, at_userinfo, USER_COUNT);
                    }
                    else 
                    {
                        const unsigned int idx_speakto = at_userinfo[idx].idx_speakto;
                        send(afd_user_sock[idx_speakto], ac_buffer, cnt_bytes_transfered,0);
                    }
                }

            }
        }
    }

    shutdown(fd_sock, SHUT_RDWR);
    close(fd_sock);
    
    exit(EXIT_SUCCESS);
}