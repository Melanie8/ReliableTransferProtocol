#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>


#include "common.h"
#include "error.h"
#include "network.h"

#include "acker.h"

struct mailbox *network_inbox;
int sfd;
int ack_id;
int verbose_flag;

void ask_if_cont () {
  struct message *cont = get_stop_message();
  cont->type = CONTINUE_ACKING_MESSAGE_TYPE;
  if (verbose_flag)
    printf("acker   sends %d to network\n", cont->type);
  send_mail(network_inbox, cont);
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
    ssize_t nread = read(sfd, p, PACKET_SIZE);
    if (nread == -1) {
      myperror("read");
      exit(EXIT_FAILURE);
    }
    struct message *ack_m = get_stop_message();
    struct simulator_message *sm = (struct simulator_message*) malloc(sizeof(struct simulator_message));
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
  } else {
    assert(m->type == STOP_MESSAGE_TYPE);
  }
  return true;
}
