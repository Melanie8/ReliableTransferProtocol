#include "mailbox.h"

struct mailbox* make_agent(pthread_t *thread, bool (*handler) (struct message*));
