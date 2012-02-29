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
static void execute_filesize (struct intr_frame *, int) UNUSED;
static void execute_read (struct intr_frame *, int, void *, unsigned) UNUSED;
static void execute_seek (int, unsigned) UNUSED;
static void execute_tell (struct intr_frame *, int) UNUSED;
static void execute_close (int) UNUSED;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Implementation for file descriptor mapping */
struct fd_elems
{
  int fd;
  struct file *file;
};

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
      execute_remove(f, (const char *)arg);
    } else if( syscall == SYS_OPEN )
    {

    } else if( syscall == SYS_FILESIZE )
    {

    } else if( syscall == SYS_TELL )
    {

    } else /*syscall == SYS_CLOSE */
    {

    }
  } else if( syscall == SYS_CREATE || syscall == SYS_SEEK )  /* TWO arguments */
  {
    void *arg1 = stackptr + 1;
    unsigned *arg2 = stackptr + 2;
    if (syscall == SYS_CREATE)
      execute_create(f, (char *)arg1, *arg2);
  } else if( syscall == SYS_READ || syscall == SYS_WRITE )  /* THREE arguments */
  {
    int fd = *(stackptr + 1);
    void *buffer = (void*)(stackptr + 2);
    unsigned size = *(stackptr + 3);
    if( syscall == SYS_WRITE ) {
      execute_write(fd, buffer, size);
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
  if(file_opened == NULL || file == NULL) // Returns -1 if file could not be opened
  {
    *open = -1;
    f->eax = (uint32_t) open;
  }

  //WHAT DO I DO HERE?
}

static void
execute_filesize (struct intr_frame *f UNUSED, int fd UNUSED)
{
  struct file *file;
  
  int *filesize;
  if(file == NULL)
  {
    *filesize = 0;
    f->eax = (uint32_t) filesize;
  }
  //do file_length
  //eax = file_length
}

static void
execute_read (struct intr_frame *f UNUSED, int fd UNUSED, void *buffer UNUSED, unsigned size UNUSED)
{

}

static void
execute_seek (int fd UNUSED, unsigned position UNUSED)
{

}

static void
execute_tell (struct intr_frame *f UNUSED, int fd UNUSED)
{

}

static void
execute_close (int fd UNUSED)
{

} 
