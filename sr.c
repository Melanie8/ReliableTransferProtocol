#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
//#include <zlib.h>

#include "mailbox.h"
#include "common.h"
#include "timer.h"
#include "network.h"
#include "sr.h"
#include "error.h"

// initialized to 0
static bool verbose_flag;
static int delay;
static int fd;
static struct mailbox *network_inbox;
static struct mailbox *timer_inbox;
static struct mailbox *sr_inbox;

static ssize_t len = PAYLOAD_SIZE;
static ssize_t nread;
/*
 * The problem is that this is shared with the network simulator agent
 * if we want to read a new packet payload from the file but the old
 * one is still being resent by the network simulator, there is problem
 * Using MAX_SEQ, it is very unlikely. If it happens, the crc will be invalid if we modify it while it is sent or if it finishes, this will be the valid new packet
 */
static struct packet *sr_packets[MAX_SEQ];
static int window_size;
static int window_start;
static int start_in_window;
static int cur_seq;
static long int sr_timeout[MAX_WIN_SIZE];

enum ack_status {
  ack_status_none=0,
  ack_status_sent,
  ack_status_acked
};
enum ack_status status[MAX_WIN_SIZE];

void send_mail_to_network_simulator (int seq, int id_in_window, bool last) {
  struct message *m = (struct message*) malloc(sizeof(struct message));
  m->type = SEND_MESSAGE_TYPE;
  struct simulator_message *sm = (struct simulator_message*) malloc(sizeof(struct simulator_message));
  sm->id = id_in_window;
  sm->p = sr_packets[seq];
  sm->last = last;
  m->data = sm;
  if (verbose_flag)
    printf("SR      sends    %d to network\n", m->type);
  send_mail(network_inbox, m);
  status[id_in_window] = ack_status_sent;

  m = (struct message*) malloc(sizeof(struct message));
  m->type = ALARM_MESSAGE_TYPE;
  struct alarm *alrm = (struct alarm*) malloc(sizeof(struct alarm));
  alrm->id = id_in_window * 3 + 2;
  alrm->timeout = get_time_usec() + ((long) delay) * 3;
  sr_timeout[id_in_window] = alrm->timeout;
  //printf("SR %d->%ld\n", id_in_window, alrm->timeout);
  alrm->inbox = sr_inbox;
  m->data = alrm;
  if (verbose_flag)
    printf("SR      sends    %d to timer\n", m->type);
  send_mail(timer_inbox, m);
}

// FIXME we shouldn't have to add MAX_WIN_SIZE here
#define INDEX_IN_WINDOW(x) index_in_window(window_start, start_in_window, (x))
#define CUR_IN_WINDOW INDEX_IN_WINDOW(cur_seq)
#define CUR_PACKET (sr_packets[cur_seq])

void check_send () {
  while (fd > 0 && between_mod(window_start, (window_start + window_size) % MAX_SEQ, cur_seq)) {
    if (CUR_PACKET == NULL) {
      CUR_PACKET = (struct packet *) malloc(sizeof(struct packet));
    }
    assert(status[CUR_IN_WINDOW] != ack_status_acked);
    bool last = false;
    if (status[CUR_IN_WINDOW] == ack_status_none) {
      // if window_size has shrinked, it could already have been read
      len = read(fd, CUR_PACKET->payload, len);
      if (len < 0) {
        myperror("read");
        exit(EXIT_FAILURE); // FIXME do it ??
      }
      if (len != PAYLOAD_SIZE) {
        // end of file
        if (verbose_flag)
          printf("SR      end of file\n");
        fd = -1;
        last = true;
      }

      memset(CUR_PACKET->payload + len, 0, PAYLOAD_SIZE-len);

      CUR_PACKET->type_and_window_size = (PTYPE_DATA << 5);
      CUR_PACKET->seq = cur_seq;
      CUR_PACKET->len = htons(len);

      CUR_PACKET->crc = htonl(crc_packet(CUR_PACKET));
    }

    send_mail_to_network_simulator(cur_seq, CUR_IN_WINDOW, last);

    cur_seq = (cur_seq + 1) % MAX_SEQ;
  }
  if (fd < 0 && status[start_in_window] == ack_status_none) {
    struct message *m_network = (struct message *) malloc(sizeof(struct message));
    struct message *m_timer = (struct message *) malloc(sizeof(struct message));
    struct message *m_self = (struct message *) malloc(sizeof(struct message));
    m_network->type = STOP_MESSAGE_TYPE;
    m_network->data = NULL;
    m_timer->type = STOP_MESSAGE_TYPE;
    m_timer->data = NULL;
    m_self->type = STOP_MESSAGE_TYPE;
    m_self->data = NULL;
    if (verbose_flag)
      printf("SR    sends     %d to network\n", m_network->type);
    send_mail(network_inbox, m_network);
    if (verbose_flag)
      printf("SR    sends     %d to timer\n", m_timer->type);
    send_mail(timer_inbox, m_timer);
    if (verbose_flag)
      printf("SR    sends     %d to sr\n", m_self->type);
    send_mail(sr_inbox, m_self);
  }
}

bool selective_repeat (struct message *m) {
  if (verbose_flag)
    printf("SR      receives %d\n", m->type);
  if (m->type == INIT_MESSAGE_TYPE) {
    struct sr_init *init = (struct sr_init *) m->data;
    verbose_flag = init->verbose_flag;
    fd = init->fd;
    delay = init->delay;
    network_inbox = init->network_inbox;
    timer_inbox = init->timer_inbox;
    sr_inbox = init->sr_inbox;
    window_size = MAX_WIN_SIZE;
    int i;
    for (i = 0; i < MAX_WIN_SIZE; i++) {
      sr_timeout[i] = -1;
    }
    check_send();
  } else if (m->type == ACK_MESSAGE_TYPE) {
    struct packet *p = (struct packet *) m->data;
    if (valid_ack(p)) {
      if (verbose_flag)
        printf("SR      valid ack seq:%d\n", p->seq);
      int i = 0;
      int lastack = (p->seq + MAX_SEQ - 1) % MAX_SEQ;
      for (i = window_start; between_mod(i, (i + window_size) % MAX_SEQ, lastack); i++) {
        status[INDEX_IN_WINDOW(i)] = ack_status_acked;
      }
      while (status[start_in_window] == ack_status_acked) {
        status[start_in_window] = ack_status_none;
        sr_timeout[start_in_window] = -1;
        if (verbose_flag)
          printf("window_start : %d",window_start);
        window_start = (window_start + 1) % MAX_SEQ;
        start_in_window = (start_in_window + 1) % MAX_WIN_SIZE;
        if (verbose_flag)
          printf("->%d\n",window_start);
      }
      window_size = p->type_and_window_size & MAX_WIN_SIZE;
    } else {
      if (verbose_flag)
        printf("SR      invalid ack seq:%d\n", p->seq);
    }
    check_send();
  } else if (m->type == TIMEOUT_MESSAGE_TYPE) {
    struct alarm *alrm = m->data;
    int id = alrm->id / 3;
    if (between_mod(start_in_window, (start_in_window + window_size) % MAX_WIN_SIZE, id)
        && status[id] == ack_status_sent
        && sr_timeout[id] == alrm->timeout) {
      // last_seq has already been registered
      send_mail_to_network_simulator(index_to_seq(window_start, start_in_window, id), id, false);
    }
  } else {
    assert(m->type == STOP_MESSAGE_TYPE);
    int i;
    for (i = 0; i < MAX_SEQ; i++) {
      if (sr_packets[i] != NULL) {
        free(sr_packets[i]);
      }
    }
  }
  return true;
}
