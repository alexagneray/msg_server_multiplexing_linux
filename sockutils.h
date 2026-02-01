/**
 * @file sockutils.h
 * @brief Socket utilities for server operations. 
 * 
 */

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
int recv_buffer(int fd_sock);
unsigned int get_idx_from_sockfd(int fd_sock);
int find_empty_slot();
int close_user_sock(unsigned int idx_user);
void buffer_ncpy(char *p_dest, size_t max_len);
int send_to_connected_user(unsigned int idx_from, unsigned int idx_to, const char *pz_msg, size_t msg_len);
int send_to_user(unsigned int idx_user, const char *pz_msg, size_t msg_len);