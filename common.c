#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "error.h"

//     _    ____   ____ ____
//    / \  |  _ \ / ___/ ___|
//   / _ \ | |_) | |  _\___ \
//  / ___ \|  _ <| |_| |___) |
// /_/   \_\_| \_\\____|____/

void bad_arg (char *message) {
  fprintf(stderr, "%s.\n", message);
  // nothing to free
  exit(EXIT_FAILURE);
}

void check_asked (void *p, char *name) {
  if (p == NULL) {
    char s[32];
    snprintf(s, 32, "Unexpected argument %s", name);
    bad_arg(s);
  }
}

int check_int (char *dec, int min, int max, char *name) {
  char *endptr = NULL;
  long n = strtol(dec, &endptr, 10);
  if (*optarg == '\0' || *endptr != '\0') {
    char s[64];
    snprintf(s, 64, "Invalid %s, it should be an integer", name);
    bad_arg(s);
  }
  if (n < min) {
    char s[64];
    snprintf(s, 64, "Invalid %s, it should be >= %d", name, min);
    bad_arg(s);
  }
  if (n > max) {
    char s[64];
    snprintf(s, 64, "Invalid %s, it should be <= %d", name, max);
    bad_arg(s);
  }
  return (int) n;
}

void read_args (int argc, char **argv, char **filename, int *sber, int *splr, int *delay, char **hostname, char **port, int *verbose_flag) {
  int c = -1;
  while (1) {
    static struct option long_options[7] =
    {
      /* These options set a flag. */
      {"verbose", no_argument,       NULL, 1},
      {"brief"  , no_argument,       NULL, 0},
      /* These options don't set a flag.
         We distinguish them by their indices. */
      {"file",    required_argument, NULL, 'f'},
      {"sber",    required_argument, NULL, 'e'},
      {"splr",    required_argument, NULL, 'l'},
      {"delay",   required_argument, NULL, 'd'},
      {NULL, 0, NULL, 0}
    };
    long_options[0].flag = verbose_flag;
    long_options[1].flag = verbose_flag;
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != NULL)
          break;
        fprintf(stderr, "option %s", long_options[option_index].name);
        if (optarg)
          fprintf(stderr, " with arg %s", optarg);
        fputc('\n', stderr);
        break;

      case 'f':
        check_asked(filename, "filename");
        *filename = optarg;
        break;

      case 'e':
        check_asked(sber, "sber");
        *sber = check_int(optarg, 0, 1000, "sber");
        break;

      case 'l':
        check_asked(splr, "splr");
        *splr = check_int(optarg, 0, 1000, "splr");
        break;

      case 'd':
        check_asked(delay, "delay");
        *delay = check_int(optarg, 0, INT_MAX, "delay");
        break;

      case '?':
        /* getopt_long already printed an error message. */
        break;

      default:
        abort();
    }
  }

  /* Instead of reporting ‘--verbose’
     and ‘--brief’ as they are encountered,
     we report the final status resulting from them. */
  if (*verbose_flag)
    fputs("verbose flag is set", stderr);

  if (optind >= argc) {
    bad_arg("Missing hostname");
  } else {
    *hostname = argv[optind++];
  }

  if (optind >= argc) {
    bad_arg("Missing port");
  } else {
    *port = argv[optind++];
  }

  if (optind < argc) {
    fprintf(stderr, "unused arguments: ");
    while (optind < argc)
      fprintf(stderr, "%s ", argv[optind++]);
    fputc('\n', stderr);
  }
}

//  ___ ___
// |_ _/ _ \
//  | | | | |
//  | | |_| |
// |___\___/

int get_fd (const char *filename, bool write) {
  if (filename == NULL) {
    return write ? STDOUT_FILENO : STDIN_FILENO;
  } else {
    int fd = 0;
    if (write) {
      fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    } else {
      fd = open(filename, O_RDONLY);
    }
    if (fd < 0) {
      myperror("open");
      exit(EXIT_FAILURE);
    }
    fprintf(stderr, "%s opened\n", filename);
    return fd;
  }
}

void close_fd (int fd) {
  if (fd != STDOUT_FILENO && fd != STDIN_FILENO) {
    int err = close(fd);
    if (err < 0) {
      myperror("close");
      // We close everything now, no need to exit
    }
  }
}
