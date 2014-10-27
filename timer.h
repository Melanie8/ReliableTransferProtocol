#ifndef _TIMER_H_
#define _TIMER_H_

#include "mailbox.h"

struct alarm {
  int id;
  long time;
  struct mailbox *inbox;
};

struct timer_init {
  int delay;
};

long get_time_usec ();

#define MILLION 1000000
//               123456

bool timer (struct message *m);

#endif
