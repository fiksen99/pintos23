#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

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

};

struct zero_data
{

};

struct page
{
  void *addr;                 /* Virtual address */
  struct hash_elem hash_elem; /* Hash table element */

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

/* Returns a hash value for page p */
unsigned
page_hash (const struct hash_elem *, void *);

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *, const struct hash_elem *, void *);

#endif
