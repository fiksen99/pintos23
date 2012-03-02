#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/kernel/list.h"
#include "threads/thread.h"

/* Implementation for file descriptor */
struct fd_elems
{
  int fd;
  struct file *file;
  tid_t tid;
  struct list_elem elem;
};

void syscall_init (void);
void close_thread_fds (void);

#endif /* userprog/syscall.h */
