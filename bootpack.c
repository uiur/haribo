#include "bootpack.h"
#include <stdio.h>

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

  struct SHTCTL *shtctl;
  struct SHEET *sht_back, *sht_mouse;
  unsigned char *buf_back, buf_mouse[16 * 16];

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

  shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);

  buf_back =
      (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrnx);
  sht_back = sheet_alloc(shtctl);
  sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
  init_screen(buf_back, binfo->scrnx, binfo->scrny);
  sheet_slide(shtctl, sht_back, 0, 0);

  enable_mouse(&mdec);

  sht_mouse = sheet_alloc(shtctl);
  sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
  init_mouse_cursor8(buf_mouse, 99);

  mx = (binfo->scrnx - 16) / 2;
  my = (binfo->scrny - 28 - 16) / 2;
  sheet_slide(shtctl, sht_mouse, mx, my);

  sheet_updown(shtctl, sht_back, 0);
  sheet_updown(shtctl, sht_mouse, 1);

  sprintf(s, "memory %dMB, free: %dKB, %d %d", memtotal / (1024 * 1024),
          memman_total(memman) / 1024, sht_mouse->bxsize, sht_mouse->bysize);
  putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

  sheet_refresh(shtctl);

  for (;;) {
    io_cli();

    if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0) {
      io_stihlt();
    } else {
      if (fifo8_status(&keyfifo) != 0) {
        i = fifo8_get(&keyfifo);
        io_sti();
        sprintf(s, "%02X", i);
        boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
        putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
      } else if (fifo8_status(&mousefifo) != 0) {
        i = fifo8_get(&mousefifo);
        io_sti();

        if (mouse_decode(&mdec, i) > 0) {
          mx += mdec.x;
          my += mdec.y;

          if (mx < 0) {
            mx = 0;
          }

          if (my < 0) {
            my = 0;
          }

          if (mx > binfo->scrnx - sht_mouse->bxsize) {
            mx = binfo->scrnx - sht_mouse->bxsize;
          }

          if (my > binfo->scrny - sht_mouse->bysize) {
            my = binfo->scrny - sht_mouse->bysize;
          }

          sprintf(s, "(%3d,%3d)", mx, my);
          boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 8 * 12 - 1,
                   31);
          putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
          // sheet_refreshsub(shtctl, 32, 16, 8 * 12, 16);

          sheet_slide(shtctl, sht_mouse, mx, my);
        }
      }
    }
  }
}
