#include "bootpack.h"

#define PORT_KEYSTA 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47

void wait_KBC_sendready(void) {
  for (;;) {
    if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
      break;
    }
  }

  return;
}

struct FIFO32 *keyfifo;

void init_keyboard(struct FIFO32 *fifo) {
  keyfifo = fifo;

  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, KBC_MODE);

  return;
}

void inthandler21(int *esp) {
  int data;
  io_out8(PIC0_OCW2, 0x61);

  data = io_in8(PORT_KEYDAT);
  fifo32_put(keyfifo, data + 256);

  return;
}

static char *keytable[0x54] = {
    "",  "",  "1",         "2",   "3", "4", "5", "6", "7", "8", "9", "0",
    "-", "=", "backspace", "tab", "Q", "W", "E", "R", "T", "Y", "U", "I",
    "O", "P", "@",         "[",   "",  "",  "A", "S", "D", "F", "G", "H",
    "J", "K", "L",         ";",   ":", "0", "",  "]", "Z", "X", "C", "V",
    "B", "N", "M",         ",",   ".", "/", 0,   "*", "",  " ", "0", "0",
    "0", "0", "0",         "0",   "0", "0", "0", "0", "0", "0", "",  "7",
    "8", "9", "-",         "4",   "5", "6", "+", "1", "2", "3", "0", "."};

char *code_to_key(int code) {
  if (code >= sizeof(keytable)) {
    return "";
  }

  return keytable[code];
}
