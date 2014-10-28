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
int delay;
int fd;
struct mailbox *network_inbox;
struct mailbox *timer_inbox;
struct mailbox *sr_inbox;

ssize_t len = PAYLOAD_SIZE;
ssize_t nread;
struct packet *packets[MAX_WIN_SIZE];
int window_size;
int window_start;
int start_in_window;
int cur_seq;
long int timeout[MAX_WIN_SIZE];

enum ack_status {
  ack_status_none=0,
  ack_status_sent,
  ack_status_acked
};
enum ack_status status[MAX_WIN_SIZE];

void send_mail_to_network_simulator (int id_in_window, bool last) {
  struct message *m = (struct message*) malloc(sizeof(struct message));
  m->type = SEND_MESSAGE_TYPE;
  struct simulator_message *sm = (struct simulator_message*) malloc(sizeof(struct simulator_message));
  sm->id = id_in_window;
  sm->p = packets[id_in_window];
  sm->last = last;
  m->data = sm;
  send_mail(network_inbox, m);
  status[id_in_window] = ack_status_sent;

  m = (struct message*) malloc(sizeof(struct message));
  m->type = ALARM_MESSAGE_TYPE;
  struct alarm *alrm = (struct alarm*) malloc(sizeof(struct alarm));
  alrm->id = id_in_window * 3 + 2;
  alrm->timeout = get_time_usec() + ((long) delay) * 3;
  timeout[id_in_window] = alrm->timeout;
  alrm->inbox = sr_inbox;
  //printf("inbox:%p\n", network_inbox);
  m->data = alrm;
  send_mail(timer_inbox, m);
}

// FIXME we shouldn't have to add MAX_WIN_SIZE here
#define INDEX_IN_WINDOW(x) ((MAX_WIN_SIZE + (x) - window_start) % MAX_WIN_SIZE)
#define CUR_IN_WINDOW INDEX_IN_WINDOW(cur_seq)
#define CUR_PACKET packets[CUR_IN_WINDOW]

bool between_mod (int a, int b, int m) {
  if (a < b) {
    return m >= a && m < b;
  } else {
    return m >= a || m < b;
  }
}

void check_send () {
  //printf("check_send %d %d %d %d\n", start_in_window, CUR_IN_WINDOW, window_size, fd);
  while (fd > 0 && between_mod(start_in_window, (start_in_window + window_size) % MAX_WIN_SIZE, CUR_IN_WINDOW)) {
    CUR_PACKET = (struct packet *) malloc(sizeof(struct packet));
    len = read(fd, CUR_PACKET->payload, len);
    if (len < 0) {
      myperror("read");
      exit(EXIT_FAILURE); // FIXME do it ??
    }
    bool last = false;
    if (len != PAYLOAD_SIZE) {
      // end of file
      printf("sr      end of file\n");
      close_fd(fd);
      fd = -1;
      last = true;
    }

    memset(CUR_PACKET->payload + len, 0, PAYLOAD_SIZE-len);

    CUR_PACKET->type_and_window_size = (PTYPE_DATA << 5);
    CUR_PACKET->seq = cur_seq;
    CUR_PACKET->len = htons(len);
    //printf("%d\n", CUR_PACKET->len);

    //CUR_PACKET->crc = htonl((uint32_t) crc32(0, (const Bytef *)CUR_PACKET, PACKET_SIZE-CRC_SIZE));
    CUR_PACKET->crc = htonl(rc_crc32(CUR_PACKET));
    //printf("sr %lu %lu\n", CUR_PACKET->crc, ntohl(CUR_PACKET->crc));
    //printf("sr %d\n", CUR_PACKET->type_and_window_size);
    //printf("sr %d\n", CUR_PACKET->seq);
    //printf("sr %u\n", CUR_PACKET->len);

    send_mail_to_network_simulator(CUR_IN_WINDOW, last);

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
    printf("sr      sends    %d to network\n", m_network->type);
    send_mail(network_inbox, m_network);
    printf("sr      sends    %d to timer\n", m_timer->type);
    send_mail(timer_inbox, m_timer);
    printf("sr      sends    %d to sr\n", m_self->type);
    send_mail(sr_inbox, m_self);
  }
}

bool selective_repeat (struct message *m) {
  printf("sr      receives %d init:%d ack:%d timeout:%d\n", m->type, INIT_MESSAGE_TYPE, ACK_MESSAGE_TYPE, TIMEOUT_MESSAGE_TYPE);
  if (m->type == INIT_MESSAGE_TYPE) {
    struct sr_init *init_data = (struct sr_init *) m->data;
    fd = init_data->fd;
    delay = init_data->delay;
    network_inbox = init_data->network_inbox;
    timer_inbox = init_data->timer_inbox;
    sr_inbox = init_data->sr_inbox;
    window_size = MAX_WIN_SIZE;
    int i;
    for (i = 0; i < MAX_WIN_SIZE; i++) {
      timeout[i] = -1;
    }
    check_send();
  } else if (m->type == ACK_MESSAGE_TYPE) {
    struct packet *p = (struct packet *) m->data;
    if (valid_ack(p)) {
      printf("sr    valid ack seq:%d\n", ntohs(p->seq));
      int i = 0;
      for (i = window_start; between_mod(i, (i + window_size) % MAX_SEQ, p->seq); i++) {
        status[INDEX_IN_WINDOW(i)] = ack_status_acked;
      }
      while (status[window_start] == ack_status_acked) {
        free(packets[window_start]);
        packets[window_start] = NULL;
        status[window_start] = ack_status_none;
        timeout[window_start] = -1;
        window_start = (window_start + 1) % MAX_SEQ;
      }
    } else {
      printf("sr  invalid ack seq:%d\n", ntohs(p->seq));
    }
    check_send();
  } else {
    assert(m->type == TIMEOUT_MESSAGE_TYPE);
    struct alarm *alrm = m->data;
    int id = alrm->id / 3;
    //printf("-->%d %d %ld %ld\n", id, status[id], timeout[id], alrm->timeout);
    if (between_mod(start_in_window, (start_in_window + window_size) % MAX_WIN_SIZE, id)
        && status[id] == ack_status_sent
        && timeout[id] == alrm->timeout) {
      // last_seq has already been registered
      send_mail_to_network_simulator(id, false);
    }
  }
  return true;
}
