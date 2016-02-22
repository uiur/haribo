#include "bootpack.h"

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
          man->free[i] = man->free[i + 1];
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
    struct FREEINFO *prev = &man->free[i - 1];
    if (prev->addr + prev->size == addr) {
      prev->size += size;

      if (i < man->frees) {
        struct FREEINFO *next = &man->free[i];
        if (addr + size == next->addr) {
          prev->size += next->size;

          man->frees--;
          for (; i < man->frees; i++) {
            man->free[i] = man->free[i + 1];
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
      man->free[j] = man->free[j - 1];
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
