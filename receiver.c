#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "common.h"

/* Flag set by ‘--verbose’. */
static int verbose_flag = 0;

char packet[PACKET_SIZE];
char *header;
uint16_t *payload_len;
char *payload;
uint32_t *crc;

int main (int argc, char **argv) {
  header = (char *) packet;
  payload_len = (uint16_t *) (header + 2);
  payload = header + 4;
  crc = (uint32_t *) (payload + 512);

  char *filename = NULL;
  char *hostname = NULL;
  char *port = NULL;

  read_args(argc, argv, &filename, NULL, NULL, NULL, &hostname, &port, &verbose_flag);

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
  hints.ai_family = AF_UNSPEC;
  //hints.ai_flags = AI_PASSIVE | AI_ALL;    /* For wildcard IP address */
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

    {
        char hostname[NI_MAXHOST];

        printf("f%d fam%d sock%d prot%d %d %d %d ai_canonname:%s\n", rp->ai_flags, rp->ai_family, rp->ai_socktype, rp->ai_protocol, rp->ai_addrlen, rp->ai_addr->sa_family, *(rp->ai_addr->sa_data), rp->ai_canonname);
        int error = getnameinfo(rp->ai_addr, rp->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0);
        if (error != 0)
        {
            fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
            continue;
        }
        if (*hostname != '\0')
            printf("hostname: %s\n", hostname);
    }


    int err = bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0;
    if (err)
      break;                  /* Success */
    else {
      printf("%d\n", errno);
      myperror("bind");
    }

    close(sfd);
  }

  if (rp == NULL) {               /* No address succeeded */
    fprintf(stderr, "Could not bind :(\n");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(result);           /* No longer needed */

  /* Read datagrams and echo them back to sender */

  int fd = get_fd(filename, true);

  size_t len = PAYLOAD_SIZE;
  ssize_t nread = 0;

  while (len == PAYLOAD_SIZE) {
    peer_addr_len = sizeof(struct sockaddr_storage);
    nread = recvfrom(sfd, packet, PACKET_SIZE, 0,
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

    len = *payload_len;
    uint32_t expected_crc = rc_crc32(0, packet, 4 + PAYLOAD_SIZE);
    if (*crc == expected_crc && len <= PAYLOAD_SIZE) {
      printf("%u\n", len);

      write(fd, payload, len);
    }

    /*if (sendto(sfd, buf, nread, 0,
          (struct sockaddr *) &peer_addr,
          peer_addr_len) != nread)
      fprintf(stderr, "Error sending response\n");*/
  }

  close_fd(fd);

  exit(EXIT_SUCCESS);
}
