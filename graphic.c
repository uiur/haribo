#include "bootpack.h"

void init_screen8(char *vram, int x, int y) {
  boxfill8(vram, x, COL8_008484, 0, 0, x - 1, y - 29);
  boxfill8(vram, x, COL8_C6C6C6, 0, y - 28, x - 1, y - 28);
  boxfill8(vram, x, COL8_FFFFFF, 0, y - 27, x - 1, y - 27);
  boxfill8(vram, x, COL8_C6C6C6, 0, y - 26, x - 1, y - 1);

  boxfill8(vram, x, COL8_FFFFFF, 3, y - 24, 59, y - 24);
  boxfill8(vram, x, COL8_FFFFFF, 2, y - 24, 2, y - 4);
  boxfill8(vram, x, COL8_848484, 3, y - 4, 59, y - 4);
  boxfill8(vram, x, COL8_848484, 59, y - 23, 59, y - 5);
  boxfill8(vram, x, COL8_000000, 2, y - 3, 59, y - 3);
  boxfill8(vram, x, COL8_000000, 60, y - 24, 60, y - 3);

  boxfill8(vram, x, COL8_848484, x - 47, y - 24, x - 4, y - 24);
  boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y - 4);
  boxfill8(vram, x, COL8_FFFFFF, x - 47, y - 3, x - 4, y - 3);
  boxfill8(vram, x, COL8_FFFFFF, x - 3, y - 24, x - 3, y - 3);
  return;
}

void set_palette(int start, int end, unsigned char *rgb) {
  int i, eflags;
  eflags = io_load_eflags();
  io_cli();
  io_out8(0x03c8, start);
  for (i = start; i <= end; i++) {
    io_out8(0x03c9, rgb[0] / 4);
    io_out8(0x03c9, rgb[1] / 4);
    io_out8(0x03c9, rgb[2] / 4);

    rgb += 3;
  }

  io_store_eflags(eflags);
  return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0,
              int x1, int y1) {
  int x, y;
  for (y = y0; y <= y1; y++) {
    for (x = x0; x <= x1; x++) {
      vram[xsize * y + x] = c;
    }
  }

  return;
}

void putfont8(char *vram, int xsize, int x, int y, char color, char *font) {
  char *p, d;

  int i;
  for (i = 0; i < 16; i++) {
    p = vram + (y + i) * xsize + x;
    d = font[i];

    if ((d & 0x80) != 0) {
      p[0] = color;
    }
    if ((d & 0x40) != 0) {
      p[1] = color;
    }
    if ((d & 0x20) != 0) {
      p[2] = color;
    }
    if ((d & 0x10) != 0) {
      p[3] = color;
    }
    if ((d & 0x08) != 0) {
      p[4] = color;
    }
    if ((d & 0x04) != 0) {
      p[5] = color;
    }
    if ((d & 0x02) != 0) {
      p[6] = color;
    }
    if ((d & 0x01) != 0) {
      p[7] = color;
    }
  }

  return;
}

void putfonts8_asc(char *vram, int xsize, int x, int y, char color,
                   unsigned char *str) {
  extern char hankaku[4096];

  int i;
  for (i = 0; *str != '\0'; i++, str++) {
    putfont8(vram, xsize, x + 8 * i, y, color, hankaku + *str * 16);
  }

  return;
}

void init_palette(void) {
  static unsigned char table_rgb[16 * 3] = {
      0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00,
      0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xc6, 0xc6, 0xc6, 0x84, 0x00, 0x00, 0x00, 0x84, 0x00, 0x84, 0x84, 0x00,
      0x00, 0x00, 0x84, 0x84, 0x00, 0x84, 0x00, 0x84, 0x84, 0x84, 0x84, 0x84};

  set_palette(0, 15, table_rgb);

  return;
}

void init_mouse_cursor8(char *mouse, char bc) {
  static char cursor[16][16] = {
      "**************..", "*OOOOOOOOOOO*...", "*OOOOOOOOOO*....",
      "*OOOOOOOOO*.....", "*OOOOOOOO*......", "*OOOOOOO*.......",
      "*OOOOOOO*.......", "*OOOOOOOO*......", "*OOOO**OOO*.....",
      "*OOO*..*OOO*....", "*OO*....*OOO*...", "*O*......*OOO*..",
      "**........*OOO*.", "*..........*OOO*", "............*OO*",
      ".............***"};
  int x, y;

  for (y = 0; y < 16; y++) {
    for (x = 0; x < 16; x++) {
      if (cursor[y][x] == '*') {
        mouse[y * 16 + x] = COL8_000000;
      }
      if (cursor[y][x] == 'O') {
        mouse[y * 16 + x] = COL8_FFFFFF;
      }
      if (cursor[y][x] == '.') {
        mouse[y * 16 + x] = bc;
      }
    }
  }
  return;
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title) {
  static char closebtn[14][16] = {
      "OOOOOOOOOOOOOOO@", "OQQQQQQQQQQQQQ$@", "OQQQQQQQQQQQQQ$@",
      "OQQQ@@QQQQ@@QQ$@", "OQQQQ@@QQ@@QQQ$@", "OQQQQQ@@@@QQQQ$@",
      "OQQQQQQ@@QQQQQ$@", "OQQQQQ@@@@QQQQ$@", "OQQQQ@@QQ@@QQQ$@",
      "OQQQ@@QQQQ@@QQ$@", "OQQQQQQQQQQQQQ$@", "OQQQQQQQQQQQQQ$@",
      "O$$$$$$$$$$$$$$@", "@@@@@@@@@@@@@@@@"};
  int x, y;
  char c;
  boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
  boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize - 2, 1);
  boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
  boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize - 2);
  boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
  boxfill8(buf, xsize, COL8_000000, xsize - 1, 0, xsize - 1, ysize - 1);
  boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
  boxfill8(buf, xsize, COL8_000084, 3, 3, xsize - 4, 20);
  boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
  boxfill8(buf, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);
  putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
  for (y = 0; y < 14; y++) {
    for (x = 0; x < 16; x++) {
      c = closebtn[y][x];
      if (c == '@') {
        c = COL8_000000;
      } else if (c == '$') {
        c = COL8_848484;
      } else if (c == 'Q') {
        c = COL8_C6C6C6;
      } else {
        c = COL8_FFFFFF;
      }
      buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
    }
  }
  return;
}

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int font_color,
                       int background_color, const char *s, int length) {
  boxfill8(sht->buf, sht->bxsize, background_color, x, y, x + 8 * length,
           y + 15);
  putfonts8_asc(sht->buf, sht->bxsize, x, y, font_color, s);
  sheet_refresh(sht, x, y, x + 8 * length, y + 16);
  return;
}
