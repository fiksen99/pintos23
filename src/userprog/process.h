#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/interrupt.h"

#define MAX_ARGS 100

#define STACK_PUSH(esp,type,value) do{(esp)-=sizeof(type);*((type*)(esp))=(value);}while(0)

#define STACK_MAX_SIZE 2000

tid_t process_execute (const char *command);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

bool is_stack_access (void *, struct intr_frame *);
void extend_stack (void);

#endif /* userprog/process.h */
