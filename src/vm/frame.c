#include "frame.h"

struct hash frame_table;

void
frame_init ()
{
  hash_init (&frame_table, hash_bytes, compare_hash_less, );
}

bool
frame_hash_less (const struct hash_elem *a, const struct hash_elem *b,
                 void *aux UNUSED);
{
  return hash_entry (a, struct frame, elem)->addr
         < hash_entry (b, struct frame, elem)->addr;
}
