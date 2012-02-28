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

static void syscall_handler (struct intr_frame *);
static void execute_exit (struct intr_frame *, int *);
static void execute_exec (struct intr_frame *, const char **);
static void execute_wait (struct intr_frame *, tid_t);
static int execute_write (int, const void *, unsigned);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  uint32_t *stackptr = f->esp;
  int syscall = *stackptr;
  if( syscall == SYS_HALT )  /* ZERO arguments */
  { 
//    shutdown_power_off();
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
//      pid_t pid = (pid_t)*arg;
      execute_wait( f, (tid_t)*arg );
    } else if( syscall == SYS_REMOVE )
    {

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
//    void *arg1 = stackptr + 1;
//    unsigned *arg2 = stackptr + 2;
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
  if( *id != TID_ERROR ) {
    struct child_status* s = malloc( sizeof(struct child_status) );
    (*s).tid = *id;
    sema_init( s->sema, 0 );
    list_push_front (&(thread_current()->children), &(s->elem));
  }
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


