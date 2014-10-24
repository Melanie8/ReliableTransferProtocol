/*
 * Written by :
 * Benoît Legat <benoit.legat@student.uclouvain.be>
 * Mélanie Sedda <melanie.sedda@student.uclouvain.be>
 * October 2014
 */


/* Node representing a slot of the sliding window */
struct window_node {
  bool received;
  char *data;
  struct window_node *next;
  struct window_node *prev;
};

/* The sliding window implemented as a doubly linked list */
struct window {
  struct window_node *first_node;
  struct window_node *last_node; // peut etre pratique si on decide de rallonger ou raccourcir la fenetre
  int size;
};

/* Creates a new sliding window.
 *
 * Returns a pointer to the list. In case of error, returns NULL. */
struct window *new_window();


/* Adds a node at the end of the window.
 *
 * If the function returns 0, the node has been added with success.
 * If the list is NULL and in case of error, the function returns -1.
 */
int push(struct window_node *, bool, char *);

/* Removes the node at the end of the window.
 *
 * 
 */
int pop(struct window_node *);


/* Frees the window.
 */
void free_window(struct window *);
