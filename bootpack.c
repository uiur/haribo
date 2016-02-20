#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};

struct MOUSE_DEC mdec;

void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

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

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready(void) {
  for (;;) {
    if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
      break;
    }
  }

  return;
}

void init_keyboard(void) {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, KBC_MODE);

  return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct MOUSE_DEC *mdec) {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);

	mdec->phase = 0;

  return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data) {
	if (mdec->phase == 0) {
		if (data == 0xfa) {
			mdec->phase++;
		}

		return 0;
	} else if (mdec->phase == 1) {
		if ((data & 0xc8) == 0x08) {
			mdec->buf[0] = data;
			mdec->phase++;
		}

		return 0;
	} else if (mdec->phase == 2) {
		mdec->buf[1] = data;
		mdec->phase++;

		return 0;
	} else if (mdec->phase == 3) {
		mdec->buf[2] = data;
		mdec->phase = 1;

		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];

		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}

		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}

		mdec->y *= -1;

		return 1;
	}

	return -1;
}
