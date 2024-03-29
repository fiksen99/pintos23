#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "userprog/process.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include <string.h>
#include "syscall.h"
#include "threads/synch.h"
#include "threads/malloc.h"

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to task 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  uint32_t esp = (uint32_t) f->esp;

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

  /* Only user code should be causing page faults now */
  if (!user)
    PANIC ("non user code causing page fault");

  if (!not_present)
    thread_exit();

  handle_page_fault (fault_addr, esp);
}

void
handle_page_fault (void *fault_addr, uint32_t esp)
{
  struct thread *curr = thread_current();
  void *fault_page = pg_round_down (fault_addr);
  struct page *supp_page = page_lookup (&curr->supp_page_table, fault_page);
  if (supp_page == NULL)
  {
    // only adds 1 page per fault, possibly inefficient if many pages need to be added
    if (fault_addr < PHYS_BASE && (uint32_t) fault_addr >= esp - 32 && curr->stack_size < STACK_MAX_SIZE)
    {
      struct page *new_stack_page = malloc (sizeof (struct page));
      new_stack_page->addr = fault_page;
      new_stack_page->writable = true;
      new_stack_page->page_location = PG_ZERO;
      hash_insert (&curr->supp_page_table, &new_stack_page->elem);
      curr->stack_size++;
      return;
    }
    // page doesnt exist
    /* TODO: modify process_exit to free all new resources */
    thread_exit ();
  }
  else
  {
    //page exists, needs to be loaded into frame table
    bool writable = supp_page->writable;
    if (supp_page->page_location == PG_ZERO)
    {
      frame_get_page (PAL_USER | PAL_ZERO, fault_page, writable);
    }
    else if (supp_page->page_location == PG_SWAP)
    {
      void *kpage = frame_get_page (PAL_USER, fault_page, writable);
      swap_read (supp_page->index, kpage);
      supp_page->index = -1;
    }
    else if (supp_page->page_location == PG_DISK)
    {
      // TODO: (eviction) When reading from the file, need to store the file it was loaded from 
      // because if it ever needs to go back into a file then it should use the old  
      void *kpage = frame_get_page (PAL_USER, fault_page, writable);
      lock_acquire (&file_lock);
      off_t read_bytes = file_read_at (supp_page->file, kpage, PGSIZE, supp_page->offset);
      lock_release (&file_lock);
      off_t zeroes = PGSIZE - read_bytes;
      memset(kpage + read_bytes, 0, zeroes);
    }
    else if (supp_page->page_location == PG_MEM)
    {
      // possibly reached if two threads page fault on the same page, one has already
      // updated page location but not added to pagedir yet.
    }
    supp_page->page_location = PG_MEM;
    return;
  }
}
