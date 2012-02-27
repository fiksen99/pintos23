#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  int *stackptr = f->esp;
  int syscall = *stackptr;
  if( syscall == SYS_HALT )  /* ZERO arguments */
  { 
    
  } else if( syscall == SYS_EXIT || syscall == SYS_EXEC || syscall == SYS_WAIT ||
              syscall == SYS_REMOVE || syscall == SYS_OPEN || syscall == SYS_FILESIZE ||
              syscall == SYS_TELL || syscall == SYS_CLOSE )  /* ONE argument */
  {
    void *arg = stackptr + 1;
    if( syscall == SYS_EXIT ) {
      f->eax = (uint32_t)arg;
      thread_exit();
    }
  } else if( syscall == SYS_CREATE || syscall == SYS_SEEK )  /* TWO arguments */
  {
    void *arg1 = stackptr + 1;
    unsigned *arg2 = stackptr + 2;
  } else if( syscall == SYS_READ || syscall == SYS_WRITE )  /* THREE arguments */
  {
    int fd = *(stackptr + 1);
    void *buffer = *(stackptr + 2);
    unsigned size = *(stackptr + 3);
    if( syscall == SYS_WRITE ) {
      f->eax = write(fd, buffer, size);
    } else {
//      read();
    }
  }
/*  printf ("system call!\n");
  thread_exit ();*/
}

static int
write (int fd, const void *buffer, unsigned size)
{
  if (fd == STDOUT_FILENO )     /*write to console*/
  {
    putbuf( buffer, size );
    return size;
  }
  return 0;
}


