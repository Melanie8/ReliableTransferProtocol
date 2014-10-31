#include "mailbox.h"

struct acker_init {
  struct mailbox *network_inbox;
  int sfd;
  bool verbose_flag;
};

bool acker (struct message *m);
