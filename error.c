#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

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
