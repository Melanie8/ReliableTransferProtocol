#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#include "error.h"
#include "common.h"


/* Flag set by ‘--verbose’. */
static int verbose_flag = 0;

char packet[PACKET_SIZE];
char *header;
char *payload;
uint16_t *payload_len;
uint32_t *crc;

int main (int argc, char **argv) {
  header = (char *) packet;
  payload_len = (uint16_t *) (header + 2);
  payload = header + 4;
  crc = (uint32_t *) (payload + 512);

  char *filename = NULL;
  int sber  = 0;
  int splr  = 0;
  int delay = 0;
  char *hostname = NULL;
  char *port = NULL;

  read_args(argc, argv, &filename, &sber, &splr, &delay, &hostname, &port, &verbose_flag);

  //  ___ ___
  // |_ _/ _ \
  //  | | | | |
  //  | | |_| |
  // |___\___/


  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd, s, j;

  /* Obtain address(es) matching host/port */

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_protocol = IPPROTO_UDP;

  s = getaddrinfo(hostname, port, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype,
        rp->ai_protocol);
    if (sfd == -1)
      continue;

    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;                  /* Success */

    close(sfd);
  }

  if (rp == NULL) {               /* No address succeeded */
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(result);           /* No longer needed */

  /* Send remaining command-line arguments as separate
     datagrams, and read responses from server */

  int fd = get_fd(filename, false);

  size_t len = PAYLOAD_SIZE;
  ssize_t nread = 0;

  int seq = 0;
  while (len == PAYLOAD_SIZE) {
    len = read(fd, payload, len);
    if (len < 0) {
      myperror("read");
      exit(EXIT_FAILURE);
    }

    memset(payload + len, 0, PAYLOAD_SIZE-len);

    header[0] = (PTYPE_DATA << 5);
    header[1] = seq++;
    *payload_len = len;
    printf("%d\n", *payload_len);

    *crc = rc_crc32(0, packet, 4 + PAYLOAD_SIZE);

    if (write(sfd, packet, PACKET_SIZE) != PACKET_SIZE) {
      fprintf(stderr, "partial/failed write\n");
      exit(EXIT_FAILURE);
    }

    /*nread = read(sfd, payload, PAYLOAD_SIZE);
    if (nread == -1) {
      myperror("read");
      exit(EXIT_FAILURE);
    }
    payload[nread] = '\0';

    printf("Received %zd bytes: %s\n", nread, payload);*/
  }

  close_fd(fd);

  exit(EXIT_SUCCESS);
}
