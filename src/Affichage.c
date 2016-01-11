#include <stdio.h>

#include "Affichage.h"

char *chaine_type[] = {
  "<vide>",	                         // Commande vide 
  "",                                    // Commande simple 
  "Séquence",      	                 // Séquence (;) 
  "Si succès",   	                 // Séquence conditionnelle (&&) 
  "Si échec",  	                         // Séquence conditionnelle (||) 
  "Arrière-plan",		         // Tache en arriere plan 
  "Pipe",	       	                 // Pipe 
  "Entrée standard depuis",	         // Redirection entree 
  "Sortie standard dans", 	         // Redirection sortie standard 
  "Sortie standard (concaténation) dans",// Redirection sortie standard, mode append 
  "Sortie d'erreur dans", 	         // Redirection sortie erreur 
  "Sorties standard et d'erreur dans",   // Redirection sortie standard et erreur
  "Sous-shell mystérieux"};              // Mystère...



void indenter_vide(int indentation, int trait){
  for(int i = 1; i<= indentation + trait; i++)
    if ( i % trait == 0)
      putchar('|');
    else
      putchar(' ');
}

void indenter(int indentation, int trait){
  for(int i = 1; i< indentation; i++)
    if ( i % trait == 0)
      putchar('|');
    else
      putchar(' '); 
  putchar('|');
  for(int i = 2; i< trait; i++)
    if ( indentation % trait == 0)
      putchar('-');
  putchar('>');
  putchar(' ');
}



  
void afficher_exprL(Expression *e, int indentation, int trait)
{
  if (e == NULL) return ;

  switch(e->type){

  case VIDE :
    indenter(indentation,trait);
    printf("%s\n", chaine_type[e->type]);
    break ;
    
  case SIMPLE :
    indenter(indentation,trait);
    printf("%s ", chaine_type[e->type]);
    for(int i=0; e->arguments[i] != NULL;i++)
      printf("[%s]",e->arguments[i]);
    putchar('\n');
    break;

  case REDIRECTION_I: 	
  case REDIRECTION_O: 	
  case REDIRECTION_A: 	
  case REDIRECTION_E: 	
  case REDIRECTION_EO :
    indenter(indentation,trait);    
    printf("%s [%s]\n",chaine_type[e->type], e->arguments[0]);
    afficher_exprL(e->gauche, indentation + trait, trait);
    break;
  case BG:
  case SOUS_SHELL:
    indenter(indentation,trait);
    printf("%s\n", chaine_type[e->type]);
    afficher_exprL(e->gauche, indentation + trait, trait);
    break;
  default :

    indenter(indentation,trait);
    printf("%s\n", chaine_type[e->type]);
    afficher_exprL(e->gauche, indentation + trait, trait);
    indenter_vide(indentation,trait);putchar('\n');
    afficher_exprL(e->droite, indentation + trait, trait);      
  }
}



void afficher_expr(Expression *e)
{
  printf("\n");
  afficher_exprL(e,4,4);
  printf("\n");
}
