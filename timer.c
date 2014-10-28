#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>

#include "common.h"
#include "timer.h"

int delay;

long get_time_usec () {
  struct timeval now;
  gettimeofday(&now, NULL);
  // TODO error
  return ((long) now.tv_sec) * MILLION + now.tv_usec;
}

#define N_TIMER (3*(BUFFER_SIZE))

int key[N_TIMER];
int back[N_TIMER];
long timeout[N_TIMER];
int n = 0;

void swap (int a, int b) {
  int tmp = key[a];
  key[a] = key[b];
  key[b] = tmp;
  back[key[a]] = a;
  back[key[b]] = b;
}

void heap_down (int cur) {
  int n1 = cur * 2 + 1;
  int n2 = cur * 2 + 2;
  int next = n1;
  if (n2 < n && timeout[key[n2]] < timeout[key[n1]]) {
    next = n2;
  }
  if (next < n && timeout[key[cur]] > timeout[key[next]]) {
    swap(cur, next);
    heap_down(next);
  }
}

void heap_up (int cur) {
  int prev = (cur - 1) / 2;
  if (prev >= 0 && timeout[key[cur]] < timeout[key[prev]]) {
    swap(cur, prev);
    heap_up(prev);
  }
}

void push_heap (int cur) {
  if (back[cur] != -1) {
    heap_down(back[cur]);
    heap_up(back[cur]);
  } else {
    key[n] = cur;
    back[cur] = n;
    n++;
    heap_up(n-1);
  }
}

int top_heap () {
  return key[0];
}

int poll_heap () {
  int k = key[0];
  n--;
  if (n > 0) {
    key[0] = key[n];
    back[key[0]] = 0;
    heap_down(0);
  }
  back[k] = -1;
  return k;
}

struct mailbox *inbox[N_TIMER];

void check_timers () {
  while (n > 0 && timeout[top_heap()] <= get_time_usec()) {
    int id = poll_heap();
    //printf("%d polled\n", id);
    struct message *m = (struct message *) malloc(sizeof(struct message));
    m->type = TIMEOUT_MESSAGE_TYPE;
    struct alarm *alrm = (struct alarm *) malloc(sizeof(struct alarm));
    alrm->id = id;
    alrm->timeout = timeout[id];
    printf("timer   timeout %d:%ld\n", id, timeout[id]);
    alrm->inbox = NULL;
    m->data = alrm;
    printf("timer send    %d to ?\n", m->type);
    send_mail(inbox[id], m);
    timeout[id] = 0;
    inbox[id] = NULL;
  }
}

bool timer (struct message *m) {
  if (m != NULL) {
    printf("timer   receives %d\n", m->type);
    if (m->type == INIT_MESSAGE_TYPE) {
      struct timer_init *init = (struct timer_init*) m->data;
      //printf("%p\n", init);
      delay = init->delay;
      int i;
      for (i = 0; i < N_TIMER; i++) {
        back[i] = -1;
      }
    } else {
      assert(m->type == ALARM_MESSAGE_TYPE);
      struct alarm *alrm = (struct alarm *) m->data;
      timeout[alrm->id] = alrm->timeout;
      //printf("inbox::%p\n", inbox[alrm->id]);
      inbox[alrm->id] = alrm->inbox;
      //printf("inbox:%d:%p:%ld\n", alrm->id, inbox[alrm->id], alrm->timeout);
      push_heap(alrm->id);
      int i;
      for (i = 0; i < n; i++) {
        //printf("%d\n", key[i]);
      }
    }
  } else {
    printf("timer   receives NULL\n");
  }
  check_timers();
  // the minimum time that can be asked is MIN(delay*MILLION, 3*delay*MILLION) = delay*MILLION
  int time_to_sleep = delay * MILLION;
  if (n > 0) {
    time_to_sleep = MIN(time_to_sleep, timeout[top_heap()]);
  }
  //printf("%d %d %d %ld\n", delay, time_to_sleep, top_heap(), timeout[top_heap()]);
  int err = usleep(time_to_sleep);
  // TODO errors
  check_timers();
  return n == 0;
}
