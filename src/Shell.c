/* Construction des arbres représentant des commandes */

#include "Shell.h"
#include "Affichage.h"
#include "Commandes_Internes.h"
#include "Evaluation.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

extern int yyparse_string(char *);

bool interactive_mode = 1; // par défaut on utilise readline
int status = 0; // valeur retournée par la dernière commande

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
      snprintf(buffer, 1024, "[%d] mini_shell > ", status);
      line = readline(buffer);
      if(line != NULL)
	{
	  int ret;
	  add_history(line);              // Enregistre la line non vide dans l'historique courant
	  *strchr(line, '\0') = '\n';      // Ajoute \n à la line pour qu'elle puisse etre traité par le parseur
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



/*--------------------------------------------------------------------------------------.
| Lorsque l'analyse de la ligne de commande est effectuée sans erreur. La variable      |
| globale ExpressionAnalysee pointe sur un arbre représentant l'expression.  Le type    |
|       "Expression" de l'arbre est décrit dans le fichier Shell.h. Il contient 4       |
|       champs. Si e est du type Expression :					        |
| 										        |
| - e.type est un type d'expression, contenant une valeur définie par énumération dans  |
|   Shell.h. Cette valeur peut être :					      	        |
| 										        |
|   - VIDE, commande vide							        |
|   - SIMPLE, commande simple et ses arguments					        |
|   - SEQUENCE, séquence (;) d'instructions					        |
|   - SEQUENCE_ET, séquence conditionnelle (&&) d'instructions			        |
|   - SEQUENCE_OU, séquence conditionnelle (||) d'instructions			        |
|   - BG, tâche en arrière plan (&)						        |
|   - PIPE, pipe (|).								        |
|   - REDIRECTION_I, redirection de l'entrée (<)					|
|   - REDIRECTION_O, redirection de la sortie (>)			       	        |
|   - REDIRECTION_A, redirection de la sortie en mode APPEND (>>).		        |
|   - REDIRECTION_E, redirection de la sortie erreur,  	   			        |
|   - REDIRECTION_EO, redirection des sorties erreur et standard.		        |
| 										        |
| - e.gauche et e.droite, de type Expression *, représentent une sous-expression gauche |
|       et une sous-expression droite. Ces deux champs ne sont pas utilisés pour les    |
|       types VIDE et SIMPLE. Pour les expressions réclamant deux sous-expressions      |
|       (SEQUENCE, SEQUENCE_ET, SEQUENCE_OU, et PIPE) ces deux champs sont utilisés     |
|       simultannément.  Pour les autres champs, seule l'expression gauche est	        |
|       utilisée.									|
| 										        |
| - e.arguments, de type char **, a deux interpretations :			        |
| 										        |
|      - si le type de la commande est simple, e.arguments pointe sur un tableau à la   |
|       argv. (e.arguments)[0] est le nom de la commande, (e.arguments)[1] est le	|
|       premier argument, etc.							        |
| 										        |
|      - si le type de la commande est une redirection, (e.arguments)[0] est le nom du  |
|       fichier vers lequel on redirige.                                                |
`--------------------------------------------------------------------------------------*/


// EXECUTER L'EXPRESSION //

int
executer_SIMPLE(Expression * e){
  
  if (!executer_interne(e, &status)){

    if (fork() == 0){
      execvp(e->arguments[0], e->arguments);
    }
    else {
      wait(&status);
    }

  }
  
  return status;
  
}

int
executer_expression(Expression * e){

  int fd; // Cas simples
  int fd_pipe[2], backup[2]; // Pipes et redirections multiples
  
  switch (e->type) {

  case VIDE :
    status = 0;
    break;
    
  case SIMPLE :
    executer_SIMPLE(e);
    break;

  case SEQUENCE :
    executer_expression(e->gauche);
    executer_expression(e->droite);
    break;

  case SEQUENCE_ET :
    executer_expression(e->gauche);
    if (status == 0)
      executer_expression(e->droite);
    break;

  case SEQUENCE_OU :
    executer_expression(e->gauche);
    if (status != 0)
      executer_expression(e->droite);
    break;
    
  case BG :
    if (fork() == 0)
      executer_expression(e->gauche);
    break;

  case PIPE :
    pipe(fd_pipe);
    
    backup[1] = dup(1);
    dup2(fd_pipe[1], 1);
    executer_expression(e->gauche);
    close(fd_pipe[1]);
    dup2(backup[1], 1);
    
    backup[0] = dup(0);
    dup2(fd_pipe[0], 0);
    executer_expression(e->droite);
    dup2(backup[0], 0);

    break;
    
  case REDIRECTION_I :
    fd = dup(0);
    dup2(open(e->arguments[0], 0), 0);
    executer_expression(e->gauche);
    dup2(fd, 0);
    break;
    
  case REDIRECTION_O :
    fd = dup(1);
    dup2(open(e->arguments[0], O_CREAT|O_TRUNC|O_WRONLY, 0x664), 1);
    executer_expression(e->gauche);
    dup2(fd, 1);
    break;
    
  case REDIRECTION_A :
    fd = dup(1);
    dup2(open(e->arguments[0], O_CREAT|O_APPEND|O_WRONLY, 0x664), 1);
    executer_expression(e->gauche);
    dup2(fd, 1);
    break;
    
  case REDIRECTION_E :
    fd = dup(2);
    dup2(open(e->arguments[0], O_CREAT|O_TRUNC|O_WRONLY, 0x664), 1);
    executer_expression(e->gauche);
    dup2(fd, 2);
    break;
    
  case REDIRECTION_EO :
    backup[0] = dup(2);
    backup[1] = dup(1);
    fd = open(e->arguments[0], O_CREAT|O_TRUNC|O_WRONLY, 0x664);
    dup2(fd, 1);
    dup2(fd, 2);
    executer_expression(e->gauche);
    dup2(backup[0], 2);
    dup2(backup[1], 1);
    break;
    
  default :
    break;
  }
  
  return status;
    
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
