#include "Commandes_Internes.h"
#include <time.h>
#include <unistd.h>
#include <readline/history.h>
#include <signal.h>

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
  
}

// cd : chdir() fait tout le travail

static void
interne_cd (Expression * e, int * status) {
  if (e->arguments[1]==NULL || e->arguments[2]!=NULL){
    *status = 1;
    fprintf(stderr, "Erreur : cd prend un chemin en paramètre (cd <chemin>).\n");
    return;
  }
  chdir(e->arguments[1]);
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
    *status = 1;
    fprintf(stderr, "Erreur d'exécution.\n");
    return;
  }
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
  exit(0);
}

// Pour le cas "remote", toutes les fonctions lui étant dédiées
// sont en fin de fichier.

static void remote_main (Expression * e, int * status);

static void
interne_remote (Expression * e, int * status) {
  remote_main(e, status);
}

// majora : c'est juste une commande pour le fun. Elle est longue donc à la fin

static void
interne_majora (Expression * e, int * status);

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
}

////////////////////////////////////////////
// VOID INTERNE_MAJORA(EXPRESSION*, INT*) //
/////////////////////////////////////////////////////////////////////////
// Affiche une masque de majora coloré (si le terminal est compatible. //
// S'il ne l'est pas, affiche de la purée.                             //
/////////////////////////////////////////////////////////////////////////

static void
interne_majora (Expression * e, int * status) {
  time_t t;
  time(&t);

  struct tm ltime = *localtime(&t);
   
  fprintf(stdout, "\x1b[01;31m\n\tDAWN OF THE DAY %d\n\t %d hours ellapsed\n\x1b[0m\n", (ltime.tm_yday-9), (24*(ltime.tm_yday-10)+ltime.tm_hour));
}
