#include "mailbox.h"

struct acker_init {
  struct mailbox *network_inbox;
};

bool acker (struct message *m);
