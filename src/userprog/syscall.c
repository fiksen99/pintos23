/*TODO:
* fix synchronisation of execute
* waiting on a process
* executing a process
* initialising a thread
* destroying a thread
* deallocate thread id at some point(see execute_exec)
* initialise list of child threads
*/

#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/process.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
static void execute_halt (void);
static void execute_exit (struct intr_frame *, int*);
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
static struct file * find_file_from_fd (int);
static void check_valid_access ( const uint32_t addr );

static struct list fd_list;
static int new_fd = 1; //fd to be allocated.

static struct lock file_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init (&fd_list);
  lock_init (&file_lock);
}

static void
syscall_handler (struct intr_frame *f)
{
  uint32_t *stackptr = f->esp;
  printf("%d",stackptr);
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
           || syscall == SYS_CLOSE) /* ONE argument */
  {
    uint32_t *arg = stackptr + 1;
    if (syscall == SYS_EXIT)
    {
      execute_exit (f, (int *) arg);
      NOT_REACHED ();
    }
    else if (syscall == SYS_EXEC)
    {
	    check_valid_access (*arg);
      result = execute_exec ((char *) *arg);
    }
    else if (syscall == SYS_WAIT)
    {
      result = execute_wait ((tid_t) *arg);
    }
    else if (syscall == SYS_REMOVE)
    {
	    check_valid_access (*arg);
      result = execute_remove ((char *) *arg);
    }
    else if (syscall == SYS_OPEN)
    {
	    check_valid_access (*arg);
      result = execute_open ((char *) *arg);
    }
    else if (syscall == SYS_FILESIZE)
    {
      result = execute_filesize ((int) *arg);
    }
    else if (syscall == SYS_TELL)
    {
      result = execute_tell((int) *arg);
    }
    else /* (syscall == SYS_CLOSE) */
    {
      result = execute_close ((int) *arg);
    }
  }
  else if (syscall == SYS_CREATE || syscall == SYS_SEEK) /* TWO arguments */
  {
    uint32_t *arg1 = stackptr + 1;
    unsigned *arg2 = stackptr + 2;
    if (syscall == SYS_CREATE)
    {
	    check_valid_access (*arg1);
      result = execute_create ((char *) *arg1, *arg2);
    }
    else
    {
      result = execute_seek ((int) *arg1, *arg2);
    }
  }
  else /* ( syscall == SYS_READ || syscall == SYS_WRITE ) */
       /* THREE arguments */
  {
    int fd = *(stackptr + 1);
    void *buffer = (void*) *(stackptr + 2);
	  check_valid_access ((uint32_t) buffer);
    unsigned size = *(stackptr + 3);
    if (syscall == SYS_WRITE)
    {
      result = execute_write (fd, buffer, size);
    }
    else
    {
      result = execute_read (fd, buffer, size);
    }
  }
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
execute_exit (struct intr_frame *f, int *arg)
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
  s->status = *arg;
  f->eax = (uint32_t)arg;
  thread_exit();
}

static uint32_t
execute_exec (const char * file_name)
{
  tid_t id = process_execute( file_name );
  return (uint32_t)id;
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
  if (fd == STDIN_FILENO )
    return 0;
  if (fd == STDOUT_FILENO )     /*write to console*/
  {
    putbuf( buffer, size );
    return size;
  }
  return 0;
}

static uint32_t
execute_create (const char *file, unsigned initial_size)
{
  lock_acquire (&file_lock);
  bool success = filesys_create (file, initial_size);
  lock_release (&file_lock);
  return (uint32_t) success;
}

static uint32_t
execute_remove (const char *file)
{
  return (uint32_t) filesys_remove (file);
}

static uint32_t
execute_open (const char *file)
{
  if (file == NULL)
  {
    return (uint32_t) -1;
  }
  struct file *file_opened = filesys_open (file);
  if (file_opened == NULL) // Returns -1 if file could not be opened
  {
    return (uint32_t) -1;
  }

  lock_acquire (&file_lock);
  struct fd_elems *new_file;
  new_file = (struct fd_elems *) malloc (sizeof (struct fd_elems));
  new_file->fd = ++new_fd;  
  new_file->file = file_opened;
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

  return (uint32_t) file_length (file);
}

static uint32_t
execute_read (int fd, void *buffer, unsigned size)
{
  if (fd == STDIN_FILENO) //reads input from the keyboard
  {
    int i;
    for (i = 0; i < (int) size; i++)
      *(uint8_t *) (buffer + i) = input_getc (); //EOF check?
    return (uint32_t) i;
  }
  struct file *file = find_file_from_fd (fd);
  if (file == NULL)
  {
    return (uint32_t) -1;
  }
  return (uint32_t) file_read (file, buffer, size);
}

/* Changes the next byte to be read or written in the open file fd to position */
static uint32_t
execute_seek (int fd, unsigned position)
{
  struct file *file;
  file = find_file_from_fd (fd);
  file_seek (file, position);
  return 0; //dummy value
}

/* eax = the position of the next byte to be read or written in the open file fd
   expressed in bytes from the beginning of the file */
static uint32_t
execute_tell (int fd)
{
  struct file *file;
  file = find_file_from_fd (fd);
  unsigned *tell = (unsigned *) malloc (sizeof (unsigned));
  *tell = (unsigned) file_tell (file);
  return (uint32_t) tell;
}

/*TODO remove ALL open file descriptors*/
static uint32_t
execute_close (int fd)
{
  struct list_elem *e;
  for(e = list_begin (&fd_list); e != list_back (&fd_list); e = list_next (e))
  {
    struct fd_elems *fd_elem = list_entry(e, struct fd_elems, elem);
    if (fd_elem->fd == fd)
    {
      file_close (fd_elem->file);      
      list_remove (e);
      free (fd_elem);
      break;
    }
  }
  lock_release (&file_lock);
  return 0; //dummy value
}

/* functions to correct help system call handling */

static struct file *
find_file_from_fd (int fd)
{
  struct list_elem *e;
  for(e = list_begin (&fd_list); e != list_back (&fd_list); e = list_next (e))
  {
    struct fd_elems *fd_elem = list_entry(e, struct fd_elems, elem);
    if (fd_elem->fd == fd)
      return fd_elem->file;
  }
  return NULL;
}


/*checks user memory access is valid*/
static void
check_valid_access (const uint32_t addr)
{
  if( !is_user_vaddr((void*)addr) ||
     pagedir_get_page( thread_current()->pagedir, (void*)addr ) == NULL ) 
  {
    thread_exit();
  }
}
