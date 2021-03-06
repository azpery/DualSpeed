/*----------------------------------------------
Serveur à lancer avant le client
------------------------------------------------*/
#include <stdlib.h> 
#include <stdbool.h> 
#include <stdio.h> 
#include <linux/types.h> /* pour les sockets */ 
#include <sys/socket.h> 
#include <netdb.h> /* pour hostent, servent */ 
#include <string.h> /* pour bcopy, ... */ 
#include <pthread.h> 
#include <time.h>


#define TAILLE_MAX_NOM 256

# define TAILLE_MAX_CLIENT 256

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

typedef struct {
  int id;   //Numéro de quesiton
  char question[256]; //Question posée
  char rep[256]; // Réponse correct
  char sugg[4][256]; //Tableau de toute les suggestions
}
Question;

typedef struct {
  int sock; /*socket du client*/
  char pseudo[50]; /*pseudo du client*/
  pthread_t thread; /*thread du serveur auquel est affecté le client*/
  int connected; /*booléen indiquant si le client est connecté ou non*/
  int score;
}
Client;

Client threads[TAILLE_MAX_CLIENT]; //Tableau de tous les clients
int socket_descriptor; /* descripteur de socket */
pthread_t threadCmd; //Thread de commande, permet d'écouter les interactions sur le serveur
int nbClientCo = 0; 
Question quizz[255]; //Tableau des questions
double mintime;
Client * winner;
Question currentQuestion;

unsigned int randint( unsigned int max)
{
    int r;
    unsigned int min = 1;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do
    {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}

/********************************************/
/***********Envoie un message à tous les*****/
/***********client connectés*****************/
/********************************************/

int broadCastMessage(char msg[256]) {
  int i = 0;
  int vreturn = 0;
  for (i; i < sizeof(threads) / sizeof(threads[0]); ++i) { //Parcours detous les utilisateurs
    if (threads[i].connected == 1) {
      if (vreturn != -1 && write(threads[i].sock, msg, strlen(msg) + 1) < 0) {
        vreturn = -1;
      }
    }
  }
  return vreturn;
}

bool isValueInArray(int val, int *arr, int size){
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return true;
    }
    return false;
}

/**********************************************/
/****Retourne le nombre de questions****/
/*********************************************/
int countNumberOfQuestions(){
  int i = 0;
  int vretour = 0;
  for (i; i < sizeof(quizz) / sizeof(quizz[0]); i++) {
    if (quizz[i].id > 0) {
      vretour++;
    }
  }
  return vretour++;
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

/**********************************************/
/****Retourne le nombre de suggestion****/
/*********************************************/
int countNumberOfSuggestion(Question q){
  int i = 0;
  int vretour = 0;
  for (i; i < sizeof(q.sugg) / sizeof(q.sugg[0]); i++)
  {
    if (q.sugg[i] != NULL  && strcmp(q.sugg[i], "") != 0)
    {
      vretour++;
    }
  }
  return vretour;
}

Question getRandomQuestion(int *questionsAsked, int nbOfQuestionsAsked){
  Question q = { 0, "", ""};;
  int r;
  while(q.id == 0){
    r = randint(countNumberOfQuestions()) - 1;
    if (!isValueInArray(r + 1, questionsAsked, countNumberOfQuestions()))
    {
      q = quizz[r];
      questionsAsked[nbOfQuestionsAsked + 1] = r + 1;
    }
  }
  return q;
}

char * getRandomSuggestion(Question question, int *suggestionsAsked, int nbOfSuggestionsAsked){
  char * q;
  bool found = false;
  int r = 0;
  while(!found){
    r = randint(countNumberOfSuggestion(question)) - 1;
    if (!isValueInArray(r + 1, suggestionsAsked, 4))
    {
      q = question.sugg[r];
      if (strcmp(q, "") != 0)
      {
        found = true;
        suggestionsAsked[nbOfSuggestionsAsked + 1] = r + 1;
      }
    }
  }
  strtok(q, "\n");
  return q;
}

/*********************************************/
/****Affiche toutes les questions chargées****/
/*********************************************/
void printQuizz(){
  int i = 0;
  int j = 0;
  for (i; i < sizeof(quizz) / sizeof(quizz[0]); i++) {
    if (quizz[i].id > 0) {
      j=0;
      printf("Question n°%d\n question:%s\n reponses:%s\n sugestions:",quizz[i].id, quizz[i].question, quizz[i].sugg[0]);
      for (j; j < sizeof(quizz[i].sugg) / sizeof(quizz[i].sugg[0]); j++)
      {
        if (quizz[i].sugg[j] != NULL)
        {
          printf("%s\n", quizz[i].sugg[j]);
        }
      }
    }
  }
}

/********************************************************/
/****Ferme toutes les connexions, et tous les threads****/
/****Ordonne aux clients de fermer leurs connexions******/
/********************************************************/
void closeConnections() {
  int i = 0;
  broadCastMessage("q");
  for (i; i < sizeof(threads) / sizeof(threads[0]); ++i) {
    if (threads[i].connected == 1) {
      printf("Fermeture de la co avec %s, état :%d\n", threads[i].pseudo, close(threads[i].sock));
    }
  }
  sleep(3);
  if (close(socket_descriptor) < 0) {
    printf("Le socket du serveur ne s'est pas fermé.\n");
  } else {
    printf("Le socket du serveur s'est fermé.\n");
    exit(1);
  }

}

int getMaxScore(){
  int vretour = 0;
  int i = 0;
  for (i; i < sizeof(threads) / sizeof(threads[0]); ++i) {
    if (threads[i].connected == 1 && threads[i].score > vretour) {
      vretour = threads[i].score;
    }
  }
  return vretour;
}


void getWinner(){
  Client win = {0};
  char * retour = concat("","");
  char pouet[256];
  int i = 0;
  for (i; i < sizeof(threads) / sizeof(threads[0]); ++i) {
    if (threads[i].connected == 1) {
      if ( threads[i].score > win.score)
      {
        win = threads[i];
      }
      sprintf(pouet,"|%s|score: %d|\n", threads[i].pseudo, threads[i].score);
      retour = concat(retour, pouet);
    }
  }
  sprintf(pouet, "****WINNER*****\n %s a gagné\n", win.pseudo);
  retour = concat(pouet, retour);
  broadCastMessage(retour);
}

/********************************************************/
/****Ecoute les messages à reçus par les clients*********/
/********************************************************/
static void * newClient(void * s) {
  Client * client = (Client * ) s;
  int sock = ( * client).sock;
  ( * client).score = 0;
  char buffer[256];
  char retour[256];
  double myTime=0.0;
  const char sut[2] = "||";
  int longueur;
  while (1) {
    memset(buffer, 0, 255);
    memset(retour, 0, 255);
    if ((longueur = read(sock, buffer, sizeof(buffer))) <= 0) {
      printf("message nul. \n");
      close(sock);
      return;
    }
    if (buffer[0] == 'q') {
      close(sock);
      sprintf(retour, "%s a quitté la partie.", ( * client).pseudo);
      printf("%s\n", retour);
      //broadCastMessage(retour);
      return;
    }
    sscanf(strtok(buffer, sut) , "%lf", &myTime);
    // sprintf(retour, "%s a dit: %s \n", ( * client).pseudo, buffer);
    if (myTime < mintime || mintime == 0)
    {
      if (strcmp(strtok(NULL, sut), currentQuestion.rep) == 0)
      {
        mintime = myTime;
        winner = client;
      }else{
        sprintf(retour, "MAUVAISE REPONSE %s perd un point\n", ( * client).pseudo);
        ( * client).score--;
        broadCastMessage(retour);
      }
    }
    printf("%s a réagi en : %lf\n",( * client).pseudo, myTime);
    printf("msg:%s\n", buffer);
    // broadCastMessage(retour);
  }
  close(sock);
  return;

}

/********************************************************/
/****Chargement des questions via le fichier*************/
/********************************************************/
void readFile(){
  const char s[2] = "|";
  int compteur = 1;
  int i = 0;
  const char sep[2] = ";";
  char * reponses;
  char *token;
  FILE *file;
  char * newLine;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  file = fopen("test.txt", "r");
  if (file == NULL)
    exit(EXIT_FAILURE);
  Question q;
  while ((read = getline(&line, &len, file)) != -1) {
    i = 0;
    newLine = line;
    token = strtok(newLine, s);
    quizz[compteur -1].id = compteur;
    sprintf(quizz[compteur - 1].question, "%s", token);
    token = strtok(NULL, s);
    token = strtok(token, sep);
    sprintf(quizz[compteur - 1].rep, "%s", token);
    while(token!= NULL){
      sprintf(quizz[compteur - 1].sugg[i], "%s", token);
      token = strtok(NULL, sep);
      i++;
    }
    compteur++;
  }
  fclose(file);
}

void playGame(){
  int scoreMax = 0;
  bool hasWinner = false;
  int questionsAsked[countNumberOfQuestions()];
  char * suggestion;
  char retour[256];
  int i = 0;
  int j = 0;
  int ok = 0;
  for (i; i < countNumberOfQuestions(); i++){
    if (getMaxScore() >= 5)
    {
      break;
    }
    currentQuestion = getRandomQuestion(questionsAsked, i);
    ok = broadCastMessage(currentQuestion.question);
    sleep(1);
    j = 0;
    int suggestionsAsked[countNumberOfSuggestion(currentQuestion)];
    memset(suggestionsAsked, 0, countNumberOfSuggestion(currentQuestion));
    for (j; j < countNumberOfSuggestion(currentQuestion); j++)
    {
      mintime = 0;
      suggestion = getRandomSuggestion(currentQuestion, suggestionsAsked, j);
      ok = broadCastMessage(suggestion);
      sleep(4);
      if (mintime>0)
      {
        sprintf(retour,"%s a été le plus rapide\n", ( * winner).pseudo);
        broadCastMessage(retour);
        sleep(1);
        j = countNumberOfSuggestion(currentQuestion);
        ( * winner).score++;
      }
    }
  }
  sleep(2);
  getWinner();
}

void printMenu(char mesg[256]) {
  printf(" ___________________Menu serveur__________________ \n");
  printf("|                                                 |\n");
  printf("|Afficher les questions chargées_________________1|\n");
  printf("|Recharger les questions_________________________2|\n");
  printf("|Commencer le jeu________________________________b|\n");
  printf("|Quitter_________________________________________q|\n");
  printf("|                                                 |\n");
  printf("|_________________________________________________|\n");
}

/********************************************************/
/****Ecoute des messages à envoyer par le serveur********/
/********************************************************/
static void * serverAction() {
  char mesg[256]; /* message à envoyé */
  char retour[256]; 
  while (1) {
    memset(mesg, 0, 255);
    memset(retour, 0, 255);
    printMenu(mesg);
    fgets(mesg, sizeof(mesg), stdin);
    mesg[strcspn(mesg, "\n")] = '\0';
    switch(mesg[0]) {
      case '1'  :
        printQuizz();
        break; 
      case 'q'  :
        closeConnections();
        break; 
      case 'b':
        playGame();
        break;  
      case '2':
        readFile();
        break;  
      default : 
        sprintf(retour, "Le serveur a dit: %s \n", mesg);
        if ((broadCastMessage(retour)) < 0) {
          perror("erreur : impossible d'ecrire le message destine au serveur.");
          exit(0);
        }
    }
  }
}

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
  adresse_locale.sin_port = htons(5001);
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
    close(socket_descriptor);
    exit(1);
  }
  /* initialisation de la file d'ecoute */
  listen(socket_descriptor, 5);
  pthread_create( & threadCmd, NULL, serverAction, NULL);
  readFile();
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