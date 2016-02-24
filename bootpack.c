#include "bootpack.h"
#include <stdio.h>
#include <string.h>

void task_b_main(void);

void HariMain(void) {
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  char s[40], str[40] = "";
  char *key;
  int mx, my, i, task_b_esp;
  unsigned int memtotal;
  struct MOUSE_DEC mdec;
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  struct SHTCTL *shtctl;
  struct SHEET *sht_back, *sht_mouse, *sht_win;
  unsigned char *buf_back, buf_mouse[256], *buf_win;
  struct TIMER *timer, *timer10;

  struct FIFO32 fifo;
  int fifobuf[256];

  struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;

  struct TSS32 tss_a, tss_b;

  init_gdtidt();
  init_pic();
  io_sti(); /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
  fifo32_init(&fifo, sizeof(fifobuf), fifobuf);
  init_pit();
  io_out8(PIC0_IMR, 0xf8); /* PITとPIC1とキーボードを許可(11111000) */
  io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

  init_keyboard(&fifo);
  enable_mouse(&fifo, &mdec);
  memtotal = memtest(0x00400000, 0xbfffffff);
  memman_init(memman);
  memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
  memman_free(memman, 0x00400000, memtotal - 0x00400000);

  timer = timer_alloc();
  timer_init(timer, &fifo, 3);
  timer_settime(timer, 3 * TIMER_SECOND);

  timer10 = timer_alloc();
  timer_init(timer10, &fifo, 10);
  timer_settime(timer10, 10 * TIMER_SECOND);

  init_palette();
  shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
  sht_back = sheet_alloc(shtctl);
  sht_mouse = sheet_alloc(shtctl);
  sht_win = sheet_alloc(shtctl);
  buf_back =
      (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
  buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
  sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny,
               -1); /* 透明色なし */
  sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
  sheet_setbuf(sht_win, buf_win, 160, 52, -1); /* 透明色なし */
  init_screen8(buf_back, binfo->scrnx, binfo->scrny);
  init_mouse_cursor8(buf_mouse, 99);
  make_window8(buf_win, 160, 52, "counter");
  sheet_slide(sht_back, 0, 0);
  mx = (binfo->scrnx - 16) / 2; /* 画面中央になるように座標計算 */
  my = (binfo->scrny - 28 - 16) / 2;
  sheet_slide(sht_mouse, mx, my);
  sheet_slide(sht_win, 80, 72);
  sheet_updown(sht_back, 0);
  sheet_updown(sht_win, 1);
  sheet_updown(sht_mouse, 2);

  sprintf(s, "(%3d, %3d)", mx, my);
  putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, strlen(s));

  sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 * 1024),
          memman_total(memman) / 1024);
  putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, strlen(s));
  sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

  tss_a.ldtr = 0;
  tss_a.iomap = 0x40000000;
  tss_b.ldtr = 0;
  tss_b.iomap = 0x40000000;

  set_segmdesc(gdt + 3, 103, (int)&tss_a, AR_TSS32);
  set_segmdesc(gdt + 4, 103, (int)&tss_b, AR_TSS32);
  load_tr(3 * 8);

  task_b_esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
  tss_b.eip = (int)&task_b_main;
  tss_b.eflags = 0x00000202; /* IF = 1; */
  tss_b.eax = 0;
  tss_b.ecx = 0;
  tss_b.edx = 0;
  tss_b.ebx = 0;
  tss_b.esp = task_b_esp;
  tss_b.ebp = 0;
  tss_b.esi = 0;
  tss_b.edi = 0;
  tss_b.es = 1 * 8;
  tss_b.cs = 2 * 8;
  tss_b.ss = 1 * 8;
  tss_b.ds = 1 * 8;
  tss_b.fs = 1 * 8;
  tss_b.gs = 1 * 8;

  mt_init();

  for (;;) {
    sprintf(s, "%010d", timerctl.count);
    putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, strlen(s));

    io_cli();
    if (fifo32_status(&fifo) == 0) {
      io_sti();
    } else {
      i = fifo32_get(&fifo);

      if (256 <= i && i < 512) {
        i -= 256;
        io_sti();
        if (i < 0x58) {
          key = code_to_key(i);
          if (strlen(key) > 0) {
            if (strcmp(key, "backspace") == 0) {
              str[strlen(str) - 1] = '\0';
            } else {
              strncat(str, key, 1);
            }
          }
          putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, str, 40);
        }
      } else if (512 <= i && i <= 768) {
        io_sti();
        i -= 512;
        if (mouse_decode(&mdec, i) != 0) {
          /* マウスカーソルの移動 */
          mx += mdec.x;
          my += mdec.y;
          if (mx < 0) {
            mx = 0;
          }
          if (my < 0) {
            my = 0;
          }
          if (mx > binfo->scrnx - 1) {
            mx = binfo->scrnx - 1;
          }
          if (my > binfo->scrny - 1) {
            my = binfo->scrny - 1;
          }
          sprintf(s, "(%3d, %3d)", mx, my);
          putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s,
                            strlen(s));
          sheet_slide(sht_mouse, mx, my);
        }
      } else if (0 < i && i < 100) {
        io_sti();

        sprintf(s, "%d sec", i);
        putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_000000, s,
                          strlen(s));
      }
    }
  }
}

void task_b_main(void) {
  for (;;) {
    io_hlt();
  }
}
