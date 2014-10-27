#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#include "mailbox.h"
#include "error.h"

struct letter {
  struct message *m;
  struct letter *next;
};

struct mailbox {
  struct letter *next;
  pthread_mutex_t lock;
  sem_t *size;
};

struct mailbox *new_mailbox () {
  struct mailbox *inbox = (struct mailbox*) malloc(sizeof(struct mailbox));
  if (inbox == NULL) {
    myperror("malloc");
    return NULL;
  }
#ifndef __APPLE__
  inbox->size = (sem_t*) malloc(sizeof(sem_t));
  if (inbox->size == NULL) {
    myperror("malloc");
    free(inbox);
    return NULL;
  }
#endif
  inbox->next = NULL;
  int err = pthread_mutex_init(&inbox->lock, NULL);
  if (err != 0) {
    // Normally pthread_mutex_init always returns 0 but
    // let's be careful
    myperror("pthread_mutex_init");
#ifndef __APPLE__
    free(inbox->size);
#endif
    free(inbox);
    return NULL;
  }
#ifdef __APPLE__
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
  int err = pthread_mutex_destroy(&inbox->lock);
  if (err != 0) {
    myerror(err, "pthread_mutex_destroy");
    // must continue to destroy and free inbox->size
  }
#ifdef __APPLE__
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
    err = sem_wait(inbox->size);
    if (err != 0) {
      myperror("sem_wait");
      // Il se pourrait que le programme se bloque...
      return NULL;
    }
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
  free(l);
  err = pthread_mutex_unlock(&inbox->lock);
  if (err != 0) {
    myerror(err, "pthread_mutex_unlock");
  }
  return m;
}

// return an int ??
void send_mail (struct mailbox *inbox, struct message *m) {
  // On se prépare un max avant de locker
  struct letter *new_l = (struct letter *) malloc(sizeof(struct letter));
  if (new_l == NULL) {
    myperror("malloc");
    //return 1;
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
  new_l->next = inbox->next;
  //new_first->next = q->next;
  inbox->next = new_l;
  err = pthread_mutex_unlock(&inbox->lock);
  if (err != 0) {
    myerror(err, "pthread_mutex_unlock");
    // free new_l ?? we need to free everything anyway
  }
  err = sem_post(inbox->size);
  if (err != 0) {
    myperror("sem_post");
    // Il se pourrait que le programme se bloque...
  }
  //return 0;
}
