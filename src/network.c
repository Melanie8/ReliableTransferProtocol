#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "common.h"
#include "timer.h"
#include "network.h"

struct network_message {
  struct network_message *next;
  long time;
  struct packet *p;
  bool ack;
};

static int sber;
static int splr;
static int sfd;
static int delay;
static struct mailbox *acker_inbox;
static struct mailbox *sr_inbox;
static struct mailbox *timer_inbox;
static struct mailbox *network_inbox;
static int last_seq;
static bool continue_acking;
static bool verbose_flag;

// initialized to 0 (i.e. NULL)
static struct network_message *first;
static struct network_message *last;

void send_scheduled_sending () {
  // avoid overhead of calling it too many times
  long t = get_time_usec();
  while (first != NULL && first->time <= t) {
    if (first->ack) {
      struct message *m = get_stop_message();
      if (m != NULL) {
        struct simulator_message *sm = (struct simulator_message*) malloc(sizeof(struct simulator_message));
        if (sm == NULL) {
          free(m);
          myperror("malloc");
        } else {
          sm->id = 0; // unused
          sm->last = false; // unused
          sm->p = first->p;
          m->type = ACK_MESSAGE_TYPE;
          m->data = sm;
          if (verbose_flag)
            printf("network send     %d to SR\n", m->type);
          send_mail(sr_inbox, m);
        }
      }
    } else {
      // TODO do an agent that send because here we make the ack
      // waits while we are sending..
      int xor = 0;
      if ((rand() % 1000) < sber) {
        xor = 0xff;
      }
      first->p->payload[0] ^= xor;
      if (write(sfd, first->p, PACKET_SIZE) != PACKET_SIZE) {
        myserror("failed to write, the receiver might have terminate");
        myperror("write");
      }
      // don't the error for the next time too
      first->p->payload[0] ^= xor;
    }
    struct network_message *oldfirst = first;
    first = first->next;
    free(oldfirst);
    if (first == NULL) {
      last = NULL;
    }
  }
}

void schedule_sending (struct simulator_message *sm, bool ack) {
  send_scheduled_sending();
  // to avoid having the problem with 2 schedule with the same id

  struct network_message *nm = (struct network_message *) malloc(sizeof(struct network_message));

  if (nm == NULL) {
    myperror("malloc");
  } else {
    // We only start time here (and not when SelectiveRepeat send it)
    // to simulate a bit of randomness (the time between SR et this network agent threating it)
    // --> to put in report
    // that way we are also sure that they are put in order in the list

    nm->time = get_time_usec() + (long) delay;
    nm->p = sm->p;
    nm->ack = ack;
    nm->next = NULL;
    if (last == NULL) {
      assert(first == NULL);
      first = nm;
    } else {
      assert(first != NULL);
      last->next = nm;
    }
    last = nm;
    struct message *m = get_stop_message();
    if (m != NULL) {
      m->type = ALARM_MESSAGE_TYPE;
      struct alarm *alrm = (struct alarm *) malloc(sizeof(struct alarm));
      if (alrm == NULL) {
        free(nm);
        free(m);
        myperror("malloc");
      } else {
        // we multiply by 3 distinguish ids for timeout, acks and normal packets
        // if we previously used this id for the timer, this timer has necessarily already expired
        alrm->id = sm->id * 3 + ack;
        alrm->timeout = nm->time;
        alrm->inbox = network_inbox;
        m->data = alrm;
        if (verbose_flag)
          printf("network send     %d to timer\n", m->type);
        send_mail(timer_inbox, m);
      }
    } else {
      free(nm);
    }
  }
}


bool network (struct message *m) {
  if (verbose_flag)
    printf("network receives %d\n", m->type);
  if (m->type == INIT_MESSAGE_TYPE) {
    struct network_init *init = (struct network_init *) m->data;
    verbose_flag = init->verbose_flag;
    sber = init->sber;
    splr = init->splr;
    sfd = init->sfd;
    delay = init->delay;
    acker_inbox = init->acker_inbox;
    sr_inbox = init->sr_inbox;
    timer_inbox = init->timer_inbox;
    network_inbox = init->network_inbox;
    last_seq = -1;
    continue_acking = true;
    srand(time(NULL));
  } else if (m->type == SEND_MESSAGE_TYPE) {
    struct simulator_message *sm = (struct simulator_message *) m->data;
    if (sm->last) {
      last_seq = (sm->p->seq + 1) % MAX_SEQ;
      if (verbose_flag)
        printf("LAST_SEQ : %d\n", last_seq);
    }
    if ((rand() % 1000) >= splr) {
      schedule_sending(sm, false);
    } else {
      if (verbose_flag)
        printf("network DROPPED packet %d\n", sm->p->seq);
    }
  } else if (m->type == ACK_MESSAGE_TYPE) {
    struct simulator_message *sm = (struct simulator_message *) m->data;
    schedule_sending(sm, true);
    if (valid_ack(sm->p) && last_seq != -1 && sm->p->seq == last_seq) {
      continue_acking = false;
    }
  } else if (m->type == CONTINUE_ACKING_MESSAGE_TYPE) {
    struct message *cont = get_stop_message();
    if (cont != NULL) {
      if (continue_acking) {
        cont->type = CONTINUE_ACKING_MESSAGE_TYPE;
      } else {
        cont->type = STOP_MESSAGE_TYPE;
      }
      if (verbose_flag)
        printf("network sends    %d to acker\n", m->type);
      send_mail(acker_inbox, cont);
    }
  } else if (m->type == TIMEOUT_MESSAGE_TYPE) {
    send_scheduled_sending();
  } else {
    assert(m->type == STOP_MESSAGE_TYPE);
    while (first != NULL) {
      struct network_message *oldfirst = first;
      first = first->next;
      if (oldfirst->ack) {
        free(oldfirst->p);
      }
      free(oldfirst);
    }
    last = NULL;
    // SR can free the packets now
    send_mail(sr_inbox, get_stop_message());
    // in case of failure
    send_mail(acker_inbox, get_stop_message());
  }
  return true;
}
