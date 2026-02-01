/**
 * @file srvcore.h
 * @brief Core server operations.
 * 
 */

#define USER_COUNT 10
#define USERNAME_LEN 64
#define BUFFER_SIZE 256
#define QUIT "/QUIT"

/**
 * @brief Démarre le serveur sur le port spécifié.
 * 
 * @param pz_port 
 * @return int -1 en cas d'erreur, 0 sinon
 */
int start_server(char *pz_port);

/**
 *  @brief Boucle principale du serveur.
 *  @return int -1 en cas d'erreur, 0 sinon
 */
int server_loop();

/**
 * @brief Génère un string contenant la liste des utilisateurs actifs. 
 * La fonction alloue dynamiquement et mets à jour le pointeur ppz_user_list.
 * La mémoire allouée devra être libérée avec free_user_list_string()
 * 
 * @param at_userinfo 
 * @param cnt_userinfo 
 * @param ppz_user_list 
 */
int get_user_list_string(char **ppz_user_list);
/**
 * @brief Libère la mémoire allouée pour la liste des utilisateurs.
 * Cette fonction doit impérativement être appelée avec get_user_list_string
 * pour libérer la mémoire allouée dynamiquement !
 * 
 * @param ppz_user_list 
 */
void free_user_list_string(char **ppz_user_list);

/**
 * @brief Ferme la connexion d'un utilisateur et libère ses ressources.
 * 
 */
void close_user_connection(unsigned int idx_user);


