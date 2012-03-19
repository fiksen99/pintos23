#include "vm/frame.h"
#include "threads/palloc.h"
#include <stdint.h>

struct hash frame_table;

void
frame_init ()
{
  hash_init (&frame_table, frame_hash_bytes, frame_hash_less, NULL);

  /* This must be executed before any other calls to palloc_get_multiple,
     interrupts are disabled so this is ensured. */
  unsigned int frames = (DYNAMIC_MEMORY_CEIL - DYNAMIC_MEMORY_FLOOR) / PGSIZE;
  unsigned int structs = frames * sizeof (struct frame);
  unsigned int pages = structs / PGSIZE;
  void *jnvfi = palloc_get_multiple (PAL_NOFRAME, pages);
  ASSERT (jnvfi != NULL);
  // need to add jnvfi to frame table at some point
  struct frame *f;
  for (f = DYNAMIC_MEMORY_FLOOR; f < DYNAMIC_MEMORY_CEIL; f++)
  {
    f->used = false;
    hash_insert (&frame_table, &f->elem);
  }
  uint32_t cfhjd;
  
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
