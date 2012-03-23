#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include <stdlib.h>
#include "random.h"
#include "userprog/pagedir.h"

struct hash frame_table;
//static struct frame *choose_frame_for_eviction ();
//static void perform_eviction ();
static struct frame * get_frame (void *, void *);
static struct frame * lookup_frame (void *);
static struct frame * choose_frame_for_eviction (void);

void
frame_init ()
{
  hash_init (&frame_table, frame_hash_bytes, frame_hash_less, NULL);
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
  return hash_bytes (frame->upage, sizeof(void *));
}

void *
frame_get_page (enum palloc_flags flags, void *upage, bool writable)
{
  void *kpage;
  while ((kpage = palloc_get_page (flags)) == NULL)
  {
    //no free frames - evict
    struct frame *frame = choose_frame_for_eviction ();
    frame_free_page (frame->upage);
  }
  get_frame (upage);
  pagedir_set_page (thread_current()->pd, upage, kpage, writable);
}

void 
frame_free_page (void *upage)
{
  struct pagedir *pd = thread_current ()->pagedir;
  void *kpage = pagedir_get_page (pd, upage);
  pagedir_clear_page (pd, upage);
  palloc_free_page (kpage);
  struct frame *f = lookup_frame (upage);
  hash_delete (&frame_table, &f->elem);
  free (f);
}

static struct frame *
lookup_frame (void *upage)
{
  struct frame frame;
  struct hash_elem *e;

  frame.upage = upage;
  e = hash_find (&frame_table, &frame.elem);
  return e != NULL ? hash_entry (e, struct frame, elem) : NULL;
}

static struct frame *
get_frame (void *upage)
{
  struct frame *frame = malloc (sizeof (struct frame));
  frame->owner_tid = thread_current ()->tid;
  frame->upage = upage;
  hash_insert (&frame_table, &frame->elem);
  return frame;
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
    //is page to evict
    if (i == rand)
    {
      return hash_entry (hash_cur (&i), struct frame, elem);
    }
  }
}
