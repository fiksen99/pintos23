#include "vm/frame.h"

struct hash frame_table;

void
frame_init ()
{
  hash_init (&frame_table, hash_bytes, frame_hash_less, NULL);
}

bool
frame_hash_less (const struct hash_elem *a, const struct hash_elem *b,
                 void *aux UNUSED);
{
  return hash_entry (a, struct frame, elem)->addr
         < hash_entry (b, struct frame, elem)->addr;
}

void
add_to_frame_table()
{
  struct frame *frame = malloc (sizeof(struct frame));
  frame->owner_tid = thread_current()->tid;
  hash_insert (&frame, &frame->elem);
}
