#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <user/syscall.h>

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  int *syscall = f->esp;
  if( *syscall == SYS_HALT )  /* ZERO arguments */
  { 
    halt();
  }else if( *syscall == SYS_EXIT || *syscall == SYS_EXEC || *syscall == SYS_WAIT ||
            *syscall == SYS_REMOVE || *syscall == SYS_OPEN || *syscall == SYS_FILESIZE ||
            *syscall == SYS_TELL || *syscall == SYS_CLOSE )  /* ONE argument */
  {

  }else if( *syscall == SYS_CREATE || *syscall == SYS_READ || *syscall == SYS_WRITE ||
            *syscall == SYS_SEEK )  /* TWO arguments */
  {

  }
  printf ("system call!\n");
  thread_exit ();
}
