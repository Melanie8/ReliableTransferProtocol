struct simulator_message {
  int id;
  struct packet *p;
};

struct network_init {
  int sfd;
  int delay;
  struct mailbox *acker_inbox;
  struct mailbox *sr_inbox;
  struct mailbox *timer_inbox;
  struct mailbox *network_inbox;
};

bool network (struct message *m);
