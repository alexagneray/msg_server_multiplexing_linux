#include "srvcore.h"
#include "sockutils.h"
#include <netdb.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 

// const unsigned int USER_COUNT = 10;
// int afd_user_sock[USER_COUNT];

// user_info_t at_userinfo[USER_COUNT];
// struct epoll_event t_event;
// struct epoll_event at_user_events[USER_COUNT+1];
// const unsigned int BUFFER_SIZE = 256;
// char ac_buffer[BUFFER_SIZE]; 
// ssize_t cnt_bytes_transfered;

// char ac_tosend[SRVMSG_MAXLEN];

// const char *QUIT = "/QUIT";

typedef struct
{
    int b_used; // 0 si inutilisé, 1 si user actif
    int b_authentified; // 0 si non authentifié, 1 si authentifié
    char ac_username[USERNAME_LEN];
    int idx_speakto; // indice de l'interlocuteur
} user_info_t;

user_info_t at_userinfo[USER_COUNT];


struct epoll_event t_event;

struct epoll_event at_user_events[USER_COUNT+1];

char ac_buffer[BUFFER_SIZE]; 
int fd_ep;

int start_server(char *pz_port)
{
    int fd_sock = start_server_sock(pz_port);

    if(fd_sock == -1)
    {
        fprintf(stderr, "Erreur lors de la création du socket serveur\n");
        return -1;
    }       
    

}

int server_loop()
{
    int cnt_user_events = 0;
    struct sockaddr t_client_sockaddr;
    socklen_t cnt_client_sockaddr;

    while(1)
    {
        cnt_user_events = epoll_wait(fd_ep,at_user_events,USER_COUNT+1,-1);

        if(cnt_user_events==-1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        int fd_newuser;
        int idx;
        for(int i=0;i<cnt_user_events;i++)
        {
            if(is_server_socket(at_user_events[i].data.fd))
            {
                    fd_newuser = accept_new_user(&idx);

                    if(fd_newuser == -1)
                    {
                        continue;
                    }
                    printf("Ajout d'un nouvel utiisateur à l'emplacement %d\n",
                            idx);

                    t_event.data.fd = fd_newuser;
                    t_event.events = EPOLLIN;
                    memset(at_userinfo + idx,0,sizeof(user_info_t));
                    at_userinfo[idx].b_used = 1;
                    epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd_newuser, &t_event);

                    send(fd_newuser, USER_WELCOME_MSG, USER_WELCOME_MSG_LEN,0);
            }
            // else // On parle avec un user connecté
            // {
            //     memset(ac_buffer, 0, BUFFER_SIZE);
            //     cnt_bytes_transfered = recv(at_user_events[i].data.fd,ac_buffer,BUFFER_SIZE,0);
            //     if(cnt_bytes_transfered==0)
            //     {
            //         continue;
            //     }
            //     char *pc_found = strpbrk(ac_buffer,"\r\n");
            //     if(pc_found)
            //     {
            //         *pc_found = '\0';
            //     }

            //     printf("%lu octets recus\n", cnt_bytes_transfered);

            //     int idx = get_idx_from_sockfd(at_user_events[i].data.fd, afd_user_sock, USER_COUNT);

            //     if(!at_userinfo[idx].b_authentified) // authentification
            //     {
            //         strncpy(at_userinfo[idx].ac_username, ac_buffer, USERNAME_LEN);
            //         at_userinfo[idx].b_authentified = 1;
            //         at_userinfo[idx].idx_speakto = -1;
            //         char *pzUserList;
            //         int nUserListLen = 0;
            //         nUserListLen = get_user_list_string(at_userinfo, USER_COUNT, &pzUserList);
            //         send(at_user_events[i].data.fd, pzUserList, nUserListLen,0);
            //         free_user_list_string(&pzUserList);
            //     }
            //     else if(at_userinfo[idx].b_authentified &&
            //         at_userinfo[idx].idx_speakto == -1) // connexion à un autre user
            //     {
            //         unsigned int idx_speakto;
            //         int ret;
            //         ret = sscanf(ac_buffer,"%u",&idx_speakto);
            //         if(ret!=EOF)
            //         {
            //             if(idx_speakto < USER_COUNT)
            //             {
            //                 memset(ac_buffer, 0, BUFFER_SIZE);
            //                 cnt_bytes_transfered = snprintf(ac_buffer, BUFFER_SIZE,
            //                                     "Vous parlez maintenant à %s\n",
            //                                     at_userinfo[idx_speakto].ac_username);
            //                 at_userinfo[idx].idx_speakto = idx_speakto;
            //                 send(afd_user_sock[idx],ac_buffer,cnt_bytes_transfered,0);
            //             }
            //         }
            //     }
            //     else // user authentifié et connecté à un autre user
            //     {
            //         if(!memcmp(QUIT, ac_buffer, strlen(QUIT)-1))
            //         {
            //             close_user_connection(idx, afd_user_sock, at_userinfo, USER_COUNT);
            //         }
            //         else 
            //         {
            //             const unsigned int idx_speakto = at_userinfo[idx].idx_speakto;
            //             send(afd_user_sock[idx_speakto], ac_buffer, cnt_bytes_transfered,0);
            //         }
            //     }

            // }
        }
    }
}
//  const unsigned int USER_COUNT = 10;
//     int afd_user_sock[USER_COUNT];
//     for(unsigned int i=0;i<USER_COUNT;++i)
//     {
//         afd_user_sock[i] = -1;
//     }

//     user_info_t at_userinfo[USER_COUNT];
//     memset(at_userinfo, 0, sizeof(at_userinfo));
    

//     struct epoll_event t_event;

//     struct epoll_event at_user_events[USER_COUNT+1];


//     const unsigned int BUFFER_SIZE = 256;
//     char ac_buffer[BUFFER_SIZE]; 
//     ssize_t cnt_bytes_transfered;

//     char ac_tosend[SRVMSG_MAXLEN];
//     memset(ac_tosend,0,SRVMSG_MAXLEN);

//     const char *QUIT = "/QUIT";

//     int fd_ep = epoll_create(11);