#ifndef SWAP_H
#define SWAP_H

#include "devices/block.h"
#include "kernel/bitmap.h"
#include "vm/frame.h"

#define NUM_SECTORS (PGSIZE / BLOCK_SECTOR_SIZE)

void swap_init (void);
void swap_destroy (void);
int swap_try_write (void *);
void swap_read (int, void *);

#endif
