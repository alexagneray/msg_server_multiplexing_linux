#include "sockutils.h"

#include <unistd.h>
#include <netdb.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int fd_server_sock;
int afd_user_sock[USER_COUNT];
char ac_buffer[USERMSG_MAXLEN];

/**
 * @brief Renvoie l'indice de la première valeur -1 du tableau.
 * Renvoie -1 si la valeur -1 n'est pas dans le tableau
 * 
 * @param ac_tocheck 
 * @param u_tab_size 
 * @return int 
 */
int find_empty_slot()
{
    for(unsigned int i=0;i<USER_COUNT;++i)
    {
        if(afd_user_sock[i]==-1)
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

void buffer_ncpy(char *p_dest, size_t max_len)
{
    assert(max_len <= USERMSG_MAXLEN);

    memcpy(p_dest, ac_buffer, max_len);
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

    for(int i=0;i<USER_COUNT;++i)
    {
        afd_user_sock[i] = -1;
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
    printf("Nouvelle connexion acceptée\n");
    if(fd_client_sock == -1)
    {
        perror("accept");
        return -1;
    }

    *idx = find_empty_slot();

    if(*idx==-1) // Il n'y a plus de slot dispo, on ferme la connexion
    {
        fprintf(stderr, "Plus de slot dispo pour un nouvel utilisateur\n");
        close(fd_client_sock);
        return -1;
    }

    afd_user_sock[*idx] = fd_client_sock;

    return fd_client_sock;
}

int recv_buffer(int fd_sock)
{
    assert(fd_sock != -1);

    ssize_t cnt_bytes_recv = recv(fd_sock, ac_buffer, USERMSG_MAXLEN, 0);
    if(cnt_bytes_recv == -1)
    {
        perror("recv");
        return -1;
    }
    else if(cnt_bytes_recv == 0)
    {
        // Connexion fermée par le client
        return 0;
    }

    printf("Reçu (%ld octets): %s\n", cnt_bytes_recv, ac_buffer);

    return cnt_bytes_recv;
}

unsigned int get_idx_from_sockfd(int fd_sock)
{
    assert(fd_sock != -1);

    for(unsigned int i=0;i<USER_COUNT;++i)
    {
        if(afd_user_sock[i]==fd_sock)
        {
            return i;
        }
    }
    return -1;
}

int close_user_sock(unsigned int idx_user)
{
    assert(idx_user < USER_COUNT);

    close(afd_user_sock[idx_user]);
    afd_user_sock[idx_user] = -1;

    return 0;
}

int send_to_connected_user(unsigned int idx_from, unsigned int idx_to, const char *pz_msg, size_t msg_len)
{
    assert(idx_from < USER_COUNT);
    assert(idx_to < USER_COUNT);
    assert(pz_msg != NULL);
    assert(msg_len <= USERMSG_MAXLEN);


    int fd_sock_to = afd_user_sock[idx_to];
    if(fd_sock_to == -1)
    {
        fprintf(stderr, "L'utilisateur %d n'est pas connecté (sockfd invalide)\n", idx_to);
        return -1;
    }

    ssize_t cnt_bytes_sent = send(fd_sock_to, pz_msg, msg_len, 0);
    if(cnt_bytes_sent == -1)
    {
        perror("send");
        return -1;
    }

    return cnt_bytes_sent;
}

int send_to_user(unsigned int idx_user, const char *pz_msg, size_t msg_len)
{
    assert(idx_user < USER_COUNT);
    assert(pz_msg != NULL);
    assert(msg_len <= USERMSG_MAXLEN);

    int fd_sock = afd_user_sock[idx_user];
    if(fd_sock == -1)
    {
        fprintf(stderr, "L'utilisateur %d n'est pas connecté (sockfd invalide)\n", idx_user);
        return -1;
    }

    ssize_t cnt_bytes_sent = send(fd_sock, pz_msg, msg_len, 0);
    if(cnt_bytes_sent == -1)
    {
        perror("send");
        return -1;
    }

    return cnt_bytes_sent;
}