
#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"

struct FILE{
	struct file* file;
	int fd;
	struct list_elem elem;
};

void syscall_init (void);

#endif /* userprog/syscall.h */
