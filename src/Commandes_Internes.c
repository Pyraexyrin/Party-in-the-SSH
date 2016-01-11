#include <readline/history.h>
#include <signal.h>
#include <time.h>

#include "Commandes_Internes.h"

////////////////////////////////
// CHAR* COMMANDES_INTERNES[] //
//////////////////////////////////////////////////////////
// Liste des commandes internes gérées par le programme //
//////////////////////////////////////////////////////////

static const char* commandes_internes[] = {
  "echo",
  "date",
  "cd",
  "pwd",
  "history",
  "hostname",
  "kill",
  "exit",
  "remote",
  "majora",
  NULL
};

////////////////////////////////////////////////
// VOID INTERNE_<commande>(EXPRESSION*, INT*) //
//////////////////////////////////////////////////////////////////////
// Pour chaque <commande>, une fonction lui est dédiée, qui exécute //
// l'équivalent de cette commande bash.                             //
// Les valeurs données à status sont :                              //
// - 0 en cas de réussite ;                                         //
// - Un entier positif en cas d'erreur, dans l'ordre du programme   //
//   (la première erreur pouvant être rencontrée donnera 1, la      //
//   deuxième donnera 2, ainsi de suite).                           //
//////////////////////////////////////////////////////////////////////

// echo : simple affichage des arguments

static void
interne_echo (Expression * e, int * status) {
  char ** s = e->arguments;
  s++; // Ne pas garder "echo"
  while (*s) {
    fprintf(stdout, "%s ", *s++); // fprintf ne demande pas la taille, contrairement à write
  }
  fprintf(stdout, "\n");

  // echo n'a pas d'erreur possible
  *status = 0;
}

// date : utilisation de time() et localtime()

static const char* jours[] = {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"};
static const char* mois[] = {"janvier", "février", "mars", "avril", "mai", "juin", "juillet",
			     "août", "septembre", "octobre", "novembre", "décembre"};

static void
interne_date (Expression * e, int * status) {
  time_t t;
  time(&t);

  struct tm ltime = *localtime(&t);
  struct tm gtime = *gmtime(&t); // Pour calculer le décalage horaire par rapport à UTC

  
  fprintf(stdout, "%s %d %s %d, %d:%d:%d (UTC+%02d%02d)\n", jours[ltime.tm_wday], ltime.tm_mday, mois[ltime.tm_mon],
	  ltime.tm_year + 1900, ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
	  (ltime.tm_hour-gtime.tm_hour), (ltime.tm_min-gtime.tm_min));

  *status=0;
  // J'avoue, j'ai pas testé les erreurs possibles (localtime ou gmtime renvoie NULL)
}

// cd : chdir() fait tout le travail

static void
interne_cd (Expression * e, int * status) {
  if (e->arguments[1]==NULL || e->arguments[2]!=NULL){
    *status = 1;
    fprintf(stderr, "Erreur : cd prend un chemin en paramètre (cd <chemin>).\n");
    return;
  }
  if (chdir(e->arguments[1])==-1){
    fprintf(stderr, "Erreur d'exécution.\n");
    *status = 1;
    return;
  }
  *status = 0;
}

// pwd : getcwd() fait exactement ce qu'on veut (donne le chemin absolu)

static void
interne_pwd (Expression * e, int * status) {
  if (e->arguments[1]!=NULL){
    *status = 1;
    fprintf(stderr, "Erreur : pwd ne prend pas de paramètre (pwd).\n");
    return;
  }
  char* buf = getcwd(NULL, 0);
  if (buf == NULL){
    *status = 2;
    fprintf(stderr, "Erreur d'exécution.\n");
    return;
  }
  *status = 0;
  fprintf(stdout, "%s\n", buf);
  free(buf);
}

// history : il faut jouer avec les fonctions de l'interface readline/history.h

static void
interne_history (Expression * e, int * status) {
  int pos = where_history();
  if (pos<0){
    fprintf(stderr, "Erreur d'exécution.\n");
    *status = 1;
    return;
  }
  HIST_ENTRY * hist;
  for(int i=0; i<=pos; i++){
    hist = history_get(i);
    if (hist != NULL)
      fprintf(stdout, "%5d  %s\n", i, hist->line);
  }

  *status = 0;
}

// hostname : là encore, gethostname() fait tout le travail

static void
interne_hostname (Expression * e, int * status) {
  char name[1024];
  gethostname(name, 1024);
  name[1023]='\0';
  fprintf(stdout, "%s\n", name);

  *status = 0;
}

// kill : on doit d'abord vérifier que les paramètres ne sont que des pids (nombres positifs)

static void
interne_kill (Expression * e, int * status) {
  if (e->arguments[1]==NULL){
    fprintf(stderr, "Erreur : kill prend au moins un pid en paramètre (kill <pid1> <pid2>...).\n");
    *status = 1;
    return;
  }
  int i = 1;
  int j;
  char c;
  while (e->arguments[i]!=NULL){ // Vérification des arguments (tous des nombres)
    c = e->arguments[i++][j];
    for (j=0; c; j++){
      if (!(c>='0' && c<='9')){
	fprintf(stderr, "Erreur : kill prend en paramètres des pids entiers.\n");
	*status = 2;
	return;
      }
    }
  }
  i=1;
  while (e->arguments[i]!=NULL){
    kill(atoi(e->arguments[i++]), 15);
  }
  *status = 0;
}

// exit : la violence pure !

static void
interne_exit (Expression * e, int * status) {
  *status = 0;
  exit(0);
}

// Pour le cas "remote", toutes les fonctions lui étant dédiées
// sont en fin de fichier.

static void remote_main (Expression * e, int * status);

static void
interne_remote (Expression * e, int * status) {
  remote_main(e, status);
}

// majora : c'est juste une commande pour le fun. Elle affiche le temps écoulé depuis la date et heure de rendu du projet.

static void
interne_majora (Expression * e, int * status) {
  time_t t;
  time(&t);

  struct tm ltime = *localtime(&t);
   
  fprintf(stdout, "\x1b[01;31m\n\tDAWN OF THE DAY %d\n\t %d hours ellapsed\n\x1b[0m\n", (ltime.tm_yday-10), (24*(ltime.tm_yday-11)+ltime.tm_hour));
}

////////////////////////////////////
// INT CHECK_INTERNE(EXPRESSION*) //
///////////////////////////////////////////////////////////////////////////////
// Vérifie si une commande est interne (compare la commande de e à la liste) //
///////////////////////////////////////////////////////////////////////////////

static int
check_interne(Expression * e){
  int ind = 0;
  while (commandes_internes[ind]){
    if (strcmp(e->arguments[0], commandes_internes[ind]) == 0)
      return ind;
    ind++;
  }
  return -1;
}

//////////////////////////////////////////////
// BOOL EXECUTER_INTERNE(EXPRESSION*, INT*) //
////////////////////////////////////////////////////////////////////////
// Choisit le traitement approprié aux différentes commandes internes //
// (retourne false si ce n'est pas une commande interne)              //
////////////////////////////////////////////////////////////////////////

bool
executer_interne(Expression * e, int * status){
  int cmd = check_interne(e);
  switch (cmd) {

  case 0 :
    interne_echo(e, status);
    break;
    
  case 1 :
    interne_date(e, status);
    break;
    
  case 2 :
    interne_cd(e, status);
    break;
    
  case 3 :
    interne_pwd(e, status);
    break;
    
  case 4 :
    interne_history(e, status);
    break;
    
  case 5 :
    interne_hostname(e, status);
    break;
    
  case 6 :
    interne_kill(e, status);
    break;
    
  case 7 :
    interne_exit(e, status);
    break;
    
  case 8 :
    interne_remote(e, status);
    break;

  case 9 :
    interne_majora(e, status);
    break;
    
  default : // Cas "ce n'est pas une commande interne"
    return false;
    
  }

  return true;
  
}

/////////////////////////////////////////
// VOID REMOTE_MAIN(EXPRESSION*, INT*) //
////////////////////////////////////////////////////////////////
// Point d'entrée du traitement de la commande interne remote //
////////////////////////////////////////////////////////////////

static void
remote_main (Expression * e, int * status){
  fprintf(stderr,"Sorry, but this command is still in work.\nYou've met with a terrible fate, haven't you ?\n");

  

  /*
  int fd[2];

  if (pipe(fd)==-1){
    fprintf(stderr, "Erreur d'exécution.\n");
    *status = 1;
    return;
  }
  
  if (fork() == 0){
    dup2(fd[0], 0);
    int rc;
    char buf[1024];
    
    
    char* list[] = {"ssh", "adubroca@jaguar.emi.u-bordeaux.fr"};
    execvp("ssh", list);
    fprintf(stderr, "Erreur de ssh.\n");
        

    // LECTURE DE L'ENTRÉE, ET REPRODUCTION SUR LA SORTIE
    while(rc = read (0, buf, 1024)){
      write(1, buf, rc);
    }

    fprintf(stdout, "Fils fini.\n");
  }
  
  else {
    FILE* stream = fdopen(fd[1], "w");
    char buf[1024];
    memset(buf, '\0', 1024);
    setvbuf(stream, buf, _IONBF, 1024);

    while(read (0, buf, 1024)){
      fputs(buf, stream);
      fputs(buf, stdout);
      memset(buf, '\0', 1024);
    }

    fprintf(stderr, "Après les fputs.\n");

    // ENVOI DE LA COMMANDE-ARGUMENT DANS LE BUFFER
    int i = 1;
    while (e->arguments[i] != NULL){
      fputs(*(e->arguments + (i++)), stream);
      fputs(" ", stream);
    }

    close(0);
    while(1){}
    
    
    fprintf(stderr, "You've leaving the Remote Zone.\n");
    
  }
  */
  
}
