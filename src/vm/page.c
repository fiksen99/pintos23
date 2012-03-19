#include "vm/page.h"
#include <hash.h>

struct hash spt_table;

void
spt_init ()
{
  hash_init (&spt_table, spt_hash_bytes, spt_hash_less, NULL);
}

bool
spt_hash_less (const struct hash_elem *a, const struct hash_elem *b,
               void *aux UNUSED)
{
  return hash_entry (a, struct page, elem)->addr
         < hash_entry (b, struct page, elem)->addr;
}

unsigned
spt_hash_bytes (const struct hash_elem *elem, void *aux UNUSED)
{
  struct page *spt = hash_entry (elem, struct page, elem);
  return hash_bytes (spt->addr, sizeof (void *));
}
