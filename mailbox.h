#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include <stdbool.h>

struct mailbox;

enum message_type {
  STOP_MESSAGE_TYPE,              // 0
  ALARM_MESSAGE_TYPE,             // 1
  ACK_MESSAGE_TYPE,               // 2
  TIMEOUT_MESSAGE_TYPE,           // 3
  SEND_MESSAGE_TYPE,              // 4
  CONTINUE_ACKING_MESSAGE_TYPE,   // 5
  INIT_MESSAGE_TYPE               // 6
};

struct message {
  enum message_type type;
  void *data;
};

struct mailbox *new_mailbox ();
void delete_mailbox (struct mailbox *inbox);
struct message *get_mail (struct mailbox *inbox, bool block);
void send_mail (struct mailbox *inbox, struct message *m);

#endif
