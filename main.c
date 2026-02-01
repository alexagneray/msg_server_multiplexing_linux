#include "sockutils.h"
#include "srvcore.h"

#include <stdlib.h>

#include <stdio.h>

#include <unistd.h>



int main(int argc, char **argv)
{

    // ------------------------------------------------------------------------
    int i_opt;
    char *pz_port = NULL;
    int i_err = 0;

    // ------------------------------------------------------------------------


    while((i_opt = getopt(argc,argv,"p:h"))!=-1)
    {
        switch (i_opt)
        {
        case 'p':
            pz_port = optarg;
            break;
        case 'h':
            printf("%s -a address -p port\n", argv[0]);
            exit(EXIT_SUCCESS);
        default:
            break;
        }
    }

    i_err = start_server(pz_port);

    if(i_err == -1)
    {
        fprintf(stderr, "Erreur lors de l'initialisation du serveur\n");
        exit(EXIT_FAILURE);
    }
    printf("Initialisation du serveur sur le port : %s Achevée\n", pz_port);

    i_err = server_loop();

    
    exit(EXIT_SUCCESS);
}