#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define ERROR_FILE
#include "error.h"

void myperror (char *msg) {
  perror(msg);
  panic = 1;
}
void myerror (int err, char *msg) {
  errno = err;
  myperror(msg);
}
void mygaierror (int err, char *msg) {
  fprintf(stderr, "%s: %s\n", msg, gai_strerror(err));
  panic = 1;
}
void myserror (char *msg) {
  fprintf(stderr, "%s\n", msg);
  panic = 1;
}
