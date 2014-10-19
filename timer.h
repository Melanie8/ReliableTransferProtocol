#include "mailbox.h"

struct alarm {
  int id;
  long time;
  struct mailbox *inbox;
};

long get_time_nsec ();
