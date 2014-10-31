#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <assert.h>

#include "mailbox.h"
#include "error.h"
// for simulator_message
#include "network.h"

struct message *get_stop_message() {
  struct message *m = (struct message *) malloc(sizeof(struct message));
  if (m == NULL) {
    myperror("malloc");
    return NULL;
  }
  m->type = STOP_MESSAGE_TYPE;
  m->data = NULL;
  return m;
}

struct mailbox {
  struct letter *next;
  struct letter *last;
  pthread_mutex_t lock;
  sem_t *size;
};

struct letter {
  struct message *m;
  struct letter *next;
};

struct mailbox *new_mailbox () {
  struct mailbox *inbox = (struct mailbox*) malloc(sizeof(struct mailbox));
  if (inbox == NULL) {
    myperror("malloc");
    return NULL;
  }
#ifndef __MACH__
  inbox->size = (sem_t*) malloc(sizeof(sem_t));
  if (inbox->size == NULL) {
    myperror("malloc");
    free(inbox);
    return NULL;
  }
#endif
  inbox->next = NULL;
  inbox->last = NULL;
  int err = pthread_mutex_init(&inbox->lock, NULL);
  if (err != 0) {
    // Normally pthread_mutex_init always returns 0 but
    // let's be careful
    myperror("pthread_mutex_init");
#ifndef __MACH__
    free(inbox->size);
#endif
    free(inbox);
    return NULL;
  }
#ifdef __MACH__
  inbox->size = sem_open("/semaphore", O_CREAT, 0644, 1);
  if (inbox->size == SEM_FAILED) {
    perror("sem_open");
    err = -1;
  }
#else
  err = sem_init(inbox->size, 0, 0);
  if (err != 0) {
    myperror("sem_init");
    free(inbox->size);
  }
#endif
  if (err != 0) {
    free(inbox);
    err = pthread_mutex_destroy(&(inbox->lock));
    if (err != 0) {
      myerror(err, "pthread_mutex_destroy");
    }
    return NULL;
  }
  return inbox;
}

void delete_mailbox (struct mailbox *inbox) {
  while (inbox->next != NULL) {
    struct letter *oldl = inbox->next;
    inbox->next = oldl->next;
    if (oldl->m->data != NULL) {
      if (oldl->m->type == ACK_MESSAGE_TYPE) {
        // I know it's ugly
        struct simulator_message *sm = (struct simulator_message *) oldl->m->data;
        free(sm->p);
      }
      free(oldl->m->data);
    }
    free(oldl->m);
    free(oldl);
  }
  inbox->last = NULL;
  int err = pthread_mutex_destroy(&inbox->lock);
  if (err != 0) {
    myerror(err, "pthread_mutex_destroy");
    // must continue to destroy and free inbox->size
  }
#ifdef __MACH__
  if (sem_close(inbox->size) == -1) {
    perror("sem_close");
  }
  if (sem_unlink("/semaphore") == -1) {
    perror("sem_unlink");
  }
#else
  err = sem_destroy(inbox->size);
  if (err != 0) {
    myerror(err, "sem_destroy");
    // must continue to free inbox
  }
  free(inbox->size);
#endif
  free(inbox);
}

struct message *get_mail (struct mailbox *inbox, bool block) {
  int err = 0;
  if (block) {
    //printf("waiting         %p\n", inbox);
    err = sem_wait(inbox->size);
    if (err != 0) {
      myperror("sem_wait");
      // Il se pourrait que le programme se bloque...
      return NULL;
    }
    //printf("stopped waiting %p\n", inbox);
  } else {
    err = sem_trywait(inbox->size);
    if (err != 0) {
      if (errno == EAGAIN) {
        return NULL;
      } else {
        myperror("sem_wait");
        // Il se pourrait que le programme se bloque...
        return NULL;
      }
    }
  }
  err = pthread_mutex_lock(&inbox->lock);
  if (err != 0) {
    myerror(err, "pthread_mutex_lock");
    // Il se pourrait que le programme se bloque...
    return NULL;
  }
  // next can't be NULL since sem_wait a été débloqué
  struct letter *l = inbox->next;
  struct message *m = l->m;
  inbox->next = l->next;
  if (inbox->next == NULL) {
    inbox->last = NULL;
  }
  free(l);
  err = pthread_mutex_unlock(&inbox->lock);
  if (err != 0) {
    myerror(err, "pthread_mutex_unlock");
  }
  return m;
}

// return an int ??
void send_mail (struct mailbox *inbox, struct message *m) {
  if (m == NULL) return;
  // On se prépare un max avant de locker
  struct letter *new_l = (struct letter *) malloc(sizeof(struct letter));
  if (new_l == NULL) {
    myperror("malloc");
    return;
  }
  new_l->m = m;
  // ajoute à q
  int err = pthread_mutex_lock(&inbox->lock);
  if (err != 0) {
    myerror(err, "pthread_mutex_lock");
    free(new_l);
    //return 1;
    return;
  }
  new_l->next = NULL;
  if (inbox->last == NULL) {
    assert(inbox->next == NULL);
    inbox->next = new_l;
  } else {
    inbox->last->next = new_l;
  }
  inbox->last = new_l;
  //new_first->next = q->next;
  err = pthread_mutex_unlock(&inbox->lock);
  if (err != 0) {
    myerror(err, "pthread_mutex_unlock");
    // free new_l ?? we need to free everything anyway
  }
  //printf("posting         %p\n", inbox);
  err = sem_post(inbox->size);
  if (err != 0) {
    myperror("sem_post");
    // Il se pourrait que le programme se bloque...
  }
  //return 0;
}
