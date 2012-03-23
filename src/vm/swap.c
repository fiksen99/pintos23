//TODO need to keep track of pages in struct frame i think.

#include "vm/swap.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "kernel/debug.c"

struct bitmap *swap_bitmap;
struct block *block;
struct swap_slot *swap_slot;
struct lock swap_lock;

static bool is_swap_full (void);
static size_t find_first_free_slot (void);

/* Initialises the swap bitmap */
void
swap_init ()
{
  block = block_get_role (BLOCK_SWAP);
  size_t swap_pages = block_size (block) / NUM_SECTORS;
  swap_bitmap = bitmap_create (swap_pages);
  if (&swap_pages == NULL)
    PANIC ("not enough memory to create swap bitmap."); //kernel panic - not enough memory for bitmap

  lock_init (&swap_lock);
  //check if bitmap is created. error checking?
}

struct swap_slot *
swap_set_frame (struct frame *frame)
{
  swap_slot->frame = frame;
  return swap_slot;
}

/* Destroys the swap bitmap */
void
swap_destroy ()
{
  lock_acquire (&swap_lock);
  bitmap_destroy (swap_bitmap); //destroys the bitmap
  lock_release (&swap_lock);
}

void
swap_write ()
{
  size_t index = find_first_free_slot ();
  int i;
  // no locking required here
  for(i = 0; i < NUM_SECTORS; i++)
    block_write (block, index + i, swap_slot->frame);
}

static bool
is_swap_full ()
{
  lock_acquire (&swap_lock);
  //check if each bit in the bitmap is true or not.
  bool ret = bitmap_all (swap_bitmap, 0, bitmap_size (swap_bitmap));  //is this right?
  lock_release (&swap_lock);
  return ret;
}

static size_t
find_first_free_slot ()
{
  if (!is_swap_full ())
  {
    lock_acquire (&swap_lock);
    // implementation pretty much the same as in palloc.
    size_t index = bitmap_scan_and_flip (swap_bitmap, 0, NUM_SECTORS, false); //is this right?
    lock_release (&swap_lock);
    return index;
  }
  else
  {
    PANIC ("swap memory full"); //kernel panic - memory full.
  }
}

void
swap_read (void *addr, struct swap_slot *slot)
{
  int i;
  size_t swap_addr = find_first_free_slot (); 
  lock_acquire (&swap_lock);

  for (i = 0; i < NUM_SECTORS; i++)
    block_read (block,/*what do i put here?*/, addr + i * BLOCK_SECTOR_SIZE ); // not sure how to use this

  bitmap_set_multiple (swap_bitmap, slot->frame, NUM_SECTORS, false); //correct?
  lock_release (&swap_lock);
}
