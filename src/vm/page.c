#include "vm/page.h"
#include <hash.h>
#include "threads/malloc.h"
#include "threads/palloc.h"

void
spt_init (struct hash *spt)
{
  hash_init (spt, spt_hash_bytes, spt_hash_less, NULL);
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
  return hash_bytes (&spt->addr, sizeof (void *));
}


void
spt_destroy (struct hash *spt)
{
  struct hash_iterator i;
  hash_first (&i, spt);
  while (hash_next (&i))
  {
    struct page *p = hash_entry (hash_cur (&i), struct page, elem);
    frame_free_page (p->addr);
  }
  free (spt);
}

struct page *
page_lookup (struct hash *spt, void *address)
{
  struct page p = { .addr = address };
  struct hash_elem *e;
  e = hash_find (spt, &p.elem);
  return e != NULL ? hash_entry (e, struct page, elem) : NULL;
}
