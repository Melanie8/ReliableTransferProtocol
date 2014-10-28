#ifndef SR_H
#define SR_H

#include "mailbox.h"

struct sr_init {
  int fd;
  int delay;
  struct mailbox *network_inbox;
  struct mailbox *timer_inbox;
  struct mailbox *sr_inbox;
};

bool selective_repeat (struct message *m);

#endif