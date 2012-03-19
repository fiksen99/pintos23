#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

/* TODO
*  add struct page to struct thread
*  all the process.c stuff
*/

struct page
{
  void *addr;                 /* Virtual address */
  struct hash_elem hash_elem; /* Hash table element */
  struct frame *frame;        /* Frame the page is stored in */
};

/* Returns a hash value for page p */
unsigned page_hash (const struct hash_elem *p_, void *aux);

/* Returns true if page a precedes page b. */
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux);

#endif
