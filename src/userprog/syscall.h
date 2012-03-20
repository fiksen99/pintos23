#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/kernel/list.h"
#include "threads/thread.h"

/* Map region identifier. */
typedef uint32_t mapid_t;
#define MAP_FAILED ((mapid_t) - 1)

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
