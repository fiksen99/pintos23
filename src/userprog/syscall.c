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
#include "process.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);
static void execute_halt (void);
static void execute_exit (struct intr_frame *, int *);
static void execute_exec (struct intr_frame *, const char **);
static void execute_wait (struct intr_frame *, tid_t);
static int execute_write (int, const void *, unsigned);
static void execute_create (struct intr_frame *, const char *, unsigned);
static void execute_remove (struct intr_frame *, const char *);
static void execute_open (struct intr_frame *, const char *);
static void execute_filesize (struct intr_frame *, int);
static void execute_read (struct intr_frame *, int, void *, unsigned) UNUSED;
static void execute_seek (int, unsigned);
static void execute_tell (struct intr_frame *, int);
static void execute_close (int);
static struct file * find_file_from_fd (int);

static struct list fd_list;
static int new_fd = 1; //fd to be allocated.

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init (&fd_list);
}

static void
syscall_handler (struct intr_frame *f) 
{
  uint32_t *stackptr = f->esp;
  int syscall = *stackptr;
  if( syscall == SYS_HALT )  /* ZERO arguments */
  { 
    execute_halt();
  } else if( syscall == SYS_EXIT || syscall == SYS_EXEC || syscall == SYS_WAIT ||
              syscall == SYS_REMOVE || syscall == SYS_OPEN || syscall == SYS_FILESIZE ||
              syscall == SYS_TELL || syscall == SYS_CLOSE )  /* ONE argument */
  {
    uint32_t *arg = stackptr + 1;
    if( syscall == SYS_EXIT ) {
      execute_exit( f, (int *)arg );
    } else if( syscall == SYS_EXEC )
    {
      execute_exec( f, (const char **)arg );
    } else if( syscall == SYS_WAIT )
    {
//    pid_t pid = (pid_t)*arg;
      execute_wait( f, (tid_t)*arg );
    } else if( syscall == SYS_REMOVE )
    {
      execute_remove(f, (char *)arg);
    } else if( syscall == SYS_OPEN )
    {
      execute_open (f, (char *) arg);
    } else if( syscall == SYS_FILESIZE )
    {
      execute_filesize (f, (int) arg);
    } else if( syscall == SYS_TELL )
    {
      execute_tell (f, (int) arg);
    } else if (syscall == SYS_CLOSE)
    {
      execute_close ((int) arg);
    }
  } else if( syscall == SYS_CREATE || syscall == SYS_SEEK )  /* TWO arguments */
  {
    void *arg1 = stackptr + 1;
    unsigned *arg2 = stackptr + 2;
    if (syscall == SYS_CREATE)
      execute_create (f, (char *)arg1, *arg2);
    if (syscall == SYS_SEEK)
      execute_seek ((int) arg1, (unsigned) arg2);
  } else if( syscall == SYS_READ || syscall == SYS_WRITE )  /* THREE arguments */
  {
    int fd = *(stackptr + 1);
    void *buffer = (void*)(stackptr + 2);
    unsigned size = *(stackptr + 3);
    if( syscall == SYS_WRITE ) {
      execute_write (fd, buffer, size);
    } else {
//      read();
    }
  }
/*  printf ("system call!\n");
  thread_exit ();*/
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
execute_exit (struct intr_frame * f, int *arg)
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
  printf("%s: exit(%d)\n", thread_name(), *arg);
  thread_exit();
}

static void
execute_exec (struct intr_frame *f, const char ** file_name)
{
  tid_t* id = malloc(sizeof(tid_t));
  *id = process_execute( *file_name );
  f->eax = (uint32_t)id;
}

static void
execute_wait ( struct intr_frame * f, tid_t tid )
{
  tid_t* id = malloc(sizeof(tid_t));
  *id = process_wait( tid );
  f->eax = (uint32_t)id;
}

static int
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

static void
execute_create (struct intr_frame *f, const char *file, unsigned initial_size)
{
  bool *success = malloc(sizeof (bool));
  *success = filesys_create (file, initial_size);
  f->eax = (uint32_t) success;
}

static void
execute_remove (struct intr_frame *f, const char *file)
{
  bool *success = malloc(sizeof (bool));
  *success = filesys_remove (file);
  f->eax = (uint32_t) success;
}

static void
execute_open (struct intr_frame *f, const char *file)
{
  struct file *file_opened;
  file_opened = filesys_open(file);
  int *open = malloc(sizeof (int));
  if(file_opened == NULL || file == NULL || !file_opened) // Returns -1 if file could not be opened
  {
    *open = -1;
    f->eax = (uint32_t) open;
  }

  struct fd_elems *new_file;
  new_file = (struct fd_elems *) malloc (sizeof (struct fd_elems));
  new_file->fd = new_fd++;  
  new_file->file = file_opened;
  *open = new_file->fd;
  f->eax = (uint32_t) open;
}

/* Stores the size of the file opened in bytes into eax. If file does not exist,
   stores 0 in eax. */
static void
execute_filesize (struct intr_frame *f, int fd)
{
  struct file *file;
  file = find_file_from_fd(fd);
  int *filesize = 0;
  if(file == NULL)
  {
    f->eax = (uint32_t) filesize;
    return;
  }
  *filesize = (int) file_length (file);
  f->eax = (uint32_t) filesize;
}

static void
execute_read (struct intr_frame *f, int fd, void *buffer, unsigned size)
{
  struct file *file;
  int i;
  int *bytes_read = malloc(sizeof (int));
  if (fd == STDIN_FILENO) //reads input from the keyboard
  {
    for (i = 0; i != (int) size; i++)
      *(uint8_t *) (buffer + 1) = input_getc ();
    *bytes_read = size;
    f->eax = (uint32_t) bytes_read;
    return;
  }
  if (fd == STDOUT_FILENO) // file cannot be read, so eax = -1
  {
    *bytes_read = -1;
    f->eax = (uint32_t) bytes_read;
    return;
  }
  file = find_file_from_fd (fd);
  if (file == NULL || !file)
  {
    *bytes_read = -1;
    f->eax = (uint32_t) bytes_read;
    return;
  }
  *bytes_read = file_read (file, buffer, size);
  f->eax = (uint32_t) bytes_read;
}

/* Changes the next byte to be read or written in the open file fd to position */
static void
execute_seek (int fd, unsigned position)
{
  struct file *file;
  file = find_file_from_fd (fd);
  file_seek (file, position);
}

/* eax = the position of the next byte to be read or written in the open file fd
   expressed in bytes from the beginning of the file */
static void
execute_tell (struct intr_frame *f UNUSED, int fd UNUSED)
{
  struct file *file;
  file = find_file_from_fd (fd);
  unsigned *tell = (unsigned *) malloc (sizeof (unsigned));
  *tell = (unsigned) file_tell (file);
  f->eax = (uint32_t) tell;
}

static void
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
      return;
    }
  }
  return;
}

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
