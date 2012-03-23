#ifndef PAGE_H
#define PAGE_H

#include <hash.h>
#include "vm/frame.h"

/* TODO
*  add struct page to struct thread
*  all the process.c stuff
*/

struct page
{
  void *addr;                   /* Virtual address */
  struct hash_elem elem;        /* Hash table element */
  bool writable;                /* page is writable or not */

  enum                          /* location the page is currently stored */
  {
    PG_MEM,
    PG_SWAP,
    PG_DISK,
    PG_ZERO
  } page_location;

  struct file *file;  
  off_t offset;                 /* offset from beginning of file */

  int index;                    /* index in swap partition */

  //what data should be at address
  //what resources to free on process termination
};

void spt_init (struct hash *spt);
bool spt_hash_less (const struct hash_elem *a, const struct hash_elem *b, void *aux);
unsigned spt_hash_bytes (const struct hash_elem *e, void *aux);
void spt_destroy (struct hash *spt);
struct page *page_lookup (struct hash *, void *address);

#endif
