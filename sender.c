#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "error.h"
#include "common.h"
#include "agent.h"
#include "acker.h"
#include "network.h"
#include "sr.h"
#include "timer.h"

static int delay;
static int sfd;

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
  int err, fd;

  /* Obtain address(es) matching hostname/port */

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_protocol = IPPROTO_UDP;

  err = getaddrinfo(hostname, port, &hints, &result);
  if (err != 0) {
    mygaierror(err, "getaddrinfo");
  } else {

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
      sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1)                                               /* QUESTION : EN QUOI C'EST UTILE? */
        continue;

      if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
        break;                  /* Success */

      close(sfd);
    }

    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
      myserror("Could not connect");
    } else {
      fd = get_fd(filename, false);

      pthread_t sr_thread, timer_thread, network_thread, acker_thread;
      struct mailbox *sr_inbox = make_agent(&sr_thread, selective_repeat);
      struct mailbox *network_inbox = make_agent(&network_thread, network);
      struct mailbox *timer_inbox = make_agent(&timer_thread, timer);
      struct mailbox *acker_inbox = make_agent(&acker_thread, acker);

      if (sr_inbox != NULL && network_inbox != NULL && timer_inbox != NULL && acker_inbox != NULL) {
        struct sr_init *sr_i = (struct sr_init *) malloc(sizeof(struct sr_init));
        if (sr_i == NULL)
          myperror("malloc");
        struct network_init *network_i = (struct network_init *) malloc(sizeof(struct network_init));
        if (network_i == NULL)
          myperror("malloc");
        struct timer_init *timer_i = (struct timer_init *) malloc(sizeof(struct timer_init));
        if (timer_i == NULL)
          myperror("malloc");
        struct acker_init *acker_i = (struct acker_init *) malloc(sizeof(struct acker_init));
        if (acker_i == NULL)
          myperror("malloc");
        if (sr_i != NULL && network_i != NULL && timer_i != NULL && acker_i != NULL) {
          network_i->sber = sber;
          network_i->splr = splr;
          network_i->sfd = sfd;
          network_i->delay = delay;
          network_i->acker_inbox = acker_inbox;
          network_i->sr_inbox = sr_inbox;
          network_i->timer_inbox = timer_inbox;
          network_i->network_inbox = network_inbox;
          network_i->verbose_flag = verbose_flag;
          timer_i->delay = delay;
          timer_i->verbose_flag = verbose_flag;
          sr_i->fd = fd;
          sr_i->delay = delay;
          sr_i->network_inbox = network_inbox;
          sr_i->timer_inbox = timer_inbox;
          sr_i->sr_inbox = sr_inbox;
          sr_i->verbose_flag = verbose_flag;
          acker_i->sfd = sfd;
          acker_i->network_inbox = network_inbox;
          acker_i->verbose_flag = verbose_flag;
          // the order is important so that INIT is
          // the first message sent
          // sr with start everything so call it last
          send_init_message(timer_inbox, timer_i);
          send_init_message(network_inbox, network_i);
          send_init_message(sr_inbox, sr_i);
          // nobody sends it message before he asks
          send_init_message(acker_inbox, acker_i);
        }

        pthread_join(sr_thread, NULL);
        if (verbose_flag)
          printf("sr joined\n");
        pthread_join(network_thread, NULL);
        if (verbose_flag)
          printf("network joined\n");
        pthread_join(acker_thread, NULL);
        if (verbose_flag)
          printf("acker joined\n");
        pthread_join(timer_thread, NULL);
        if (verbose_flag)
          printf("timer joined\n");
      }
    }
  }

  close_fd(fd);

  exit(panic ? EXIT_FAILURE : EXIT_SUCCESS);
}
