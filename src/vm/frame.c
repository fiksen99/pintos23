#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include <stdlib.h>

struct hash frame_table;
//static struct frame *choose_frame_for_eviction ();
//static void perform_eviction ();
static void evict_page (void);

void
frame_init ()
{
  hash_init (&frame_table, frame_hash_bytes, frame_hash_less, NULL);
}

bool
frame_hash_less (const struct hash_elem *a, const struct hash_elem *b,
                 void *aux UNUSED)
{
  return hash_entry (a, struct frame, elem)->addr
         < hash_entry (b, struct frame, elem)->addr;
}

unsigned
frame_hash_bytes (const struct hash_elem *elem, void *aux UNUSED)
{
  struct frame *frame = hash_entry (elem, struct frame, elem);
  return hash_bytes (frame->addr, sizeof(void *));
}

void *
frame_get_page (enum palloc_flags flags)
{
  void *kpage;
  while ((kpage = palloc_get_page (flags)) == NULL)
  {
    //no free frames - evict
    evict_page();
  }
  get_frame (kpage);
  return kpage;
}

void 
frame_free_page (void *page)
{
  printf ("page to free: %p\n", page);
  palloc_free_page (page);
  struct frame *f = lookup_frame (page);
  hash_delete (&frame_table, &f->elem);
  free (f);
}

struct frame *
lookup_frame (void *addr)
{
  struct frame frame;
  struct hash_elem *e;

  frame.addr = addr;
  e = hash_find (&frame_table, &frame.elem);
  return e != NULL ? hash_entry (e, struct frame, elem) : NULL;
}

//TODO: currently assumes always free frame.
struct frame *
get_frame (void *kpage)
{
  struct frame *frame = malloc (sizeof (struct frame));
  frame->owner_tid = thread_current ()->tid;
  frame->addr = kpage;
  hash_insert (&frame_table, &frame->elem);

  return frame;
}

static void
evict_page ()
{
  //void *kpage = choose_frame()
}

//TODO currently chooses randomly. change so follows our algorithm.
/*static struct frame *
choose_frame_for_eviction ()
{
  ASSERT (!hash_empty (&frame_table));
  struct hash_iterator it;
  return hash_entry (hash_cur (&it), struct frame, elem);
   ------------------
  hash_first (&it, &frame_table);
  while (hash_next (&it))
  {
    
  }
   ------------------ 
//}

 performs eviction 
static void
perform_eviction ()
{
  
}*/
