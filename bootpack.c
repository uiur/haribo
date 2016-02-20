#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

unsigned int memtest(unsigned int start, unsigned int end);

struct MOUSE_DEC mdec;
void HariMain(void) {
  struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
  int i, mx, my;
  char s[40], keybuf[32], mousebuf[128];

  init_gdtidt();
  init_pic();
  io_sti();
  fifo8_init(&keyfifo, sizeof(keybuf), keybuf);
  fifo8_init(&mousefifo, sizeof(mousebuf), mousebuf);

  io_out8(PIC0_IMR, 0xf9);
  io_out8(PIC1_IMR, 0xef);

  init_keyboard();

  init_palette();
  init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

  enable_mouse(&mdec);

	mx = binfo->scrnx / 2 - 4;
	my = binfo->scrny / 2 - 8;
	putfonts8_asc(binfo->vram, binfo->scrnx, mx, my, COL8_FFFFFF, "*");

	i = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
	sprintf(s, "memory %dMB", i);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 48, COL8_FFFFFF, s);

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

		      boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 8 * 12 - 1, 31);
		      putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

		      boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 8 - 1, my + 16 - 1);

					mx += mdec.x;
					my += mdec.y;

					putfonts8_asc(binfo->vram, binfo->scrnx, mx, my, COL8_FFFFFF, "*");
				}
	    }
		}
  }
}

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start, unsigned int end) {
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT;
	io_store_eflags(eflg);

	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) {
		flg486 = 1;
	}

	eflg &= ~EFLAGS_AC_BIT;

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	return i;
}
