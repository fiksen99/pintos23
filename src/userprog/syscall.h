#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/kernel/list.h"

/* Implementation for file descriptor */
struct fd_elems
{
  int fd;
  struct file *file;
  struct list_elem elem;
};

void syscall_init (void);

#endif /* userprog/syscall.h */
