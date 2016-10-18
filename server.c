/*----------------------------------------------
Serveur à lancer avant le client
------------------------------------------------*/
#include <stdlib.h> 
#include <stdio.h> 
#include <linux/types.h> /* pour les sockets */ 
#include <sys/socket.h> 
#include <netdb.h> /* pour hostent, servent */ 
#include <string.h> /* pour bcopy, ... */ 
#include <pthread.h> 
#define TAILLE_MAX_NOM 256

# define TAILLE_MAX_CLIENT 256

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

typedef struct {
  int sock; /*socket du client*/
  char pseudo[50]; /*pseudo du client*/
  pthread_t thread; /*thread du serveur auquel est affecté le client*/
  int connected; /*booléen indiquant si le client est connecté ou non*/
}
Client;

Client threads[TAILLE_MAX_CLIENT];
int socket_descriptor; /* descripteur de socket */

pthread_t threadCmd;

int nbClientCo = 0;

int broadCastMessage(char msg[256]) {
  int i = 0;
  int vreturn = 0;
  for (i; i < sizeof(threads) / sizeof(threads[0]); ++i) {
    if (threads[i].connected == 1) {
      if (vreturn != -1 && write(threads[i].sock, msg, strlen(msg) + 1) < 0) {
        vreturn = -1;
      }

    }
  }

  return vreturn;

}

void closeConnections() {
  int i = 0;
  broadCastMessage("q");
  for (i; i < sizeof(threads) / sizeof(threads[0]); ++i) {
    if (threads[i].connected = 1) {
      close(threads[i].sock);
    }
  }
  if (close(socket_descriptor) < 0) {
    printf("Le socket du serveur ne s'est pas fermé.\n");
  } else {
    exit(0);
  }

}

static void * serverAction() {
  char mesg[256]; /* message envoyé */
  char retour[256];
  while (1) {
    memset(mesg, 0, 255);
    memset(retour, 0, 255);
    fgets(mesg, sizeof(mesg), stdin);
    mesg[strcspn(mesg, "\n")] = '\0';
    if (mesg[0] == 'q') {
      closeConnections();
    } else {
      sprintf(retour, "Le serveur a dit: %s \n", mesg);
      if ((broadCastMessage(retour)) < 0) {
        perror("erreur : impossible d'ecrire le message destine au serveur.");
        exit(0);
      }
    }
  }
}

/*------------------------------------------------------*/
static void * newClient(void * s) {
  Client * client = (Client * ) s;
  int sock = ( * client).sock;
  char buffer[256];
  char retour[256];
  int longueur;
  while (1) {
    memset(buffer, 0, 255);
    memset(retour, 0, 255);
    if ((longueur = read(sock, buffer, sizeof(buffer))) <= 0) {
      printf("message nul. \n");
      return;
    }
    sprintf(retour, "%s a dit: %s \n", ( * client).pseudo, buffer);
    printf("%s\n", retour);
    broadCastMessage(retour);
    //write(sock,retour,strlen(retour)+1);

  }
  close(sock);
  return;

}

/*------------------------------------------------------*/

/*------------------------------------------------------*/
main(int argc, char * * argv) {
  int nouv_socket_descriptor, /* [nouveau] descripteur de socket */
  longueur_adresse_courante; /* longueur d'adresse courante d'un client */
  sockaddr_in adresse_locale, /* structure d'adresse locale*/
  adresse_client_courant; /* adresse client courant */
  hostent * ptr_hote; /* les infos recuperees sur la machine hote */
  servent * ptr_service; /* les infos recuperees sur le service de la machine */
  char machine[TAILLE_MAX_NOM + 1]; /* nom de la machine locale */

  gethostname(machine, TAILLE_MAX_NOM); /* recuperation du nom de la machine */

  /* recuperation de la structure d'adresse en utilisant le nom */
  if ((ptr_hote = gethostbyname(machine)) == NULL) {
    perror("erreur : impossible de trouver le serveur a partir de son nom.");
    exit(1);
  }

  /* initialisation de la structure adresse_locale avec les infos recuperees */

  /* copie de ptr_hote vers adresse_locale */
  bcopy((char *)ptr_hote->h_addr, (char * ) & adresse_locale.sin_addr, ptr_hote->h_length);
  adresse_locale.sin_family = ptr_hote->h_addrtype; /* ou AF_INET */
  adresse_locale.sin_addr.s_addr = INADDR_ANY; /* ou AF_INET */

  adresse_locale.sin_port = htons(5000);

  printf("numero de port pour la connexion au serveur : %d \n",
    ntohs(adresse_locale.sin_port) /*ntohs(ptr_service->s_port)*/ );

  /* creation de la socket */
  if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("erreur : impossible de creer la socket de connexion avec le client.");
    exit(1);
  }

  /* association du socket socket_descriptor à la structure d'adresse adresse_locale */
  if ((bind(socket_descriptor, (sockaddr * )( & adresse_locale), sizeof(adresse_locale))) < 0) {
    perror("erreur : impossible de lier la socket a l'adresse de connexion.");
    exit(1);
  }

  /* initialisation de la file d'ecoute */
  listen(socket_descriptor, 5);

  pthread_create( & threadCmd, NULL, serverAction, NULL);

  /* attente des connexions et traitement des donnees recues */
  while (1) {

    longueur_adresse_courante = sizeof(adresse_client_courant);

    /* adresse_client_courant sera renseigné par accept via les infos du connect */
    if ((nouv_socket_descriptor =
        accept(socket_descriptor,
          (sockaddr * )( & adresse_client_courant), & longueur_adresse_courante)) < 0) {
      perror("erreur : impossible d'accepter la connexion avec le client.");
      exit(1);
    }

    threads[nbClientCo].sock = nouv_socket_descriptor;
    threads[nbClientCo].connected = 1;
    sprintf(threads[nbClientCo].pseudo, "joueur %d", nouv_socket_descriptor);
    printf("Démarrage connection avec un nouveau client \n");
    pthread_create( & threads[nbClientCo].thread, NULL, newClient, & threads[nbClientCo]);
    nbClientCo++;

  }

}

char * concat(const char * s1,
  const char * s2) {
  char * result = malloc(strlen(s1) + strlen(s2) + 1); //+1 for the zero-terminator
  //in real code you would check for errors in malloc here
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}