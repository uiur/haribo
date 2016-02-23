#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USING 2

struct TIMERCTL timerctl;

void init_pit(void) {
  int i;

  io_out8(PIT_CTRL, 0x34);
  io_out8(PIT_CNT0, 0x9c);
  io_out8(PIT_CNT0, 0x2e);

  timerctl.count = 0;
  for (i = 0; i < sizeof(timerctl.timer); i++) {
    timerctl.timer[i].flags = 0;
  }

  return;
}

struct TIMER *timer_alloc(void) {
  int i;
  for (i = 0; i < sizeof(timerctl.timer); i++) {
    if (timerctl.timer[i].flags == 0) {
      timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
      return &timerctl.timer[i];
    }
  }

  return 0;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned int data) {
  timer->fifo = fifo;
  timer->data = data;

  return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
  timer->timeout = timeout;
  timer->flags = TIMER_FLAGS_USING;

  return;
}

void inthandler20(int *esp) {
  struct TIMER *timer;
  int i = 0;

  io_out8(PIC0_OCW2, 0x60);
  timerctl.count++;

  for (i = 0; i < sizeof(timerctl.timer); i++) {
    timer = &timerctl.timer[i];

    if (timer->flags == TIMER_FLAGS_USING) {
      timer->timeout--;

      if (timer->timeout == 0) {
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo8_put(timer->fifo, timer->data);
      }
    }
  }
  return;
}
