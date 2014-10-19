#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include "common.h"
#include "timer.h"

#define MILLION 1000000
//               123456

extern int min_usec;
long get_time_usec () {
  struct timeval now;
  gettimeofday(&now, NULL);
  // TODO error
  return ((long) now.tv_sec) * MILLION + now.tv_usec;
}

#define N 32

int key[N];
int back[N];
long time[N];
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
  if (n2 < n && time[key[n2]] < time[key[n1]]) {
    next = n2;
  }
  if (next < n && time[key[cur]] > time[key[next]]) {
    swap(cur, next);
    heap_down(next);
  }
}

void heap_up (int cur) {
  int prev = (cur - 1) / 2;
  if (prev >= 0 && time[key[cur]] < time[key[prev]]) {
    swap(cur, prev);
    heap_up(prev);
  }
}

void push_heap (int cur) {
  if (back[cur] != 0) {
    heap_down(back[cur]);
    heap_up(back[cur]);
  } else {
    key[n++] = cur;
    back[cur] = n;
    heap_up(n);
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
  return k;
}

struct mailbox *inbox[N];

void check_timers () {
  while (n > 0 && time[top_heap()] <= get_time_usec()) {
    int id = poll_heap();
    struct message *m = (struct message *) malloc(sizeof(struct message));
    m->type = TIMEOUT_MESSAGE_TYPE;
    struct alarm *alrm = (struct alarm *) malloc(sizeof(struct alarm));
    alrm->id = id;
    alrm->time = time[id];
    m->data = alrm;
    send_mail(inbox[id], m);
    time[id] = 0;
    inbox[id] = NULL;
  }
}

void timer (struct message *m) {
  if (m != NULL) {
    struct alarm *alrm = (struct alarm *) m->data;
    time[alrm->id] = alrm->time;
    inbox[alrm->id] = alrm->inbox;
    push_heap(alrm->id);
    free(alrm);
    free(m);
  }
  check_timers();
  int time_to_sleep = min_usec;
  if (n > 0) {
    time_to_sleep = MIN(time_to_sleep, time[top_heap()]);
  }
  int err = usleep(time_to_sleep);
  // TODO errors
  check_timers();
}
