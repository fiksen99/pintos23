
#ifndef FRAME_H
#define FRAME_H

#include <hash.h>

struct frame
{
  struct page *addr;
  tid_t owner_tid;
  struct hash_elem elem;
}

extern struct hash frame_table;

void frame_init ();

bool frame_hash_less (const struct hash_elem *a, const struct hash_elem *b,
                      void *aux);

#endif
