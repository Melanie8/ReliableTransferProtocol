#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#include "common.h"

/* Flag set by ‘--verbose’. */
static int verbose_flag = 0;

int main (int argc, char **argv) {

  //     _    ____   ____ ____
  //    / \  |  _ \ / ___/ ___|
  //   / _ \ | |_) | |  _\___ \
  //  / ___ \|  _ <| |_| |___) |
  // /_/   \_\_| \_\\____|____/

  int c = -1;
  char *filename = NULL;
  char *hostname = NULL;
  char *port = NULL;

  while (1) {
    static struct option long_options[] =
    {
      /* These options set a flag. */
      {"verbose", no_argument,       &verbose_flag, 1},
      {"brief"  , no_argument,       &verbose_flag, 0},
      /* These options don't set a flag.
         We distinguish them by their indices. */
      {"file",    required_argument, NULL, 'f'},
      {NULL, 0, NULL, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    char *endptr = NULL;
    switch (c) {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != NULL)
          break;
        printf ("option %s", long_options[option_index].name);
        if (optarg)
          printf (" with arg %s", optarg);
        printf ("\n");
        break;

      case 'f':
        filename = optarg;
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
  if (verbose_flag)
    puts ("verbose flag is set");

  if (optind >= argc) {
    fprintf(stderr, "Missing hostname\n");
    exit(1);
    hostname = argv[optind++];
  } else {
    hostname = argv[optind++];
  }

  if (optind >= argc) {
    fprintf(stderr, "Missing port\n");
    exit(1);
  } else {
    port = argv[optind++];
  }

  if (optind < argc) {
    printf ("unused arguments: ");
    while (optind < argc)
      printf ("%s ", argv[optind++]);
    putchar ('\n');
  }


  //  ___ ___
  // |_ _/ _ \
  //  | | | | |
  //  | | |_| |
  // |___\___/

  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd, s;
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET6;
  //hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
  // I have specified the hostname so no wildcard.
  hints.ai_protocol = IPPROTO_UDP;

  s = getaddrinfo(hostname, port, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully bind(2).
     If socket(2) (or bind(2)) fails, we (close the socket
     and) try the next address. */

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype,
        rp->ai_protocol);
    if (sfd == -1)
      continue;

    if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;                  /* Success */

    close(sfd);
  }

  if (rp == NULL) {               /* No address succeeded */
    fprintf(stderr, "Could not bind\n");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(result);           /* No longer needed */

  /* Read datagrams and echo them back to sender */

  int fd = get_fd(filename, true);

  ssize_t nread = BUF_SIZE;
  char buf[BUF_SIZE];

  while (nread == BUF_SIZE) {
    peer_addr_len = sizeof(struct sockaddr_storage);
    nread = recvfrom(sfd, buf, BUF_SIZE, 0,
        (struct sockaddr *) &peer_addr, &peer_addr_len);
    if (nread == -1)
      continue;               /* Ignore failed request */

    char host[NI_MAXHOST], service[NI_MAXSERV];

    s = getnameinfo((struct sockaddr *) &peer_addr,
        peer_addr_len, host, NI_MAXHOST,
        service, NI_MAXSERV, NI_NUMERICSERV);
    if (s == 0)
      printf("Received %zd bytes from %s:%s\n",
          nread, host, service);
    else
      fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

    write(fd, buf, nread);

    if (sendto(sfd, buf, nread, 0,
          (struct sockaddr *) &peer_addr,
          peer_addr_len) != nread)
      fprintf(stderr, "Error sending response\n");
  }

  exit(EXIT_SUCCESS);
}
