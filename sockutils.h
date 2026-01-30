#include <sys/types.h>
#include <sys/socket.h>


#define USERID_LEN 4

#define USERMSG_MAXLEN 4096
#define SRVMSG_MAXLEN 8192
#define USER_COUNT 10

#define USER_WELCOME_MSG "Bienvenue sur le serveur de discussion, quel est votre nom ?\n"
#define USER_WELCOME_MSG_LEN sizeof(USER_WELCOME_MSG)


int start_server_sock(char *pz_port);
int accept_new_user(int *idx);
int is_server_socket(int fd_sock);