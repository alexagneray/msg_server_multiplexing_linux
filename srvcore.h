#define USER_COUNT 10
#define USERNAME_LEN 64
#define BUFFER_SIZE 256


// extern ssize_t cnt_bytes_transfered;

// extern char ac_tosend[SRVMSG_MAXLEN];

// extern const char *QUIT = "/QUIT";




int start_server(char *pz_port);
int server_loop();