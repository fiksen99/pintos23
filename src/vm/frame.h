
#ifndef FRAME_H
#define FRAME_H

#include <hash.h>
#include "threads/thread.h"
#include "vm/page.h"

#define DYNAMIC_MEM_CEIL 0x04000000
#define DYNAMIC_MEM_FLOOR 0x00100000

struct frame
{
  void *addr;
  tid_t owner_tid;
  struct hash_elem elem;
  bool used;
};

extern struct hash frame_table;

void frame_init (void);

bool frame_hash_less (const struct hash_elem *a, const struct hash_elem *b,
                      void *aux);

unsigned frame_hash_bytes (const struct hash_elem *elem, void *aux);

#endif
