#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include "agent.h"
#include "error.h"

struct agent_args {
  struct mailbox *inbox;
  void (*handler) (struct message*);
  bool block;
};

int agent (void *data) {
  struct agent_args *args = (struct agent_args*) data;
  struct mailbox *inbox = args->inbox;
  bool block = args->block;
  bool stop = false;
  while (!stop && !panic) {
    struct message *mail = get_mail(inbox, block);
    if (mail != NULL && mail->type == STOP_MESSAGE_TYPE) {
      stop = true;
    } else {
      if (panic) break;
      args->handler(mail);
    }
    free(mail);
  }
}

struct mailbox* make_agent(pthread_t *thread, void (*handler) (struct message*), bool block) {
  struct agent_args *args = (struct agent_args*) malloc(sizeof(struct agent_args));
  if (args == NULL) {
    myperror("malloc");
    return NULL;
  }
  args->inbox = new_mailbox();
  if (args->inbox == NULL) {
    free(args);
    return NULL;
  }
  args->handler = handler;
  args->block = block;
  int err = pthread_create(thread, NULL, agent, args);
  if (err != 0) {
    myerror(err, "pthread_create");
    delete_mailbox(args->inbox);
    free(args);
    return NULL;
  }
  return args->inbox;
}
