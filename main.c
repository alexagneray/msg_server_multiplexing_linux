#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <stdio.h>
#include <string.h>

#include <fcntl.h>

/**
 * @brief Renvoie l'indice de la première valeur -1 du tableau.
 * Renvoie -1 si la valeur -1 n'est pas dans le tableau
 * 
 * @param tab 
 * @param tabSize 
 * @return int 
 */
int findEmptySlot(int tab[], unsigned int tabSize)
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

int main(int argc, char **argv)
{
    int opt;
    char *pzPort = NULL;
    int sockfd;
    int err;

    struct addrinfo hints;
    struct addrinfo *pRes;

    struct sockaddr cltSockAddr;
    socklen_t cltSockAddrLen;

    const unsigned int SOCKET_COUNT = 10;
    int csockfd[SOCKET_COUNT];
    for(unsigned int i=0;i<SOCKET_COUNT;++i)
    {
        csockfd[i] = -1;
    }

    struct epoll_event ev;

    struct epoll_event events[SOCKET_COUNT+1];


    const unsigned int BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE]; 
    ssize_t nBytes;

    const char * QUIT = "QUIT";

    int epfd = epoll_create(11);
    if(epfd < 0)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    int n;



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
        n = epoll_wait(epfd,events,SOCKET_COUNT+1,-1);

        if(n==-1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for(int i=0;i<n;i++)
        {
            // printf("%d",i);
            if(events[i].data.fd==sockfd)
            {
                printf("Un nouveau client veut se connecter !\n");
                int newsock;
                newsock = accept(sockfd,&cltSockAddr,&cltSockAddrLen);
                int idx = findEmptySlot(csockfd, SOCKET_COUNT);
                if(idx==-1)
                {
                    close(newsock);
                }
                else
                {
                    printf("Ajout d'un nouveau client à l'emplacement %d\n", idx);
                    csockfd[idx] = newsock;
                    ev.data.fd = newsock;
                    ev.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, csockfd[idx], &ev);
                }
            }
            else
            {
                memset(buffer, 0, BUFFER_SIZE);
                nBytes = recv(events[i].data.fd,buffer,BUFFER_SIZE,0);
                if(nBytes==0)
                {
                    continue;
                }
                printf("%lu octets recus\n", nBytes);
                // fwrite(buffer,1,nBytes,stdout);
                if(!memcmp(QUIT, buffer, strlen(QUIT)-1))
                {
                    printf("ON QUITTE\n");
                }
            }
        }
    }
    
    


    // shutdown(csockfd, SHUT_RDWR);
    shutdown(sockfd, SHUT_RDWR);
    // close(csockfd);
    close(sockfd);
    
    exit(EXIT_SUCCESS);
}