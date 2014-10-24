#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "common.h"


/* SLIDING WINDOW */
struct window *new_window () {
  struct window *w = (struct window*) malloc(sizeof(struct window));
  if (w == NULL) {
    myperror("malloc");
    return NULL;
  }
  inbox->size = 0;
  inbox->first_node = NULL;
  inbox->last_node = NULL;
}

int push(struct window *w, bool r, char *d) {
  /* Checks that w is not NULL */
  if (!w) {
    myperror( "Error calling push : the window is NULL \n");
    return -1;
  }
  
  /* Allocates memory for the new node */
  struct window_node *new = (struct window_node*) malloc(sizeof(struct window_node));
  if (!new) {
    myperror( "Error calling push : malloc failed\n");
    return -1;
  }
  
  /* Save the args in the node */
  new->received = r;
  new->data = d;
  
  /* Modifies the links in the list */
  if (!(w->first_node)) {
    w->first_node = new;
    w->last_node = new;
    w->size = 1;
  } else {
    w->last->next = new;
    new->prev = w->last;
    w->last = new;
    w->size ++;
  }
  
  return 0;
}

int pop(struct window *w) {
  /* Checks that w is not NULL */
  if (!w) {
    myperror( "Error calling pop : the window is NULL \n");
    return -1;
  }
  
  /* Modifies the links in the list */
  if (!(w->first_node)) {
    myperror( "Error calling pop : the window is empty \n");
    return -1;
  } else if (w->size == 1) {
    w->first_node = NULL;
    w->last_node = NULL;
    w->size = 0;
  } else {
    w->last->prev->next = NULL;
    struct window_node *temp = w->last->prev;
    free(w->last);
    w->last = temp;
    w->size --;    
  }
  
  return 0;
}

void free_window(struct window *w) {
  /* Vérifie que q est non NULL. */
  if (!w) {
    return;
	/* Si q est vide, il faut juste libérer la file. */
  } else if(!(q->first)) {
    free(w);
    /* Sinon, il faut aussi libérer la mémoire de tous les noeuds avant de
	 * libérer celle de la file. */
  } else {
    struct window_node *temp;
    while (q->first) {
      temp = q->first;
      q->first = q->first->next;
      free(temp); temp = NULL;
    }
  }
  free(w);
}