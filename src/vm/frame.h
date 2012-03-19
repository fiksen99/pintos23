
#ifndef FRAME_H
#define FRAME_H

#include <hash.h>
#include "threads/thread.h"
#include "vm/page.h"
#include "threads/palloc.h"

struct frame
{
  void *addr;
  tid_t owner_tid;
  struct hash_elem elem;
};

extern struct hash frame_table;

void frame_init (void);

bool frame_hash_less (const struct hash_elem *a, const struct hash_elem *b,
                      void *aux);

unsigned frame_hash_bytes (const struct hash_elem *elem, void *aux);

void * frame_get_page (enum palloc_flags flags);

#endif