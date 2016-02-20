#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo;

void HariMain(void)
{
  struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;

  init_gdtidt();
  init_pic();
  io_sti();

  init_palette();
  init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

  io_out8(PIC0_IMR, 0xf9);
  io_out8(PIC1_IMR, 0xef);

  int i;
  char s[40], keybuf[32];

  fifo8_init(&keyfifo, 32, keybuf);

  for (;;) {
    io_cli();

    if (fifo8_status(&keyfifo) == 0) {
      io_stihlt();
    } else {
      i = fifo8_get(&keyfifo);
      io_sti();
      sprintf(s, "%02X", i);
      boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
      putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
    }
  }
}

void init_palette(void)
{
  static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,
		0xff, 0x00, 0x00,
		0x00, 0xff, 0x00,
		0xff, 0xff, 0x00,
		0x00, 0x00, 0xff,
		0xff, 0x00, 0xff,
		0x00, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0xc6, 0xc6, 0xc6,
		0x84, 0x00, 0x00,
		0x00, 0x84, 0x00,
		0x84, 0x84, 0x00,
		0x00, 0x00, 0x84,
		0x84, 0x00, 0x84,
		0x00, 0x84, 0x84,
		0x84, 0x84, 0x84
	};

  set_palette(0, 15, table_rgb);

  return;
}
