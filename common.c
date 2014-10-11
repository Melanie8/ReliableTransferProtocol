#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "common.h"

int get_fd (const char *filename, bool write) {
  if (filename == NULL) {
    return write ? STDOUT_FILENO : STDIN_FILENO;
  } else {
    int fd = 0;
    if (write) {
      printf("%s\n", filename);
      fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    } else {
      fd = open(filename, O_RDONLY);
    }
    if (fd < 0) {
      perror("open");
      exit(EXIT_FAILURE);
    }
    printf("%s opened\n", filename);
    return fd;
  }
}
