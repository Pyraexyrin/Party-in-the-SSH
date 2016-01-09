#include "Shell.h"
#include "Evaluation.h"
#include "Commandes_Internes.h"

#include <sys/wait.h>
#include <fcntl.h>

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

//////////////////////////////////////
// INT EXECUTER_SIMPLE(EXPRESSION*) //
///////////////////////////////////////////////////////////////////////////////
// Fonction exécutant une commande simple (cas SIMPLE de l'arbre syntaxique) //
///////////////////////////////////////////////////////////////////////////////

static int
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

//////////////////////////////////////////
// INT EXECUTER_EXPRESSION(EXPRESSION*) //
////////////////////////////////////////////////////////////////////////////////////////
// Fonction récursive parcourant l'arbre et s'assurant du respect des types de noeuds //
// de l'arbre syntaxique (pipes, redirections, séquences...)                          //
////////////////////////////////////////////////////////////////////////////////////////

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
