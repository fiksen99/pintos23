#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include <stdlib.h>
#include "random.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "threads/vaddr.h"

struct hash frame_table;
//static struct frame *choose_frame_for_eviction ();
//static void perform_eviction ();
static struct frame * lookup_frame (void *);
static struct frame * choose_frame_for_eviction (void);

struct lock frame_table_lock;

void
frame_init ()
{
  hash_init (&frame_table, frame_hash_bytes, frame_hash_less, NULL);
  lock_init (&frame_table_lock);
}

bool
frame_hash_less (const struct hash_elem *a, const struct hash_elem *b,
                 void *aux UNUSED)
{
  return hash_entry (a, struct frame, elem)->upage
         < hash_entry (b, struct frame, elem)->upage;
}

unsigned
frame_hash_bytes (const struct hash_elem *elem, void *aux UNUSED)
{
  struct frame *frame = hash_entry (elem, struct frame, elem);
  return hash_bytes (&frame->upage, sizeof(void *));
}

void *
frame_get_page (enum palloc_flags flags, void *upage, bool writable)
{
  void *kpage;
  struct thread *curr = thread_current ();
  while ((kpage = palloc_get_page (flags)) == NULL)
  {
    //no free frames - evict
    struct frame *evicted_frame = choose_frame_for_eviction ();
    struct page *evicted_page = page_lookup (&curr->supp_page_table, evicted_frame->upage);
    void *kpage_to_evict = pagedir_get_page (curr->pagedir, evicted_frame->upage);
    while ((evicted_page->index = swap_try_write (kpage_to_evict)) == -1)
    {
      //move page out of swap to filesys
      //printf("NOT ENOUGH SPACE IN SWAP.DSK\n\n\n");
      
    }
    evicted_page->page_location = PG_SWAP;
    frame_free_page (evicted_frame->upage);
    
  }
  struct frame *frame = malloc (sizeof (struct frame));
  frame->owner = thread_current ();
  frame->upage = upage;
  lock_acquire (&frame_table_lock);
  hash_insert (&frame_table, &frame->elem);
  lock_release (&frame_table_lock);
  pagedir_set_page (thread_current()->pagedir, upage, kpage, writable);
  return kpage;
}

void 
frame_free_page (void *upage)
{
  uint32_t *pd = thread_current ()->pagedir;
  void *kpage = pagedir_get_page (pd, upage);
  pagedir_clear_page (pd, upage);
  palloc_free_page (kpage);
  struct frame *f = lookup_frame (upage);
  lock_acquire (&frame_table_lock);
  hash_delete (&frame_table, &f->elem);
  lock_release (&frame_table_lock);
  free (f);
}

void
frame_table_reclaim ()
{
  struct frame frame;
  struct hash_elem *e;

  frame.owner = thread_current ();
  lock_acquire (&frame_table_lock);

  while ((e = hash_find (&frame_table, &frame.elem)) != NULL)
  {
    struct frame *f = hash_entry (e, struct frame, elem);
    hash_delete (&frame_table, e);
    uint32_t *pd = thread_current ()->pagedir;
    void *kpage = pagedir_get_page (pd, f->upage);
    palloc_free_page (kpage);
    free (f);
  }
  lock_release (&frame_table_lock);
}

void
frame_table_destory ()
{
  struct frame frame;
  struct hash_elem *e;

  lock_acquire (&frame_table_lock);

  while ((e = hash_find (&frame_table, &frame.elem)) != NULL)
  {
    struct frame *f = hash_entry (e, struct frame, elem);
    hash_delete (&frame_table, e);
    uint32_t *pd = thread_current ()->pagedir;
    void *kpage = pagedir_get_page (pd, f->upage);
    palloc_free_page (kpage);
    free (f);
  }
  lock_release (&frame_table_lock);
}

static struct frame *
lookup_frame (void *upage)
{
  struct frame frame;
  struct hash_elem *e;

  frame.upage = upage;
  lock_acquire (&frame_table_lock);
  e = hash_find (&frame_table, &frame.elem);
  lock_release (&frame_table_lock);
  return e != NULL ? hash_entry (e, struct frame, elem) : NULL;
}

static struct frame *
choose_frame_for_eviction ()
{
  ASSERT (!hash_empty (&frame_table));
  size_t size = hash_size (&frame_table);
  unsigned rand = random_ulong () % size;
  struct hash_iterator it;
  hash_first (&it, &frame_table);
  unsigned i;
  for (i = 0; hash_next (&it); i++)
  {
    if (i == rand)
    {
      return hash_entry (hash_cur (&it), struct frame, elem);
    }
  }
  NOT_REACHED();
}
