#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/malloc.h"

struct hash frame_table;

void
frame_init ()
{
  hash_init (&frame_table, frame_hash_bytes, frame_hash_less, NULL);
}

bool
frame_hash_less (const struct hash_elem *a, const struct hash_elem *b,
                 void *aux UNUSED)
{
  return hash_entry (a, struct frame, elem)->addr
         < hash_entry (b, struct frame, elem)->addr;
}

unsigned
frame_hash_bytes (const struct hash_elem *elem, void *aux UNUSED)
{
  struct frame *frame = hash_entry (elem, struct frame, elem);
  return hash_bytes (frame->addr, sizeof(struct page*));
}

void *
frame_get_page (enum palloc_flags flags)
{
  void *page = palloc_get_page (flags);
  if (page == NULL)
    return NULL;
  struct frame *frame = malloc (sizeof (struct frame));
  frame -> addr = page;
  frame -> owner_tid = thread_current () -> tid;
  hash_insert( &frame_table, &frame -> elem );
  return page;
}
