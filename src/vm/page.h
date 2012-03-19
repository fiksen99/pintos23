#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

/* TODO
*  add struct page to struct thread
*  all the process.c stuff
*/

struct page
{
  struct hash_elem hash_elem; /* Has table element */
  void *addr;                 /* Virtual address */
  struct frame *frame;        /* Frame the page is stored in */
};

/* Returns a hash value for page p */
unsigned
page_hash (const struct hash_elem *, void *);

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *, const struct hash_elem *, void *);

#endif
