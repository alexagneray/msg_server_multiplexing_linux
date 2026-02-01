/**
 * @file sockutils.h
 * @brief Socket utilities for server operations. Manipule les sockets et connexions utilisateurs.
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

/**
 * @brief Démarre le socket serveur sur le port spécifié, et initialise les slots utilisateurs.
 * 
 * @param pz_port 
 * @return int -1 en cas d'erreur, le descripteur du socket serveur sinon
 */
int start_server_sock(char *pz_port);

/**
 * @brief Accepte une nouvelle connexion utilisateur.
 * 
 * @param idx Pointeur vers l'indice utilisateur alloué.
 * @return int -1 en cas d'erreur, le descripteur du socket client sinon    
 */
int accept_new_user(int *idx);

/**
 * @brief Vérifie si le descripteur de socket fourni est celui du serveur.
 * 
 * @param fd_sock 
 * @return int 1 si c'est le socket serveur, 0 sinon
 */
int is_server_socket(int fd_sock);

/**
 * @brief Reçoit des données sur le socket spécifié et les stocke dans le buffer global.
 * 
 * @param fd_sock 
 * @return int Nombre d'octets reçus, 0 si la connexion est fermée, -1 en cas d'erreur
 */
int recv_buffer(int fd_sock);

/**
 * @brief Récupère l'indice utilisateur associé au descripteur de socket donné.
 */
unsigned int get_idx_from_sockfd(int fd_sock);

/**
 * @brief Ferme la connexion utilisateur à l'indice spécifié.
 */
int find_empty_slot();

/**
 * @brief Ferme la connexion utilisateur à l'indice spécifié.
 * 
 * @param idx_user 
 * @return int 
 */
int close_user_sock(unsigned int idx_user);

/**
 * @brief Envoie un message d'un utilisateur connecté à un autre utilisateur connecté.
 */
void buffer_ncpy(char *p_dest, size_t max_len);

/**
 * @brief Envoie un message d'un utilisateur connecté à un autre utilisateur connecté.  
 */
int send_to_connected_user(unsigned int idx_from, unsigned int idx_to, const char *pz_msg, size_t msg_len);

/**
 * @brief Envoie un message à l'utilisateur spécifié par son indice.
 */
int send_to_user(unsigned int idx_user, const char *pz_msg, size_t msg_len);

