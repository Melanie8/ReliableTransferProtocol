#include <assert.h>
#include <arpa/inet.h>

#include "mailbox.h"

extern delay;

// initialized to 0
int fd;
size_t len = PAYLOAD_SIZE;
ssize_t nread;
int seq;
struct packet *packets[MAX_WIN_SIZE];
int window_size;
int window_start;
int start_in_window;
int cur_seq;
int timeout[MAX_WIN_SIZE];

enum ack_status {
  ack_status_none=0,
  ack_status_sent,
  ack_status_acked
};
enum ack_status status[MAX_WIN_SIZE];

void send_message_to_network_simulator (int id_in_window) {
  struct message *m = (struct message*) malloc(sizeof(struct message));
  m->type = SEND_MESSAGE_TYPE;
  struct simulator_message *sm = (struct simulator_message*) malloc(sizeof(struct simulator_message));
  sm->id = id_in_window;
  sm->p = packets[id_in_window];
  m->data = sm;
  send_message(network_inbox, m);
  status[id_in_window] = ack_status_sent;

  m = (struct message*) malloc(sizeof(struct message));
  m->type = ALARM_MESSAGE_TYPE;
  struct alarm *alrm = (struct alarm*) malloc(sizeof(struct alarm));
  alrm->id = id_in_window * 3 + 2;
  alrm->time = get_time_usec + delay * 3 * MILLION;
  alrm->inbox = sr_inbox;
  m->data = alrm;
  send_message(timer_inbox, m);
}

#define INDEX_IN_WINDOW(x) ((MAX_WIN_SIZE + (x) - window_start) % MAX_WIN_SIZE)
#define CUR_IN_WINDOW INDEX_IN_WINDOW(cur_seq)
#define CUR_PACKET packets[CUR_IN_WINDOW]

bool between_mod (int a, int b, int m) {
  if (a <= b) {
    return m >= a && m < b;
  } else {
    return m >= a || m < b;
  }
}

void check_send () {
  while (fd > 0 && between_mod(start_in_window, (start_in_window + window_size) % MAX_WIN_SIZE, CUR_IN_WINDOW)) {
    len = read(fd, CUR_PACKET->payload, len);
    if (len < 0) {
      myperror("read");
      exit(EXIT_FAILURE); // FIXME do it ??
    }
    if (len != PAYLOAD_SIZE) {
      close_fd(fd);
      fd = -1;
    }

    memset(CUR_PACKET->payload + len, 0, PAYLOAD_SIZE-len);

    CUR_PACKET->type_and_window_size = (PTYPE_DATA << 5);
    CUR_PACKET->seq = cur_seq;
    CUR_PACKET->len = htons(len);
    printf("%d\n", CUR_PACKET->len);

    CUR_PACKET->crc = htonl(rc_crc32(0, packet, 4 + PAYLOAD_SIZE));

    send_message_to_network_simulator(CUR_IN_WINDOW);

    cur_seq = (cur_seq + 1) % MAX_SEQ;
  }
}

void selective_repeat (struct message *m) {
  if (m->type == INIT_MESSAGE_TYPE) {
    fd = get_fd(filename, false);
    check_send();
  } else if (m->type == ACK_MESSAGE) {
    // TODO free
    struct simulator_message *sm = m->data;
    uint32_t expected_crc = rc_crc32(0, sm->p, 4 + PAYLOAD_SIZE);
    if (expected_crc == ntohl(sm->p->crc)) {
      // FIXME check len is 0 ??
      for (int i = window_start; between_mod(i, (i + window_size) % MAX_SEQ, sm->p->seq)) {
        status[i] = ack_status_acked;
      }
      while (status[window_start] == ack_status_acked) {
        free(packets[window_start]);
        packets[window_start] = NULL;
        status[window_start] = ack_status_none;
        timeout[window_start] = -1;
        window_start++;
      }
    }
    check_send();
  } else {
    assert(m->type == TIMEOUT_MESSAGE);
    struct alarm *alrm = m->data;
    if (between_mod(start_in_window, (start_in_window + window_size) % MAX_WIN_SIZE, alrm->id)
        && status[alrm->id] == ack_status_sent
        && timer[alrm->id] == alrm->time) {
      send_message_to_network_simulator(alrm->id);
    }
  }
  free(m);
}
