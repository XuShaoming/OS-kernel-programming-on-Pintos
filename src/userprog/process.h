#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

void process_init (void);
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void process_finish (void);

void filesys_lock_acquire (void);
void filesys_lock_release (void);

#endif /* userprog/process.h */
