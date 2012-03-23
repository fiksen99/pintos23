#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "userprog/exception.h"

static void syscall_handler (struct intr_frame *);
static void execute_halt (void);
static void execute_exit (struct intr_frame *, int);
static uint32_t execute_exec (const char *);
static uint32_t execute_wait (tid_t);
static uint32_t execute_write (int, const void *, unsigned);
static uint32_t execute_create (const char *, unsigned);
static uint32_t execute_remove (const char *);
static uint32_t execute_open (const char *);
static uint32_t execute_filesize (int);
static uint32_t execute_read (int, void *, unsigned) UNUSED;
static uint32_t execute_seek (int, unsigned);
static uint32_t execute_tell (int);
static uint32_t execute_close (int);
static struct list_elem *execute_munmap (mapid_t);
static mapid_t execute_mmap (int, void *);
static struct file *find_file_from_fd (int);
struct mapid_elems *find_struct_from_mapid (mapid_t);
static void check_valid_access (const uint32_t addr, uint32_t esp);
static void check_valid_string (const uint32_t addr, uint32_t esp);
static void check_valid_buffer (const uint32_t addr, uint32_t esp, const unsigned int size);

static struct list fd_list;
static int new_fd = 2; //fd to be allocated.

struct list mapid_list;
static mapid_t new_mapid = 0;

struct lock file_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init (&fd_list);
  list_init (&mapid_list);
  lock_init (&file_lock);
}

static void
syscall_handler (struct intr_frame *f)
{
  uint32_t *stackptr = f->esp;
  check_valid_access ((uint32_t) stackptr, (uint32_t) stackptr);
  int syscall = *stackptr;
  uint32_t result; // return value stored in eax
  if (syscall == SYS_HALT) /* ZERO arguments */
  {
    execute_halt ();
    NOT_REACHED ();
  }
  else if (syscall == SYS_EXIT || syscall == SYS_EXEC || syscall == SYS_WAIT
           || syscall == SYS_REMOVE || syscall == SYS_OPEN
           || syscall == SYS_FILESIZE || syscall == SYS_TELL
           || syscall == SYS_CLOSE || syscall == SYS_MUNMAP) /* ONE argument */
  {
    uint32_t *arg = stackptr + 1;
    if (syscall == SYS_EXIT)
    {
      execute_exit (f, (int) *arg);
      NOT_REACHED ();
    }
    else if (syscall == SYS_EXEC)
    {
	    check_valid_string (*arg, (uint32_t) stackptr);
      result = execute_exec ((char *) *arg);
    }
    else if (syscall == SYS_WAIT)
      result = execute_wait ((tid_t) *arg);
    else if (syscall == SYS_REMOVE)
    {
	    check_valid_string (*arg, (uint32_t) stackptr);
      result = execute_remove ((char *) *arg);
    }
    else if (syscall == SYS_OPEN)
    {
	    check_valid_string (*arg, (uint32_t) stackptr);
      result = execute_open ((char *) *arg);
    }
    else if (syscall == SYS_FILESIZE)
      result = execute_filesize ((int) *arg);
    else if (syscall == SYS_TELL)
      result = execute_tell((int) *arg);
    else if (syscall == SYS_CLOSE)
      result = execute_close ((int) *arg);
    else
    {
//      printf("execute_munmap\n");
      result = (execute_munmap ((mapid_t) *arg) == NULL) ? -1 : 1;
    }
  }
  else if (syscall == SYS_CREATE || syscall == SYS_SEEK || syscall == SYS_MMAP) /* TWO arguments */
  {
    uint32_t *arg1 = stackptr + 1;
    unsigned *arg2 = stackptr + 2;
    if (syscall == SYS_CREATE)
    {
	    check_valid_string (*arg1, (uint32_t) stackptr);
      result = execute_create ((char *) *arg1, *arg2);
    }
    else if (syscall == SYS_SEEK)
      result = execute_seek ((int) *arg1, *arg2);
    else
    {
      if (!is_user_vaddr((void*)*arg2))
        thread_exit();
      result = execute_mmap ((int) *arg1, (void *) *arg2);
    }
  }
  else if ( syscall == SYS_READ || syscall == SYS_WRITE ) /* THREE arguments */
  {
    int fd = *(stackptr + 1);
    void *buffer = (void*) *(stackptr + 2);
    unsigned size = *(stackptr + 3);
	  check_valid_buffer ((uint32_t) buffer, (uint32_t) stackptr, size);
    if (syscall == SYS_WRITE)
      result = execute_write (fd, buffer, size);
    else
      result = execute_read (fd, buffer, size);
  }
  else
    thread_exit ();
  f->eax = result;
}

/* Executes the halt system call. Terminates pintos by calling shutdown_power_off () */
static void
execute_halt (void)
{
  shutdown_power_off ();
}

/* Terminates the current user program, and sets eax of the intr_frame as the
status of the kernel. A status of 0 indicates success, and nonzero values indicate
errors */
static void
execute_exit (struct intr_frame *f, int arg)
{
  struct thread *current = thread_current();
  struct list parents_children = current->parent->children;
  struct list_elem *e;
  struct child_status *s = NULL;
  for (e = list_begin (&parents_children); e != list_end (&parents_children);
       e = list_next (e))
  {
    s = list_entry (e, struct child_status, elem);
	  if ( s->tid == current->tid ) break;
  }
  if (arg < 0)
  {
    arg = -1;
  }
  s->status = arg;
  f->eax = (uint32_t) arg;
  thread_exit();
}

static uint32_t
execute_exec (const char * file_name)
{
  struct thread * curr = thread_current();
  curr->load_fail = false;
  tid_t id = process_execute (file_name);
  sema_down(&curr->exec_sema);
  if (curr->load_fail) return (uint32_t) -1;
  return (uint32_t) id;
}

static uint32_t
execute_wait (tid_t tid)
{
  tid_t id = process_wait( tid );
  return (uint32_t)id;
}

static uint32_t
execute_write (int fd, const void *buffer, unsigned size)
{
  if (fd == STDOUT_FILENO )     /*write to console*/
  {
    putbuf( buffer, size );
    return (uint32_t) size;
  }
  struct file *file = find_file_from_fd (fd);
  if (file == NULL)
    return 0;
  lock_acquire (&file_lock);
  uint32_t bytes = (uint32_t) file_write (file, buffer, size);
  lock_release (&file_lock);
  return bytes; 
}

static uint32_t
execute_create (const char *file, unsigned initial_size)
{
  lock_acquire (&file_lock);
  if (!filesys_create (file, initial_size))
  {
    // Failed to create file
    lock_release (&file_lock);
    return (uint32_t) false;
  }
  struct file *file_opened = filesys_open (file);
  if (file_opened == NULL)
  {
    // Failed to open file
    lock_release (&file_lock);
    return (uint32_t) false;
  }
  void *zeroes = palloc_get_page (PAL_ZERO);
  if (zeroes == NULL)
  {
    // Failed to allocate page of zeroes to write to file
    file_close (file_opened);
    lock_release (&file_lock);
    return (uint32_t) false;
  }
  uint32_t bytes_written;
  uint32_t i;
  for (i = 0; i < initial_size / PGSIZE; i++)
  {
    bytes_written = file_write (file_opened, zeroes, PGSIZE);
    if (bytes_written < PGSIZE)
    {
      // Failed to write some zeroes
      file_close (file_opened);
      palloc_free_page (zeroes);
      lock_release (&file_lock);
      return (uint32_t) false;
    }
  }
  bytes_written = file_write (file_opened, zeroes, initial_size % PGSIZE);
  file_close (file_opened);
  palloc_free_page (zeroes);
  lock_release (&file_lock);
  return (uint32_t) (bytes_written == initial_size % PGSIZE);
}

static uint32_t
execute_remove (const char *file)
{
  lock_acquire (&file_lock);
  uint32_t ret = (uint32_t) filesys_remove (file);
  lock_release (&file_lock);
  return ret;
}

static uint32_t
execute_open (const char *file)
{
  if (file == NULL)
  {
    return (uint32_t) -1;
  }
  lock_acquire (&file_lock);
  struct file *file_opened = filesys_open (file);
  lock_release (&file_lock);
  if (file_opened == NULL) // Returns -1 if file could not be opened
  {
    return (uint32_t) -1;
  }

  struct fd_elems *new_file;
  new_file = (struct fd_elems *) malloc (sizeof (struct fd_elems));
  new_file->fd = new_fd++;
  new_file->file = file_opened;
  new_file->tid = thread_current ()->tid;
  list_push_back (&fd_list, &new_file->elem);
  return (uint32_t) new_file->fd;
}

/* Stores the size of the file opened in bytes into eax. If file does not exist,
   stores 0 in eax. */
static uint32_t
execute_filesize (int fd)
{
  struct file *file = find_file_from_fd (fd);
  if (file == NULL)
    return 0;

  lock_acquire (&file_lock);
  uint32_t bytes = (uint32_t) file_length (file);
  lock_release (&file_lock);

  return bytes;
}

static uint32_t
execute_read (int fd, void *buffer, unsigned size)
{
  if (fd == STDIN_FILENO) //reads input from the keyboard
  {
    int i;
    for (i = 0; i < (int) size; i++)
      *(uint8_t *) (buffer + i) = input_getc ();
    return (uint32_t) i;
  }
  struct file *file = find_file_from_fd (fd);
  if (file == NULL)
  {
    return (uint32_t) -1;
  }
  lock_acquire (&file_lock);
  uint32_t bytes = (uint32_t) file_read (file, buffer, size);
  lock_release (&file_lock);

  return bytes;
}

/* Changes the next byte to be read or written in the open file fd to position */
static uint32_t
execute_seek (int fd, unsigned position)
{
  struct file *file = find_file_from_fd (fd);
  if (file != NULL)
  {
    lock_acquire (&file_lock);
    file_seek (file, position);
    lock_release (&file_lock);  
  }
  return 0; //dummy value
}

/* eax = the position of the next byte to be read or written in the open file fd
   expressed in bytes from the beginning of the file */
static uint32_t
execute_tell (int fd)
{
  struct file *file = find_file_from_fd (fd);
  if (file == NULL)
    return 0;
  
  lock_acquire (&file_lock);
  uint32_t position = (uint32_t) file_tell (file);
  lock_release (&file_lock);

  return position;
}

static uint32_t
execute_close (int fd)
{
  if (fd == 0 || fd == 1)
    return -1;
  struct list_elem *e;
  for(e = list_begin (&fd_list); e != list_end (&fd_list); e = list_next (e))
  {
    struct fd_elems *fd_elem = list_entry(e, struct fd_elems, elem);
    if (fd_elem->fd == fd)
    {
      if (fd_elem->tid != thread_current ()->tid)
      {
        return -1;
      }
      lock_acquire (&file_lock);
      file_close (fd_elem->file);
      lock_release (&file_lock);      
      list_remove (e);
      free (fd_elem);
      return 0; //dummy value
    }
  }
  return -1;
}

/* Maps the file open as fd into the process's virtual address space. The entire
   file is mapped into consecutive virtual pages starting at addr.
   If successful, this function returns a mapping ID that uniquely identifies 
   the mapping within the process. */
static mapid_t 
execute_mmap (int fd, void *addr)
{
  if (fd == STDOUT_FILENO || fd == STDIN_FILENO || addr == 0 
      || (int) addr % PGSIZE != 0)
  {
    return MAP_FAILED;
  }

  struct file *oldfile = find_file_from_fd (fd);
  if (oldfile == NULL)
    return MAP_FAILED;

  lock_acquire (&file_lock);
  off_t filesize = file_length (oldfile);
  struct file *file = file_reopen (oldfile);
  lock_release (&file_lock);
  
  if (filesize == 0)
    return MAP_FAILED;

  struct thread *t = thread_current ();

  unsigned int pages = filesize / PGSIZE + (filesize % PGSIZE ? 1 : 0);
  unsigned int i;
  void *page_addr;
  for (i = 0, page_addr = addr; i < pages; i++, page_addr += PGSIZE)
  {
    if (page_lookup (&t->supp_page_table, page_addr) != NULL)
    {
      return MAP_FAILED;
    }
  }

  struct mapid_elems *new_entry = malloc (sizeof (struct mapid_elems));
  new_entry->mapid = new_mapid++;
  new_entry->file = file;
  new_entry->addr = addr;
  new_entry->tid = t->tid;
  list_push_back (&mapid_list, &new_entry->elem);

  off_t offset;
  for (i = 0, offset = 0; i < pages; i++, offset += PGSIZE)
  {
    struct page *p = malloc (sizeof (struct page));
    p->addr = addr + offset;
    p->page_location = PG_DISK;
    p->writable = true;
    p->file = file;
    p->offset = offset;
    hash_insert (&t->supp_page_table, &p->elem);
  }

  return new_entry->mapid;
}

/* Unmaps the mapping designated by 'mapping', which must be a mapping ID 
   returned by a previous call to mmap by the same process that has not yet been 
   unmapped.*/
static struct list_elem *
execute_munmap (mapid_t mapid)
{
  struct mapid_elems *mapid_elem = find_struct_from_mapid (mapid);
  if (mapid_elem == NULL)
  {
    return NULL;
  }

  struct file *f = mapid_elem->file;
  if (f == NULL)
    return NULL;
  // TODO: what if the file no longer exists

  void *addr = mapid_elem->addr;
  struct thread *t = thread_current ();

  struct list_elem *ret = list_remove (&mapid_elem->elem);
  free (mapid_elem);

  lock_acquire (&file_lock);

  off_t filesize = file_length (f);
  unsigned int pages = filesize / PGSIZE + (filesize % PGSIZE ? 1 : 0);
  unsigned int i;
  off_t offset;
  for (i = 0, offset = 0; i < pages; i++, offset += PGSIZE)
  {
    struct page *p = page_lookup (&t->supp_page_table, pg_round_down (addr + offset));
    ASSERT (p != NULL);
    if (p->page_location == PG_SWAP)
    {
      // TODO: read page from swap partition and write it back to the file
    }
    else if (p->page_location == PG_ZERO)
    {
      void *zero_page = frame_get_page (PAL_ZERO);
      file_write_at (f, addr + offset, PGSIZE, offset);
      frame_free_page (zero_page);
    }
    else if (p->page_location == PG_MEM)
    {
      if (pagedir_is_dirty (t->pagedir, addr + offset))
      {
        if (offset == (filesize / PGSIZE) * PGSIZE)
        {
          file_write_at (f, addr + offset, filesize - offset, offset);
        }
        else
        {
          file_write_at (f, addr + offset, PGSIZE, offset);
        }
      }
      // TODO: remove struct frame associated with this frame
      /*struct frame *frame = lookup_frame (addr + offset);
      hash_delete (&frame_table, &frame->elem);
      free (frame);*/
      pagedir_clear_page (t->pagedir, addr + offset);
    }
    hash_delete (&t->supp_page_table, &p->elem);
    free (p);
  }

  file_close (f);

  lock_release (&file_lock);

  return ret;
}

/* functions to help system call handling */

static struct file *
find_file_from_fd (int fd)
{
  struct list_elem *e;
  if (list_empty (&fd_list))
    return NULL;
  for (e = list_begin (&fd_list); e != list_end (&fd_list); e = list_next (e))
  {
    struct fd_elems *fd_elem = list_entry(e, struct fd_elems, elem);
    if (fd_elem->fd == fd)
      return fd_elem->file;
  }
  return NULL;
}

struct mapid_elems *
find_struct_from_mapid (mapid_t mapid)
{
  if (list_empty (&mapid_list))
  {
    return NULL;
  }

  struct list_elem *e;
  for (e = list_begin (&mapid_list); e != list_end (&mapid_list); e = list_next (e))
  {
    struct mapid_elems *mapid_elem = list_entry(e, struct mapid_elems, elem);
    if (mapid_elem->mapid == mapid)
    {
      return mapid_elem;
    }
  }
  return NULL;
}

/*checks user memory access is valid*/
static void
check_valid_access (const uint32_t addr, uint32_t stackptr)
{
  if (!is_user_vaddr((void*)addr)) 
  {
    thread_exit();
  }
  while (pagedir_get_page (thread_current()->pagedir, (void*) addr) == NULL)
  {
    handle_page_fault ((void *) addr, stackptr);
  }
}

/* Checks that a buffer is valid */
static void
check_valid_buffer (const uint32_t addr, uint32_t stackptr, const unsigned int size)
{
  void * check_addr;
  for (check_addr = pg_round_down ((void *) addr); check_addr < (void *) (addr + size); check_addr += PGSIZE)
  {
    check_valid_access ((uint32_t) check_addr, stackptr);
  }
}

/* Checks that a string passed by the user is valid */
static void
check_valid_string (const uint32_t addr, uint32_t stackptr)
{
  char *check_addr = (char *) addr;
  while (true)
  {
    check_valid_access ((uint32_t) check_addr, stackptr);
    if (*check_addr == '\0')
    {
      return;
    }
    check_addr++;
  }
}

void
close_thread_fds (void)
{
  tid_t tid = thread_current ()->tid;
  if (list_empty (&fd_list))
    return;
  struct list_elem *e = list_begin (&fd_list);
  while (e != list_end (&fd_list))
  {
    struct fd_elems *fd_elem = list_entry(e, struct fd_elems, elem);
    if (fd_elem->tid == tid)
    {
      lock_acquire (&file_lock);
      file_close (fd_elem->file);
      lock_release (&file_lock);
      e = list_remove (e);
      free (fd_elem);
    }
    else
    {
      e = list_next (e);
    }
  }
}

void close_thread_mapids ()
{
  tid_t tid = thread_current ()->tid;
  struct list_elem *e;
  for (e = list_begin (&mapid_list); e != list_end (&mapid_list);)
  {
    struct mapid_elems *mapid_elem = list_entry (e, struct mapid_elems, elem);
    //printf("mapid_elem being munmapped: %p\n", mapid_elem);
    if (mapid_elem->tid == tid)
    {
      e = execute_munmap (mapid_elem->mapid);
    }
    else
    {
      e = list_next (e);
    }
  }
}
