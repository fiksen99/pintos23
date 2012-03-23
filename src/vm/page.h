#ifndef PAGE_H
#define PAGE_H

#include <hash.h>
#include "vm/frame.h"

/* TODO
*  add struct page to struct thread
*  all the process.c stuff
*/


struct mem_data
{  
  struct frame *frame;        /* Frame the page is stored in */
};

struct swap_data
{

};

struct disk_data
{
  struct file *file;
  off_t offset;                 /* offset from beginning of file */
};

struct zero_data
{

};

struct page
{
  void *addr;           /* Virtual address */
  struct hash_elem elem;      /* Hash table element */
  bool writable;                /* page is writable or not */

  enum
  {
    PG_MEM,
    PG_SWAP,
    PG_DISK,
    PG_ZERO
  } page_location;

  union
  {
    struct mem_data mem;
    struct swap_data swap;
    struct disk_data disk;
    struct zero_data zero;
  } data;

  //what data should be at address
  //what resources to free on process termination
};

void spt_init (struct hash *spt);
bool spt_hash_less (const struct hash_elem *a, const struct hash_elem *b, void *aux);
unsigned spt_hash_bytes (const struct hash_elem *e, void *aux);
void spt_destroy (struct hash *spt);
struct page * page_lookup (struct hash *, const void *address);

#endif
