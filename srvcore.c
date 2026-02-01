#include "srvcore.h"
#include "sockutils.h"
#include <netdb.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <assert.h>


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

int fd_ep;


int get_user_list_string(char **ppz_user_list)
{
    const unsigned int USER_LABEL_SIZE = USERID_LEN + USERNAME_LEN + 3; // userid - USERNAME
    *ppz_user_list = malloc(USER_LABEL_SIZE * USER_COUNT);
    int offset = 0;
    for(unsigned int i=0;i<USER_COUNT;++i)
    {
        if(at_userinfo[i].b_used)
        {
            offset += sprintf((*ppz_user_list)+offset, "%d - %s\n", i,at_userinfo[i].ac_username);
        }
    }
    return offset;
}


void free_user_list_string(char **ppz_user_list)
{
    if(*ppz_user_list)
    {
        free(*ppz_user_list);
        *ppz_user_list = NULL;
    }
    
}

int start_server(char *pz_port)
{
    int fd_sock = start_server_sock(pz_port);

    if(fd_sock == -1)
    {
        fprintf(stderr, "Erreur lors de la création du socket serveur\n");
        return -1;
    }      
    
    epoll_create1(0);
    fd_ep = epoll_create(11);
    t_event.data.fd = fd_sock;
    t_event.events = EPOLLIN;
    epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd_sock, &t_event);

    memset(at_userinfo, 0, sizeof(at_userinfo));
    for(int i=0;i<USER_COUNT;++i)
    {
        at_userinfo[i].idx_speakto = -1;
    }

    return fd_sock;
}


void close_user_connection(unsigned int idx_user)
{
    assert(idx_user < USER_COUNT);

    close_user_sock(idx_user);
    memset(&at_userinfo[idx_user], 0, sizeof(user_info_t));
    at_userinfo[idx_user].idx_speakto = -1;
}




int server_loop()
{
    int cnt_user_events = 0;

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
        char ac_buffer[BUFFER_SIZE];
        for(int i=0;i<cnt_user_events;i++)
        {
            memset(ac_buffer, 0, BUFFER_SIZE);
            if(is_server_socket(at_user_events[i].data.fd))
            {
                printf("Gestion d'une nouvelle connexion utilisateur\n");
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
                send_to_user(idx, USER_WELCOME_MSG, USER_WELCOME_MSG_LEN);
            }
            else // On parle avec un user connecté
            {
                int cnt_ret = recv_buffer(at_user_events[i].data.fd);
                buffer_ncpy(ac_buffer, cnt_ret);

                unsigned int idx = get_idx_from_sockfd(at_user_events[i].data.fd);
            
                if(cnt_ret <= 0)
                {
                    close_user_connection(idx);
                    continue;
                }

                printf("%d octets recus\n", cnt_ret);

                if(!at_userinfo[idx].b_authentified) // utilisateur connecté mais non authentifié
                {
                    printf("Authentification de l'utilisateur %d avec le nom : %s\n",
                            idx, ac_buffer);
                    strncpy(at_userinfo[idx].ac_username, ac_buffer, USERNAME_LEN);
                    at_userinfo[idx].b_authentified = 1;
                    at_userinfo[idx].idx_speakto = -1;

                    char *pzUserList;
                    int nUserListLen = 0;
                    nUserListLen = get_user_list_string(&pzUserList);
                    send(at_user_events[i].data.fd, pzUserList, nUserListLen,0);
                    free_user_list_string(&pzUserList);
                }
                else if(at_userinfo[idx].b_authentified &&
                    at_userinfo[idx].idx_speakto == -1) // utilisateur authentifié mais non connecté à un autre user
                {
                    unsigned int idx_speakto;
                    int ret;
                    unsigned int cnt_bytes_transfered;
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
                            send_to_user(idx, ac_buffer, cnt_bytes_transfered);
                        }
                    }
                }
                else // user authentifié et connecté à un autre user
                {
                    if(!memcmp(QUIT, ac_buffer, strlen(QUIT)-1))
                    {
                        close_user_connection(idx);
                    }
                    else 
                    {
                        cnt_ret = send_to_connected_user(idx, at_userinfo[idx].idx_speakto, ac_buffer, cnt_ret);
                    }
                }

            }
        }
    }
}