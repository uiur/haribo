#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram,
                           int xsize, int ysize) {
  struct SHTCTL *ctl;
  int i;

  ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
  if (ctl == 0) {
    return ctl;
  }

  ctl->map = (unsigned char *)memman_alloc_4k(memman, xsize * ysize);
  if (ctl->map == 0) {
    ctl->map =
        (unsigned char *)memman_free_4k(memman, ctl, sizeof(struct SHTCTL));

    return ctl;
  }

  ctl->vram = vram;
  ctl->xsize = xsize;
  ctl->ysize = ysize;
  ctl->top = -1;

  for (i = 0; i < sizeof(ctl->sheets0); i++) {
    ctl->sheets0[i].flags = 0;
  }

  return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl) {
  struct SHEET *sht;
  int i;
  for (i = 0; i < sizeof(ctl->sheets0); i++) {
    if (ctl->sheets0[i].flags == 0) {
      sht = &ctl->sheets0[i];
      sht->flags = 1;
      sht->height = -1;
      sht->ctl = ctl;

      return sht;
    }
  }

  return 0;
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize,
                  int col_inv) {
  sht->buf = buf;
  sht->bxsize = xsize;
  sht->bysize = ysize;
  sht->col_inv = col_inv;
  return;
}

void sheet_updown(struct SHEET *sht, int height) {
  struct SHTCTL *ctl = sht->ctl;
  int h, old = sht->height;

  if (height > ctl->top + 1) {
    height = ctl->top + 1;
  }

  if (height < -1) {
    height = -1;
  }

  sht->height = height;

  if (old > height) {
    if (height >= 0) {
      for (h = old; h > height; h--) {
        ctl->sheets[h] = ctl->sheets[h - 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
      sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize,
                       sht->vy0 + sht->bysize, height + 1);
      sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize,
                       sht->vy0 + sht->bysize, height + 1, old);
    } else {
      if (ctl->top > old) {
        for (h = old; h < ctl->top; h++) {
          ctl->sheets[h] = ctl->sheets[h + 1];
          ctl->sheets[h]->height = h;
        }
      }
      ctl->top--;
      sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize,
                       sht->vy0 + sht->bysize, 0);
      sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize,
                       sht->vy0 + sht->bysize, 0, old - 1);
    }
  } else if (old < height) {
    if (old >= 0) {
      for (h = old; h < height; h++) {
        ctl->sheets[h] = ctl->sheets[h + 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
    } else {
      for (h = ctl->top + 1; h > height; h--) {
        ctl->sheets[h] = ctl->sheets[h - 1];
        ctl->sheets[h]->height = h;
      }

      ctl->sheets[height] = sht;
      ctl->top++;
    }
    sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize,
                     sht->vy0 + sht->bysize, height);
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize,
                     sht->vy0 + sht->bysize, height, height);
  }
}

void sheet_refresh(struct SHEET *sheet, int bx0, int by0, int bx1, int by1) {
  if (sheet->height >= 0) {
    sheet_refreshsub(sheet->ctl, sheet->vx0 + bx0, sheet->vy0 + by0,
                     sheet->vx0 + bx1, sheet->vy0 + by1, sheet->height,
                     sheet->height);
  }

  return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int x0, int y0, int x1, int y1,
                      int h0, int h1) {
  int h, bx, by, bx0, by0, bx1, by1, sid;
  unsigned char c;
  struct SHEET *sheet;

  if (x1 > ctl->xsize) {
    x1 = ctl->xsize;
  }

  if (y1 > ctl->ysize) {
    y1 = ctl->ysize;
  }

  for (h = h0; h <= h1; h++) {
    sheet = ctl->sheets[h];

    bx0 = x0 - sheet->vx0;
    by0 = y0 - sheet->vy0;
    bx1 = x1 - sheet->vx0;
    by1 = y1 - sheet->vy0;
    sid = sheet - ctl->sheets0;

    if (bx0 < 0) {
      bx0 = 0;
    }

    if (by0 < 0) {
      by0 = 0;
    }

    if (bx1 > sheet->bxsize) {
      bx1 = sheet->bxsize;
    }

    if (by1 > sheet->bysize) {
      by1 = sheet->bysize;
    }

    for (by = by0; by < by1; by++) {
      for (bx = bx0; bx < bx1; bx++) {
        if (ctl->map[(by + sheet->vy0) * ctl->xsize + (bx + sheet->vx0)] ==
            sid) {
          ctl->vram[(by + sheet->vy0) * ctl->xsize + (bx + sheet->vx0)] =
              sheet->buf[by * sheet->bxsize + bx];
        }
      }
    }
  }

  return;
}

void sheet_slide(struct SHEET *sht, int vx0, int vy0) {
  struct SHTCTL *ctl = sht->ctl;

  int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
  sht->vx0 = vx0;
  sht->vy0 = vy0;

  if (sht->height >= 0) {
    sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize,
                     old_vy0 + sht->bysize, 0);
    sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize,
                     sht->vy0 + sht->bysize, sht->height);
    sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize,
                     old_vy0 + sht->bysize, 0, sht->height - 1);
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize,
                     sht->vy0 + sht->bysize, sht->height, sht->height);
  }

  return;
}

void sheet_free(struct SHEET *sht) {
  if (sht->height >= 0) {
    sheet_updown(sht, -1);
  }

  sht->flags = 0;

  return;
}

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1,
                      int h0) {
  int h, bx, by, vx, vy, bx0, by0, bx1, by1;
  unsigned char *buf, sid, *map = ctl->map;
  struct SHEET *sht;
  if (vx0 < 0) {
    vx0 = 0;
  }
  if (vy0 < 0) {
    vy0 = 0;
  }
  if (vx1 > ctl->xsize) {
    vx1 = ctl->xsize;
  }
  if (vy1 > ctl->ysize) {
    vy1 = ctl->ysize;
  }
  for (h = h0; h <= ctl->top; h++) {
    sht = ctl->sheets[h];
    sid = sht - ctl->sheets0;
    buf = sht->buf;
    bx0 = vx0 - sht->vx0;
    by0 = vy0 - sht->vy0;
    bx1 = vx1 - sht->vx0;
    by1 = vy1 - sht->vy0;
    if (bx0 < 0) {
      bx0 = 0;
    }
    if (by0 < 0) {
      by0 = 0;
    }
    if (bx1 > sht->bxsize) {
      bx1 = sht->bxsize;
    }
    if (by1 > sht->bysize) {
      by1 = sht->bysize;
    }
    for (by = by0; by < by1; by++) {
      vy = sht->vy0 + by;
      for (bx = bx0; bx < bx1; bx++) {
        vx = sht->vx0 + bx;
        if (buf[by * sht->bxsize + bx] != sht->col_inv) {
          map[vy * ctl->xsize + vx] = sid;
        }
      }
    }
  }
  return;
}
