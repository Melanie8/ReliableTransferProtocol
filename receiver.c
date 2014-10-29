/*
 * Written by :
 * Benoît Legat <benoit.legat@student.uclouvain.be>
 * Mélanie Sedda <melanie.sedda@student.uclouvain.be>
 * October 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
//#include <zlib.h>

#include "error.h"
#include "receiver.h"
#include "common.h"

static int verbose_flag = 0;        // flag set by ‘--verbose’

char packet[PACKET_SIZE];           // a received packet
char *header, *payload;   // pointers to the different areas of the packet
uint8_t *seq_num;
uint16_t *payload_len;
uint32_t *crc;
struct slot buffer[MAX_WIN_SIZE];    // the receiving buffer containing out of sequence packets
int lastack;                       // sequence number of the last acknowledged packet
int lastack_in_window;            // a quel sequence number correspond le début de ma window
char real_window_size;              // the current size of the receiving window
int lastseq;                        // variable qui vaut -1 au début, devient le seqnum du dernier packet, stop programme quand = lastack

int main (int argc, char **argv) {

  /* Initialisation of the variables */
  header = (char *) packet;
  seq_num = (uint8_t *) (header + 1);
  payload = header + 4;
  payload_len = (uint16_t *) (header + 2);
  crc = (uint32_t *) (payload + 512);
  lastack = MAX_SEQ - 1;
  lastack_in_window = MAX_WIN_SIZE - 1;
  real_window_size = MAX_WIN_SIZE;
  lastseq = -1;
  int i;
  for (i=0; i<MAX_WIN_SIZE; i++) {
    buffer[i].received = false;
  }

  char *filename = NULL;
  char *hostname = NULL;
  char *port = NULL;

  /* Reading the arguments passed on the command line */
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
  char type, window;

  peer_addr_len = sizeof(struct sockaddr_storage);


  /* Until we reach the end of the transmission */
  while (lastseq != lastack) {
    fprintf(stderr, "LASTSEQ %d LASTACK %d\n", lastseq, lastack);
	printf("len : %d\n", len);

    /* Packet reception */
    nread = recvfrom(sfd, packet, PACKET_SIZE, 0,
                     (struct sockaddr *) &peer_addr, &peer_addr_len);
    printf("nread : %d\n", nread);
    if (nread == -1)
      continue; //Ignore failed request

    /* Identification of the host name of the sender and the service name associated with its port number */
    char host[NI_MAXHOST], service[NI_MAXSERV];
    s = getnameinfo((struct sockaddr *) &peer_addr,
                    peer_addr_len, host, NI_MAXHOST,
                    service, NI_MAXSERV, NI_NUMERICSERV);
    if (s == 0)
      printf("Received %zd bytes from %s:%s\n",
             nread, host, service);
    else
      fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

    /* If the CRC is not correct, the packet is dropped */
    len = ntohs(*payload_len);
    uint32_t expected_crc = crc_packet((struct packet*) &packet[0]);

    // FIXME attendre un peu avant de qui pour si jamais le dernier ACK a ete perdu

    // If the CRC is not correct but len < PAYLOAD_SIZE, don't stop
    printf("%lu~%lu==%lu %d <= %d\n", *crc, ntohl(*crc), expected_crc, len, PAYLOAD_SIZE);
    if (ntohl(*crc) == expected_crc && len <= PAYLOAD_SIZE) {
      printf("Accepted !\n");

      if (len < PAYLOAD_SIZE) {
        printf("LEN %u\n", len);
        lastseq = *seq_num;
      }

      /* Sanity check : the receiver only receives DATA packets of size <= 512 bytes with
       a receiving window size equal to zero */
      type = (*header >> WINDOW_SIZE);
      window = (*header & MAX_WIN_SIZE);
      printf("type:%d==%d window==%d len==%d\n", type, PTYPE_DATA, window, len);
      if (type == PTYPE_DATA && window==0 && len<=512) {
        printf("Passed sanity check !\n");

        /* A packet outside the receiving window is dropped */
        printf("lastack_in_window : %d, seq_num : %d, end_window_seqnum : %d, N : %d\n", lastack_in_window, *seq_num, lastack_in_window+real_window_size, N);
        printf("%d >= %d && %d <= %d\n", *seq_num, (lastack_in_window)%(N-1), *seq_num, (lastack_in_window+real_window_size)%(N-1));

        if (between_mod((lastack+1)%MAX_SEQ, (lastack+1+real_window_size)%MAX_SEQ, *seq_num )) {
          printf("Inside receiving window !\n");
          /* The good packets are placed in the receive buffer */
          int slot_number = index_in_window(lastack, lastack_in_window, *seq_num);
          printf("slot_number:%d\n", slot_number);
          buffer[slot_number].received = true;
          memcpy(buffer[slot_number].data, header, PACKET_SIZE);

          /* All consecutive packets starting at lastack are removed from the receive buffer.
           * The payload of these packets are delivered to the user.
           * Lastack and the receiving window are updated.
           */
          for (i=0; i < MAX_WIN_SIZE && buffer[(lastack_in_window + i + 1) % MAX_WIN_SIZE].received; i++) {
            int k = (lastack_in_window + i + 1) % MAX_WIN_SIZE;
            printf("i = %d, k = %d!\n", i, k);
            len = ntohs(*((uint16_t*)(buffer[k].data+2)));
            printf("Write : fd : %d, payload : %s, len : %u!\n", fd, buffer[k].data + 4, (uint32_t) len);
            if (write(fd, buffer[k].data + 4, len) != len) {
              fprintf(stderr, "Error writing the payload in the file\n");
            }
            buffer[k].received = false;
          }
          lastack = (lastack+i)%MAX_SEQ;
          lastack_in_window = (lastack_in_window+i)%MAX_WIN_SIZE;
        }

        /* An acknowledgement is sent */
        *seq_num = (lastack+1)%MAX_SEQ;
        header[0] = (PTYPE_ACK << WINDOW_SIZE) | MAX_WIN_SIZE;
        memset(payload, 0, PAYLOAD_SIZE);
        *crc = htonl(crc_packet((struct packet*) &packet[0]));
        fprintf(stderr, "%d %u\n", *seq_num, crc);
        if (sendto(sfd, packet, PACKET_SIZE, 0, (struct sockaddr *) &peer_addr, peer_addr_len) != PACKET_SIZE) {
          fprintf(stderr, "Error sending response\n");
        }
        printf("Ack sent !\n");
      }
    }
  }

  close_fd(fd);

  exit(EXIT_SUCCESS);
}
