#ifndef PAGE_H
#define PAGE_H

#include <hash.h>
#include "vm/frame.h"

/* TODO
*  add struct page to struct thread
*  all the process.c stuff
*/

extern struct hash spt_table;

struct page
{
  void *addr;                 /* Virtual address */
  struct hash_elem elem;      /* Hash table element */
  struct frame *frame;        /* Frame the page is stored in */
};

void spt_init (void);
bool spt_hash_less (const struct hash_elem *a, const struct hash_elem *b, void *aux);
unsigned spt_hash_bytes (const struct hash_elem *e, void *aux);

#endif
