#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "mailbox.h"
#include "timer.h"
#include "network.h"

struct network_message {
  struct network_message *next;
  long time;
  struct packet *p;
  bool ack;
};

extern int sfd;

struct network_message *first;
struct network_message *last;

extern int delay;
extern struct mailbox *acker_inbox;
extern struct mailbox *sr_inbox;
extern struct mailbox *timer_inbox;
extern struct mailbox *network_inbox;

void send_scheduled_sending () {
  // avoid overhead of calling it too many times
  int t = get_time_usec();
  while (first != NULL && first->time <= t) {
    if (first->ack) {
      struct message *m = (struct message*) malloc(sizeof(struct message));
      m->type = ACK_MESSAGE_TYPE;
      m->data = first->p;
      send_mail(sr_inbox, m);
    } else {
      // TODO do an agent that send because here we make the ack
      // waits while we are sending..
      if (write(sfd, first->p, PACKET_SIZE) != PACKET_SIZE) {
        fprintf(stderr, "partial/failed write\n");
        exit(EXIT_FAILURE);
      }
    }
    first = first->next;
    if (first == NULL) {
      last = NULL;
    }
  }
}

void schedule_sending (struct simulator_message *sm, bool ack) {
  send_scheduled_sending();
  // to avoid having the problem with 2 schedule with the same id

  struct network_message *nm = (struct network_message *) malloc(sizeof(struct network_message));
  // We only start time here (and not when SelectiveRepeat send it)
  // to simulate a bit of randomness (the time between SR et this network agent threating it)
  // --> to put in report
  // that way we are also sure that they are put in order in the list
  
  nm->time = get_time_usec() + delay * MILLION;
  nm->p = sm->p;
  nm->ack = ack;
  nm->next = NULL;
  if (last == NULL) {
    assert(first == NULL);
    first = nm;
  } else {
    last->next = nm;
  }
  last = nm;
  struct message *m = (struct message *) malloc(sizeof(struct message));
  m->type = ALARM_MESSAGE_TYPE;
  struct alarm *alrm = (struct alarm *) malloc(sizeof(struct alarm));
  // we multiply by 3 distinguish ids for timeout, acks and normal packets
  // if we previously used this id for the timer, this timer has necessarily already expired
  alrm->id = sm->id * 3 + ack;
  alrm->time = nm->time;
  alrm->inbox = network_inbox;
  m->data = alrm;
  send_mail(timer_inbox, m);
}


void network (struct message *m) {
  if (m->type == INIT_MESSAGE_TYPE) {
  } else if (m->type == SEND_MESSAGE_TYPE) {
    schedule_sending((struct simulator_message *) m->data, false);
  } else if (m->type == ACK_MESSAGE_TYPE) {
    schedule_sending((struct simulator_message *) m->data, true);
  } else {
    assert(m->type == TIMEOUT_MESSAGE_TYPE);
    send_scheduled_sending();
  }
}
