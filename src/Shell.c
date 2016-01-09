/* Construction des arbres représentant des commandes */

#include "Shell.h"
#include "Affichage.h"
#include "Commandes_Internes.h"
#include "Evaluation.h"

#include <stdbool.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

//////////
// DATA //
//////////

extern int yyparse_string(char *);

bool interactive_mode = 1; // par défaut on utilise readline
int status = 0; // valeur retournée par la dernière commande

///////////////////////
// FONCTIONS DONNÉES //
///////////////////////

/*
 * Construit une expression à partir de sous-expressions
 */

Expression *ConstruireNoeud (expr_t type, Expression *g, Expression *d, char **args) {
  Expression *e;

  if ((e = (Expression *)malloc(sizeof(Expression))) == NULL){
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  e->type = type;
  e->gauche = g;
  e->droite = d;
  e->arguments = args;
  return e;
}

/*
 * Renvoie la longueur d'une liste d'arguments
 */

int LongueurListe(char **l) {
  char **p;
  for (p=l; *p != NULL; p++)
    ;
  return p-l;
}

/*
 * Renvoie une liste d'arguments, la première case étant initialisée à NULL, la
 * liste pouvant contenir NB_ARGS arguments (plus le pointeur NULL de fin de
 * liste)
 */

char **InitialiserListeArguments (void) {
  char **l;

  l = (char **) (calloc (NB_ARGS+1, sizeof (char *)));
  *l = NULL;
  return l;
}

/*
 * Ajoute en fin de liste le nouvel argument et renvoie la liste résultante
 */

char **AjouterArg (char **Liste, char *Arg) {
  char **l;

  l = Liste + LongueurListe (Liste);
  *l = (char *) (malloc (1+strlen (Arg)));
  strcpy (*l++, Arg);
  *l = NULL;
  return Liste;
}

/*
 * Fonction appelée lorsque l'utilisateur tape "".
 */

void EndOfFile (void)
{
  exit (0);
}

/*
 * Appelée par yyparse() sur erreur syntaxique
 */

void yyerror (char *s)
{
  fprintf(stderr, "%s\n", s);
}


/*
 * Libération de la mémoire occupée par une expression
 */

void
expression_free(Expression *e)
{
  if (e == NULL)
    return;

  expression_free(e->gauche);
  expression_free(e->droite);

  if (e->arguments != NULL)
    {
      for (int i = 0; e->arguments[i] != NULL; i++)
	free(e->arguments[i]);
      free(e->arguments);
    }

  free(e);
}

/*
 * Lecture de la ligne de commande à l'aide de readline en mode interactif
 * Mémorisation dans l'historique des commandes
 * Analyse de la ligne lue
 */

int
my_yyparse(void)
{
  if (interactive_mode)
    {
      char *line = NULL;
      char buffer[1024];
      snprintf(buffer, 1024, "\x1b[01;33m[%d] \x1b[01;34mTermina \x1b[01;33m> \x1b[0m", status);
      line = readline(buffer);
      if(line != NULL)
	{
	  int ret;
	  add_history(line);              // Enregistre la line non vide dans l'historique courant
	  *strchr(line, '\0') = '\n';     // Ajoute \n à la line pour qu'elle puisse etre traité par le parseur
	  ret = yyparse_string(line);     // Remplace l'entrée standard de yyparse par s
	  free(line);
	  return ret;
	}
      else
	{
	  EndOfFile();
	  return -1;
	}
    }
  else
    {
      // pour le mode distant par exemple

      int ret; int c;

      char *line = NULL;
      size_t linecap = 0;
      ssize_t linelen;
      linelen = getline(&line, &linecap, stdin);

      if(linelen>0)
	{
	  int ret;
	  ret = yyparse_string(line);
	  free(line);
	  return ret;
	}
    }
}



//////////
// MAIN //
//////////

int
main (int argc, char **argv)
{

  // faire en sorte qu'interactive_mode = 0 lorsque le shell est distant

  if (interactive_mode)
    {
      using_history();
    }
  else
    {
      //  mode distant
    }

  while (1){
    if (my_yyparse () == 0) {  /* L'analyse a abouti */
      // afficher_expr(ExpressionAnalysee);
      status = 0; // On réinitialise le statut
      executer_expression(ExpressionAnalysee);
      fflush(stdout);
      expression_free(ExpressionAnalysee);
    }
    else {
      /* L'analyse de la ligne de commande a donné une erreur */
    }
  }
  return 0;
}
