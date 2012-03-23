#include "vm/swap.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <debug.h>

static struct bitmap *swap_bitmap;
static struct block *block;
static struct lock swap_lock;

static int find_first_free_slot (void);
static bool is_swap_full (void);

/* Initialises the swap bitmap */
void
swap_init ()
{
  block = block_get_role (BLOCK_SWAP);
  int swap_pages = block_size (block) / NUM_SECTORS;
  swap_bitmap = bitmap_create (swap_pages);
  if (swap_bitmap == NULL)
    PANIC ("not enough memory to create swap bitmap.");

  lock_init (&swap_lock);
}

/* Destroys the swap bitmap */
void
swap_destroy ()
{
  lock_acquire (&swap_lock);
  bitmap_destroy (swap_bitmap);
  lock_release (&swap_lock);
}

/* Attempts to write the given frame to a free swap slot, returns -1 if swap is
   full, swap slot otherwise */
int
swap_try_write (struct frame *frame)
{
  lock_acquire (&swap_lock);
  int index;
  if (bitmap_all (swap_bitmap, 0, bitmap_size (swap_bitmap)))
  {
    index = -1;
  }
  else
  {
    index = bitmap_scan_and_flip (swap_bitmap, 0, 1, false);

    int i;
    for (i = 0; i < NUM_SECTORS; i++)
      block_write (block, (index * NUM_SECTORS) + i, frame->addr
                                                     + (i * BLOCK_SECTOR_SIZE));
  }
  lock_release (&swap_lock);
  return index;
}

/* Reads the data from the given swap slot into the given frame */
void
swap_read (int index, struct frame *frame)
{
  lock_acquire (&swap_lock);

  int i;
  for (i = 0; i < NUM_SECTORS; i++)
    block_read (block, (index * NUM_SECTORS) + i, frame->addr
                                                  + (i * BLOCK_SECTOR_SIZE));

  bitmap_set_multiple (swap_bitmap, index, 1, false);
  lock_release (&swap_lock);
}
