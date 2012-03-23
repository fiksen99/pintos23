#ifndef SWAP_H
#define SWAP_H

#include "devices/block.h"
#include "kernel/bitmap.h"
#include "vm/frame.h"

#define NUM_SECTORS (PGSIZE / BLOCK_SECTOR_SIZE)

struct swap_slot
{
  struct frame *frame;
  //other info needed? i think we need to keep the index in the bitmap here, and store everything in a list maybe?

};

void swap_init (void);
void swap_destroy (void);
void swap_write (void);
struct swap_slot *swap_set_frame (struct frame *frame);
void swap_read (void *addr, struct swap_slot *slot);

#endif
