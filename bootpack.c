#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

#define MEMMAN_ADDR 0x003c0000;

struct MOUSE_DEC mdec;
void HariMain(void) {
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;

  int i, mx, my;
  char s[40], keybuf[32], mousebuf[128];

  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  unsigned int memtotal;

  init_gdtidt();
  init_pic();
  io_sti();
  fifo8_init(&keyfifo, sizeof(keybuf), keybuf);
  fifo8_init(&mousefifo, sizeof(mousebuf), mousebuf);

  io_out8(PIC0_IMR, 0xf9);
  io_out8(PIC1_IMR, 0xef);

  init_keyboard();

  memtotal = memtest(0x00400000, 0xbfffffff);
  memman_init(memman);
  memman_free(memman, 0x00001000, 0x0009e000);
  memman_free(memman, 0x00400000, memtotal - 0x00400000);

  init_palette();
  init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
  enable_mouse(&mdec);

  mx = binfo->scrnx / 2 - 4;
  my = binfo->scrny / 2 - 8;
  putfonts8_asc(binfo->vram, binfo->scrnx, mx, my, COL8_FFFFFF, "*");

  sprintf(s, "memory %dMB, free: %dKB", memtotal / (1024 * 1024),
          memman_total(memman) / 1024);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

  for (;;) {
    io_cli();

    if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0) {
      io_stihlt();
    } else {
      if (fifo8_status(&keyfifo) != 0) {
        i = fifo8_get(&keyfifo);
        io_sti();
        sprintf(s, "%02X", i);
        boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
        putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
      } else if (fifo8_status(&mousefifo) != 0) {
        i = fifo8_get(&mousefifo);
        io_sti();

        if (mouse_decode(&mdec, i) > 0) {
          sprintf(s, "%d %4d %4d", mdec.btn, mdec.x, mdec.y);

          boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16,
                   32 + 8 * 12 - 1, 31);
          putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

          boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 8 - 1,
                   my + 16 - 1);

          mx += mdec.x;
          my += mdec.y;

          putfonts8_asc(binfo->vram, binfo->scrnx, mx, my, COL8_FFFFFF, "*");
        }
      }
    }
  }
}
