#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <error.h>

#include "error.h"

/*pthread_mutex_t panic_mutex;

void init_panic () {
  panic = 0;
  int err = pthread_mutex_init(&panic_mutex, NULL);
  if (err != 0) {
    // shouldn't happen according to man
    myerror(err, "pthread_mutex_init");
  }
}

int get_panic () {
}

void set_panic (int level) {
  int err = pthread_mutex_lock(&panic_mutex);
  if (err != 0) {
  }
}

void end_panic () {
  int err = pthread_mutex_destroy(&panic_mutex);
  if (err != 0) {
    myerror(err, "pthread_mutex_destroy");
  }
}*/

void myperror (char *msg) {
  /*fprintf(stderr, "%s a retourné %d, message d'erreur : %s\n",
      msg, err, strerror(errno));*/
  perror(msg);
  //panic = 1;
}
void myerror (int err, char *msg) {
  errno = err;
  myperror(msg);
}
