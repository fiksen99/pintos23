#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

static int write (int fd, const void *buffer, unsigned size);

#endif /* userprog/syscall.h */
