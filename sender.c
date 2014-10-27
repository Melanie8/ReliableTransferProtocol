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
#include "agent.h"
#include "acker.h"
#include "network.h"
#include "sr.h"
#include "timer.h"

int delay;
int sfd;

/* Flag set by ‘--verbose’. */
static int verbose_flag = 0;

void send_init_message (struct mailbox *inbox, void *init_data) {
  struct message *m = (struct message *) malloc(sizeof(struct message));
  m->type = INIT_MESSAGE_TYPE;
  m->data = init_data;
  send_mail(inbox, m);
}

int main (int argc, char **argv) {
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
  int s, j;

  /* Obtain address(es) matching hostname/port */

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
    if (sfd == -1)                                               /* QUESTION : EN QUOI C'EST UTILE? */
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

  pthread_t sr_thread, timer_thread, network_thread, acker_thread;
  struct mailbox *sr_inbox = make_agent(&sr_thread, selective_repeat);
  struct mailbox *network_inbox = make_agent(&network_thread, network);
  struct mailbox *timer_inbox = make_agent(&timer_thread, timer);
  struct mailbox *acker_inbox = make_agent(&acker_thread, acker);

  struct sr_init *sr_i = (struct sr_init *) malloc(sizeof(struct sr_init));
  struct network_init *network_i = (struct network_init *) malloc(sizeof(struct network_init));
  struct timer_init *timer_i = (struct timer_init *) malloc(sizeof(struct timer_init));
  struct sr_init *acker_i = (struct sr_init *) malloc(sizeof(struct sr_init));
  network_i->sfd = sfd;
  network_i->delay = delay;
  network_i->acker_inbox = acker_inbox;
  network_i->sr_inbox = sr_inbox;
  network_i->timer_inbox = timer_inbox;
  network_i->network_inbox = network_inbox;
  timer_i->delay = delay;
  sr_i->fd = fd;
  sr_i->delay = delay;
  sr_i->network_inbox = network_inbox;
  sr_i->timer_inbox = timer_inbox;
  sr_i->sr_inbox = sr_inbox;
  // the order is important so that INIT is
  // the first message sent
  // sr with start everything so call it last
  send_init_message(timer_inbox, timer_i);
  send_init_message(network_inbox, network_i);
  send_init_message(sr_inbox, sr_i);

  pthread_join(sr_thread, NULL);
  printf("sr joined\n");
  pthread_join(network_thread, NULL);
  printf("network joined\n");
  pthread_join(acker_thread, NULL);
  printf("acker joined\n");
  pthread_join(timer_thread, NULL);
  printf("timer joined\n");

  //close_fd(fd); // done by sr

  exit(EXIT_SUCCESS);
}
