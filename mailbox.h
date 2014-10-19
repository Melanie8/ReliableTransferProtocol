#include <stdbool.h>

struct mailbox;

enum message_type {
  STOP_MESSAGE_TYPE,
  ALARM_MESSAGE_TYPE,
  ACK_MESSAGE_TYPE,
  TIMEOUT_MESSAGE_TYPE,
  SEND_MESSAGE_TYPE
};

struct message {
  enum message_type type;
  void *data;
};

struct mailbox *new_mailbox ();
void delete_mailbox (struct mailbox *inbox);
struct message *get_mail (struct mailbox *inbox, bool block);
void send_mail (struct mailbox *inbox, struct message *m);
