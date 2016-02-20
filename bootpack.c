#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

unsigned int memtest(unsigned int start, unsigned int end);

#define MEMMAN_ADDR 0x003c0000;

struct MOUSE_DEC mdec;
void HariMain(void) {
  struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;

  int i, mx, my;
  char s[40], keybuf[32], mousebuf[128];

	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
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

	sprintf(s, "memory %dMB, free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
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

void memman_init(struct MEMMAN *man) {
	man->frees = 0;
	man->maxfrees = 0;
	man->lostsize = 0;
	man->losts = 0;
	return;
}

unsigned int memman_total(struct MEMMAN *man) {
	unsigned int i, total = 0;

	for (i = 0; i < man->frees; i++) {
		total += man->free[i].size;
	}

	return total;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size > size) {
			a = man->free[i].addr;

			man->free[i].addr += size;
			man->free[i].size -= size;

			if (man->free[i].size == 0) {
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i+1];
				}
			}

			return a;
		}
	}

	return 0;
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
	int i, j;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}

	/* man->free[i-1].addr < addr < man->free[i].addr */
	if (i > 0) {
		struct FREEINFO *prev = &man->free[i-1];
		if (prev->addr + prev->size == addr) {
			prev->size += size;

			if (i < man->frees) {
				struct FREEINFO *next = &man->free[i];
				if (addr + size == next->addr) {
					prev->size += next->size;

					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i+1];
					}
				}
			}
			return 0;
		}
	}

	if (i < man->frees) {
		struct FREEINFO *next = &man->free[i];
		if (addr + size == next->addr) {
			next->addr = addr;
			next->size += size;

			return 0;
		}
	}

	if (man->frees < MEMMAN_FREES) {
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j-1];
		}

		man->frees++;

		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees;
		}

		man->free[i].addr = addr;
		man->free[i].size = size;

		return 0;
	}

	man->losts++;
	man->lostsize += size;

	return -1;
}
