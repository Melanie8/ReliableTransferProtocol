#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>

#include "common.h"
#include "timer.h"

static int delay;
static bool verbose_flag;

long get_time_usec () {
  struct timeval now;
  gettimeofday(&now, NULL);
  // TODO error
  return ((long) now.tv_sec) * MILLION + now.tv_usec;
}

#define N_TIMER (3*(MAX_WIN_SIZE))

int key[N_TIMER];
int back[N_TIMER];
long timer_timeout[N_TIMER];
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
  if (n2 < n && timer_timeout[key[n2]] < timer_timeout[key[n1]]) {
    next = n2;
  }
  if (next < n && timer_timeout[key[cur]] > timer_timeout[key[next]]) {
    swap(cur, next);
    heap_down(next);
  }
}

void heap_up (int cur) {
  int prev = (cur - 1) / 2;
  if (prev >= 0 && timer_timeout[key[cur]] < timer_timeout[key[prev]]) {
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
  while (n > 0 && timer_timeout[top_heap()] <= get_time_usec()) {
    int id = poll_heap();
    struct message *m = (struct message *) malloc(sizeof(struct message));
    m->type = TIMEOUT_MESSAGE_TYPE;
    struct alarm *alrm = (struct alarm *) malloc(sizeof(struct alarm));
    alrm->id = id;
    alrm->timeout = timer_timeout[id];
    alrm->inbox = NULL;
    m->data = alrm;
    send_mail(inbox[id], m);
    timer_timeout[id] = 0;
    inbox[id] = NULL;
  }
}

bool timer (struct message *m) {
  if (m != NULL) {
    if (verbose_flag)
      printf("timer   receives %d\n", m->type);
    if (m->type == INIT_MESSAGE_TYPE) {
      struct timer_init *init = (struct timer_init*) m->data;
      delay = init->delay;
      verbose_flag = init->verbose_flag;
      int i;
      for (i = 0; i < N_TIMER; i++) {
        back[i] = -1;
      }
    } else if (m->type == ALARM_MESSAGE_TYPE) {
      struct alarm *alrm = (struct alarm *) m->data;
      timer_timeout[alrm->id] = alrm->timeout;
      inbox[alrm->id] = alrm->inbox;
      push_heap(alrm->id);
      check_timers();
    } else {
      assert(m->type == STOP_MESSAGE_TYPE);
    }
  } else {
    if (verbose_flag)
      printf("timer   receives NULL\n");
    // the minimum time that can be asked is MIN(delay, 3*delay) = delay
    int time_to_sleep = delay;
    if (n > 0) {
      time_to_sleep = MIN(time_to_sleep, timer_timeout[top_heap()]);
    }
    int err = usleep(time_to_sleep);
    // TODO errors
    check_timers();
  }
  return n == 0;
}
