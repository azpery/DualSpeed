/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <message-a-transmettre>
------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>

typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;

pthread_t threadEcoute; 

static void * ecouteReponse(void * s)
{
    int * sock = (int *) s;
    char buffer[256];
    int longueur;
    while(1){
       memset(buffer, 0, 255);
       if ((longueur = read(*sock, buffer, sizeof(buffer))) <= 0) {
              printf("message nul. \n");
              close(sock);  
              exit(1);
       }
    
       printf("%s \n", buffer);

       /* mise en attente du prgramme pour simuler un delai de transmission */
    
       write(sock,buffer,strlen(buffer)+1);
    
    }  
    close(sock);  
    return;
}

int main(int argc, char **argv) {
  
    int 	socket_descriptor, 	/* descripteur de socket */
		longueur; 		/* longueur d'un buffer utilisé */
    sockaddr_in adresse_locale; 	/* adresse de socket local */
    hostent *	ptr_host; 		/* info sur une machine hote */
    servent *	ptr_service; 		/* info sur service */
    char 	buffer[256];
    char *	prog; 			/* nom du programme */
    char *	host; 			/* nom de la machine distante */
    char *	mesg = malloc(  256  ); ; 			/* message envoyé */
     
    if (argc != 2) {
	perror("usage : client <adresse-serveur>");
	exit(1);
    }
   
    prog = argv[0];
    host = argv[1];
    
    printf("nom de l'executable : %s \n", prog);
    printf("adresse du serveur  : %s \n", host);
    
    if ((ptr_host = gethostbyname(host)) == NULL) {
	perror("erreur : impossible de trouver le serveur a partir de son adresse.");
	exit(1);
    }
    
    /* copie caractere par caractere des infos de ptr_host vers adresse_locale */
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; /* ou ptr_host->h_addrtype; */
    

    adresse_locale.sin_port = htons(5000);
    /*-----------------------------------------------------------*/
    
    
    printf("numero de port pour la connexion au serveur : %d \n", ntohs(adresse_locale.sin_port));
    
    /* creation de la socket d'écoute */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("erreur : impossible de creer la socket de connexion avec le serveur.");
	exit(1);
    }

    /* Démarrage du thread d'écoute*/
    pthread_create(&threadEcoute, NULL, ecouteReponse, &socket_descriptor);

    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("erreur : impossible de creer la socket de connexion avec le serveur.");
    exit(1);
    }
    
    /* tentative de connexion au serveur dont les infos sont dans adresse_locale */
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
	perror("erreur : impossible de se connecter au serveur.");
	exit(1);
    }
    
    printf("connexion etablie avec le serveur. \n");
    
    while(1){
    
       fgets(mesg, sizeof(mesg), stdin);
       
       printf("envoi d'un message au serveur. \n%s \n", mesg);
       
       /* envoi du message vers le serveur */
       if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
	       perror("erreur : impossible d'ecrire le message destine au serveur.");
	       exit(1);
       }
    
       /* mise en attente du prgramme pour simuler un delai de transmission */
                     
       /* lecture de la reponse en provenance du serveur */
       //while((longueur = read(socket_descriptor, buffer, sizeof(buffer))) > 0) {
	       //printf("reponse du serveur : \n");
	       //write(1,buffer,longueur);
       //}
    
    }
    
    close(socket_descriptor);
    
    printf("connexion avec le serveur fermee, fin du programme.\n");
    
    exit(0);
    
}

