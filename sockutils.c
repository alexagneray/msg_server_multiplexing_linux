#include "sockutils.h"

#include <unistd.h>
#include <netdb.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int fd_server_sock;
int afd_user_sock[USER_COUNT];

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

int is_server_socket(int fd_sock)
{
    return (fd_sock == fd_server_sock);
}

int start_server_sock(char *pz_port)
{
    assert(pz_port != NULL);

    int i_err = -1;
    struct addrinfo t_hints;
    struct addrinfo *pt_res = NULL; 

    memset(&t_hints, 0, sizeof(t_hints));
    t_hints.ai_family = AF_INET;
    t_hints.ai_socktype = SOCK_STREAM;
    t_hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    i_err = getaddrinfo(NULL, pz_port, &t_hints, &pt_res);
    if(i_err == -1)
    {
        fprintf(stderr, "%s\n", gai_strerror(i_err));
        return -1;
    }

    fd_server_sock = socket(AF_INET, SOCK_STREAM, 0);

    i_err = bind(fd_server_sock, pt_res->ai_addr, pt_res->ai_addrlen);
    if(i_err == -1)
    {
        shutdown(fd_server_sock, SHUT_RDWR);
        close(fd_server_sock);
        perror("bind");
        return -1;
    }

    i_err = listen(fd_server_sock, 10);
    if(i_err == -1)
    {
        shutdown(fd_server_sock, SHUT_RDWR);
        close(fd_server_sock);
        perror("listen");
        return -1;
    }

    return fd_server_sock;
}

int accept_new_user(int *idx)
{
    assert(idx != NULL);    

    struct sockaddr_in t_client_addr;
    socklen_t sz_client_addr = sizeof(t_client_addr);
    int fd_client_sock = -1;

    fd_client_sock = accept(fd_server_sock,
                            (struct sockaddr *)&t_client_addr,
                            &sz_client_addr);
    if(fd_client_sock == -1)
    {
        perror("accept");
        return -1;
    }

    *idx = find_empty_slot(afd_user_sock, USER_COUNT);

    if(*idx==-1) // Il n'y a plus de slot dispo, on ferme la connexion
    {
        close(fd_client_sock);
        return -1;
    }

    afd_user_sock[*idx] = fd_client_sock;


    send(fd_client_sock, USER_WELCOME_MSG, USER_WELCOME_MSG_LEN,0);

    return fd_client_sock;
}