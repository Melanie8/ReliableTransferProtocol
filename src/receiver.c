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
#include "common.h"

/* Slot in the receiving buffer */
struct slot {
  bool received;
  char data[PACKET_SIZE];
};

static int verbose_flag = 0;        // flag set by ‘--verbose’

static char packet[PACKET_SIZE];           // a received packet
static char *header, *payload;             // pointers to the different areas of the packet
static uint8_t *seq_num;
static uint16_t *payload_len;
static uint32_t *crc;
static struct slot buffer[MAX_WIN_SIZE];   // the receiving buffer containing out of sequence packets
static int lastack;                        // sequence number of the last in sequence acknowledged packet
static int lastack_in_window;              // at which sequence number corresponds the start of the window
static char real_window_size;              // the current size of the receiving window
static int lastseq;                        // useful variable to know when to stop the receiver : = -1 at the beginning,
                                    // becomes the seqnum of the last packet, stop when lastack = lastseq

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
  struct addrinfo *result = NULL, *rp = NULL;
  int sfd = 0, err = 0, fd = -1;
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_protocol = IPPROTO_UDP;

  err = getaddrinfo(hostname, port, &hints, &result);
  if (err != 0) {
    mygaierror(err, "getaddrinfo");
  } else {

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind(2).
       If socket(2) (or bind(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
      sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1)
        continue;

      if (bind(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
        break;                  /* Success */

      close(sfd);
    }

    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
      myserror("Could not bind :(");
    } else {
      fd = get_fd(filename, true);
    }
  }

  size_t len = PAYLOAD_SIZE;
  ssize_t nread = 0;
  char type, window;

  peer_addr_len = sizeof(struct sockaddr_storage);

  /* Until we reach the end of the transmission */
  while (fd != -1 && !panic && lastseq != lastack) {

    if (verbose_flag) {
      printf("LASTSEQ %d LASTACK %d\n", lastseq, lastack);
      printf("len : %lu\n", len);
    }

    /* Packet reception */
    nread = recvfrom(sfd, packet, PACKET_SIZE, 0,
                     (struct sockaddr *) &peer_addr, &peer_addr_len);
    if (verbose_flag)
      printf("nread : %ld\n", nread);
    if (nread == -1)
      continue; //Ignore failed request

    /* Identification of the host name of the sender and the service name associated with its port number */
    char host[NI_MAXHOST], service[NI_MAXSERV];
    err = getnameinfo((struct sockaddr *) &peer_addr,
                    peer_addr_len, host, NI_MAXHOST,
                    service, NI_MAXSERV, NI_NUMERICSERV);
    if (err == 0) {
      if (verbose_flag)
        printf("Received %zd bytes from %s:%s\n",
             nread, host, service);
    } else {
      mygaierror(err, "getnameinfo");
    }

    /* If the CRC is not correct, the packet is dropped */
    len = ntohs(*payload_len);
    uint32_t expected_crc = crc_packet((struct packet*) &packet[0]);

    if (verbose_flag)
      printf("%u~%u==%u %lu <= %d\n", *crc, ntohl(*crc), expected_crc, len, PAYLOAD_SIZE);
    if (ntohl(*crc) == expected_crc && len <= PAYLOAD_SIZE) {
      if (verbose_flag)
        printf("Accepted !\n");

      if (len < PAYLOAD_SIZE) {
        if (verbose_flag)
          printf("LEN %lu\n", len);
        lastseq = *seq_num;
      }

      /* Sanity check : the receiver only receives DATA packets of size <= 512 bytes with
       a receiving window size equal to zero */
      type = (*header >> WINDOW_SIZE);
      window = (*header & MAX_WIN_SIZE);
      if (verbose_flag)
        printf("type:%d==%d window==%d len==%lu\n", type, PTYPE_DATA, window, len);
      if (type == PTYPE_DATA && window==0 && len<=512) {
        if (verbose_flag)
          printf("Passed sanity check !\n");

        /* A packet outside the receiving window is dropped */

        if (between_mod((lastack+1)%MAX_SEQ, (lastack+1+real_window_size)%MAX_SEQ, *seq_num )) {
          if (verbose_flag)
            printf("Inside receiving window !\n");
          /* The good packets are placed in the receive buffer */
          int slot_number = index_in_window(lastack, lastack_in_window, *seq_num);
          if (verbose_flag)
            printf("slot_number:%d\n", slot_number);
          buffer[slot_number].received = true;
          memcpy(buffer[slot_number].data, header, PACKET_SIZE);

          /* All consecutive packets starting at lastack are removed from the receive buffer.
           * The payload of these packets are delivered to the user.
           * Lastack and the receiving window are updated.
           */
          for (i=0; i < MAX_WIN_SIZE && buffer[(lastack_in_window + i + 1) % MAX_WIN_SIZE].received; i++) {
            int k = (lastack_in_window + i + 1) % MAX_WIN_SIZE;
            if (verbose_flag)
              printf("i = %d, k = %d!\n", i, k);
            len = ntohs(*((uint16_t*)(buffer[k].data+2)));
            if (verbose_flag)
              printf("Write : fd : %d, payload : %s, len : %u!\n", fd, buffer[k].data + 4, (uint32_t) len);
            if (write(fd, buffer[k].data + 4, len) != len) {
              fprintf(stderr, "Error writing the payload in the file\n");
              myperror("write");
            }
            buffer[k].received = false;
          }
          lastack = (lastack+i)%MAX_SEQ;
          lastack_in_window = (lastack_in_window+i)%MAX_WIN_SIZE;
        }

        /* An acknowledgement is sent */
        *seq_num = (lastack+1) % MAX_SEQ;
        header[0] = (PTYPE_ACK << WINDOW_SIZE) | MAX_WIN_SIZE;
        memset(payload_len, 0, LEN_SIZE);
        memset(payload, 0, PAYLOAD_SIZE);
        *crc = htonl(crc_packet((struct packet*) &packet[0]));
        if (verbose_flag)
          printf("%d %u\n", *seq_num, *crc);
        if (sendto(sfd, packet, PACKET_SIZE, 0, (struct sockaddr *) &peer_addr, peer_addr_len) != PACKET_SIZE) {
          fprintf(stderr, "Error sending response\n");
          myperror("sendto");
        }
        if (verbose_flag)
          printf("Ack sent !\n");
      }
    }
    
    // FIXME attendre un peu avant de qui pour si jamais le dernier ACK a ete perdu
  }

  close_fd(fd);

  err = close(sfd);
  if (err < 0) {
    myserror("Error while closing the socket");
    myperror("close");
  }

  exit(panic ? EXIT_FAILURE : EXIT_SUCCESS);
}
