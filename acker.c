#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "network.h"

#include "acker.h"

struct mailbox *network_inbox;
int sfd;
int ack_id;

bool acker (struct message *m) {
  if (m->type == INIT_MESSAGE_TYPE) {
    struct acker_init *init = (struct acker_init *) malloc(sizeof(struct acker_init));
    network_inbox = init->network_inbox;
    sfd = init->sfd;
  } else {
    assert(m->type == CONTINUE_ACKING_MESSAGE_TYPE);
    // TODO do also non-block
    struct packet *p = (struct packet *) malloc(sizeof(struct packet));
    ssize_t nread = read(sfd, p, PAYLOAD_SIZE);
    if (nread == -1) {
      myperror("read");
      exit(EXIT_FAILURE);
    }
    struct message *m = (struct message*) malloc(sizeof(struct message));
    struct simulator_message *sm = (struct simulator_message*) malloc(sizeof(struct simulator_message));
    sm->p = p;
    sm->id = (ack_id + 1) % MAX_WIN_SIZE;
    sm->last = false; // nonsense here
    m->type = ACK_MESSAGE_TYPE;
    m->data = sm;
    send_mail(network_inbox, m);

    struct message *cont = (struct message*) malloc(sizeof(struct message));
    cont->type = CONTINUE_ACKING_MESSAGE_TYPE;
    cont->data = NULL;
    send_mail(network_inbox, cont);
  }
  return true;
}
