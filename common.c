/*
 * Written by :
 * Benoît Legat <benoit.legat@student.uclouvain.be>
 * Mélanie Sedda <melanie.sedda@student.uclouvain.be>
 * October 2014
 */

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

#include <inttypes.h>

#include "common.h"
#include "error.h"

//     _    ____   ____ ____
//    / \  |  _ \ / ___/ ___|
//   / _ \ | |_) | |  _\___ \
//  / ___ \|  _ <| |_| |___) |
// /_/   \_\_| \_\\____|____/

/* Writes the message on stderr and exits the program with a
 * failure status. 
 */
void bad_arg (char *message) {
  fprintf(stderr, "%s.\n", message);
  // nothing to free
  exit(EXIT_FAILURE);
}

/* Calls bad_arg if the pointer p is NULL. */
void check_asked (void *p, char *name) {
  if (p == NULL) {
    char s[32];
    snprintf(s, 32, "Unexpected argument %s", name);
    bad_arg(s);
  }
}

/* Checks wheter dec points to a string to can be casted into an
 * integer within min and max. If yes, returns the integer, if 
 * not, calls bad_arg.
 */
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

uint32_t
rc_crc32(const struct packet *pack)
{
	uint32_t crc = 0;
	const char *buf = (const char *)(&pack);
	size_t len = 4 + PAYLOAD_SIZE;
	static uint32_t table[256];
	static int have_table = 0;
	uint32_t rem;
	uint8_t octet;
	int i, j;
	const char *p, *q;

	/* This check is not thread safe; there is no mutex. */
	if (have_table == 0) {
		/* Calculate CRC table. */
		for (i = 0; i < 256; i++) {
			rem = i;  /* remainder from polynomial division */
			for (j = 0; j < 8; j++) {
				if (rem & 1) {
					rem >>= 1;
					rem ^= 0xedb88320;
				} else
					rem >>= 1;
			}
			table[i] = rem;
		}
		have_table = 1;
	}

	crc = ~crc;
	q = buf + len;
	for (p = buf; p < q; p++) {
		octet = *p;  /* Cast to unsigned octet. */
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	return ~crc;
}

bool valid_ack(struct packet *p) {
  uint32_t expected_crc = rc_crc32(p);
  // FIXME check len is 0 ??
  return (expected_crc == ntohl(p->crc));
}
