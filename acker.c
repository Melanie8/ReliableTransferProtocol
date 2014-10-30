#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>


#include "common.h"
#include "error.h"
#include "network.h"

#include "acker.h"

static struct mailbox *network_inbox;
static int sfd;
static int ack_id;
static int verbose_flag;

void ask_if_cont () {
  struct message *cont = get_stop_message();
  if (cont) {
    cont->type = CONTINUE_ACKING_MESSAGE_TYPE;
    if (verbose_flag)
      printf("acker   sends %d to network\n", cont->type);
    send_mail(network_inbox, cont);
  }
}

bool acker (struct message *m) {
  if (m->type == INIT_MESSAGE_TYPE) {
    struct acker_init *init = m->data;
    network_inbox = init->network_inbox;
    sfd = init->sfd;
    verbose_flag = init->verbose_flag;
    ask_if_cont();
  } else if (m->type == CONTINUE_ACKING_MESSAGE_TYPE) {
    struct packet *p = (struct packet *) malloc(sizeof(struct packet));
    if (p == NULL) {
      myperror("malloc");
    } else {
      ssize_t nread = read(sfd, p, PACKET_SIZE);
      if (nread == -1) {
        free(p);
        myserror("Error receiving a packet");
        myperror("read");
      } else {
        struct message *ack_m = get_stop_message();
        if (ack_m == NULL) {
          free(p);
        } else {
          struct simulator_message *sm = (struct simulator_message*) malloc(sizeof(struct simulator_message));
          if (sm == NULL) {
            free(p);
            free(ack_m);
            myperror("malloc");
          } else {
            sm->p = p;
            sm->id = ack_id;
            ack_id = (ack_id + 1) % MAX_WIN_SIZE;
            sm->last = false; // nonsense here
            ack_m->type = ACK_MESSAGE_TYPE;
            ack_m->data = sm;
            if (verbose_flag)
              printf("acker   sends %d to network\n", m->type);
            send_mail(network_inbox, ack_m);
            ask_if_cont();
          }
        }
      }
    }
  } else {
    assert(m->type == STOP_MESSAGE_TYPE);
  }
  return true;
}
