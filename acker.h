#include "mailbox.h"

struct acker_init {
  struct mailbox *network_inbox;
  int sfd;
};

bool acker (struct message *m);
