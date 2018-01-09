#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
//#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

struct lock file_lock;



static void syscall_handler (struct intr_frame *);

struct file* get_file(int fd);
void valid_addres(const void * vaddr);
void valid_buffer(void* buffer, unsigned size);
void get_arg(uint32_t* esp_array, int args_number, int* arguments);
tid_t handle_exec(const char* comand);
void handle_exit(int exit_value);
int handle_wait(tid_t tid);
int handle_open_file(const char* file);
int handle_read_file(int fd, void* buffer, unsigned size);
bool handle_create_file(const char* file, unsigned size);
int handle_write_file(int fd, const void* buffer, unsigned size);
void handle_seek_file(int fd, unsigned position);
void handle_close_file(int fd);

void
syscall_init (void) 
{
	lock_init(&file_lock);
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

struct file*
get_file(int fd){
	struct thread* cur = thread_current();
	struct list_elem *next, *element = list_begin(&cur->file_list);

	while(element != list_end(&cur->file_list)){
		next = list_next(element);
		struct FILE* FILE = list_entry(element, struct FILE, elem);
		if(fd == FILE->fd){
			return FILE->file;
		}
		element = next;
	}
	return NULL;
}

void
valid_addres(const void * vaddr){
	if(!is_user_vaddr(vaddr) || vaddr < (const void *)0x08048000){
		handle_exit(-1);
	}
}

void
valid_buffer(void* buffer, unsigned size){

	if (size < 0)
		handle_exit(-1);

	char* aux = (char*) buffer;
	unsigned i = 0;
	for(i = 0; i < size; i++){
		valid_addres((const void*)aux);
		aux++;
	}
}

void
get_arg(uint32_t* esp_array, int args_number, int *arguments){
	int i = 0;
	for(i = 1; i < args_number+1; i++){
		valid_addres((const void*)(esp_array + i));
		arguments[i] = malloc(sizeof(int *));
		arguments[i] = (int) esp_array[i];
	}
}

void
handle_exit(int exit_value){

	struct thread* parent=get_thread(thread_current()->parent_tid);
	if (parent != NULL)
	{
		parent->child_thread->exit_value = exit_value;
	}

	thread_exit();
}

tid_t
handle_exec(const char* comand){

	tid_t tid = process_execute(comand);
	struct child_thread* child = thread_current()->child_thread;

	while(child->load == false);


	return tid;
}

int
handle_wait(tid_t tid){
	return process_wait(tid);
}

bool
handle_create_file(const char* file, unsigned size){
	lock_acquire(&file_lock);
	bool success = filesys_create(file, size);
	lock_release(&file_lock);
	return success;
}

int
handle_open_file(const char* file){
	lock_acquire(&file_lock);
	struct file* f = filesys_open(file);

	if(!f){
		lock_release(&file_lock);
		return -1;
	}else{
		struct FILE* FILE = malloc(sizeof(struct FILE));
		FILE->file = f;
		FILE->fd = thread_current()->fd;
		thread_current()->fd ++;
		list_push_back(&thread_current()->file_list, &FILE->elem);
		lock_release(&file_lock);
		return FILE->fd;
	}
}

int
handle_read_file(int fd, void* buffer, unsigned size){
	if(fd == STDIN_FILENO){
		unsigned i = 0;
		uint8_t* aux = (uint8_t*) buffer;
		for(i = 0; i < size; i++){
			aux[i] = input_getc();
		}
		return size;
	}else{
		lock_acquire(&file_lock);
		struct file* file = get_file(fd);
		if(!file){
			lock_release(&file_lock);
			return -1;
		}else{
			int bytes = file_read(file, buffer, size);
			lock_release(&file_lock);
			return bytes;
		}
	}
}

int
handle_write_file(int fd, const void* buffer, unsigned size){

	if(fd == STDOUT_FILENO){
		putbuf(buffer, size);
		return size;
	}else{
		lock_acquire(&file_lock);
		struct file* file = get_file(fd);
		if(!file){
			lock_release(&file_lock);
			return -1;
		}else{
			int bytes = file_write(file, buffer, size);
			lock_release(&file_lock);
			return bytes;
		}
	}
}

void
handle_seek_file(int fd, unsigned position){
	lock_acquire(&file_lock);
	struct file* file = get_file(fd);
	if(!file){
		lock_release(&file_lock);
		return;
	}else{
		file_seek(file,position);
		lock_release(&file_lock);
	}
}

void
handle_close_file(int fd){
	lock_acquire(&file_lock);

	struct thread* cur = thread_current();
	struct list_elem *next, *element = list_begin(&cur->file_list);

	while(element != list_end(&cur->file_list)){
		next = list_next(element);
		struct FILE *FILE = list_entry( element, struct FILE, elem);
		if(fd ==FILE->fd){
			file_close(FILE->file);
			list_remove(&FILE->elem);
			free(FILE);
			break;
		}
		element = next;
	}

	lock_release(&file_lock);
}


#define MAX_NUMER_OF_ARGS 5

static void
syscall_handler (struct intr_frame *f)
{
	int arguments[MAX_NUMER_OF_ARGS], i;
	uint32_t* esp_array;
	esp_array = (uint32_t*)f->esp;

	for(i = 0; i < MAX_NUMER_OF_ARGS; i++)
	{
		arguments[i] = 0;
	}

	switch(esp_array[0])
	{
		case SYS_EXIT:
		{
			get_arg(esp_array, 1, arguments);
			handle_exit(arguments[1]);
			break;
		}
		case SYS_EXEC:
		{
			get_arg(esp_array, 1, arguments);
			arguments[1] = (int)pagedir_get_page(thread_current()->pagedir, (const void*)arguments[1]);
			if(!arguments[1]){
				handle_exit(-1);
			}

			f->eax = handle_exec((const char*)arguments[1]);
			break;
		}
		case SYS_WAIT:
		{
			get_arg(esp_array, 1, arguments);
			f->eax = handle_wait(arguments[1]);
			break;
		}
		case SYS_CREATE:
		{
			get_arg(esp_array, 2, arguments);
			arguments[1] = (int)pagedir_get_page(thread_current()->pagedir, (const void*)arguments[1]);
			if(!arguments[1]){
				handle_exit(-1);
			}

			f->eax = handle_create_file((const char*)arguments[1], (unsigned)arguments[2]);
			break;
		}
		case SYS_OPEN:
		{
			get_arg(esp_array, 1, arguments);
			arguments[1] = (int)pagedir_get_page(thread_current()->pagedir,(const void*) arguments[1]);
			if(!arguments[1]){
				handle_exit(-1);
			}

			f->eax = handle_open_file((const char*)arguments[1]);
			break;
		}
		case SYS_READ:
		{
			get_arg(esp_array, 3, arguments);
			valid_buffer((void*)arguments[1], (unsigned)arguments[2]);
			arguments[2] = (int)pagedir_get_page(thread_current()->pagedir,(const void*) arguments[2]);
			if(!arguments[2]){
				handle_exit(-1);
			}

			f->eax = handle_read_file(arguments[1], (void*)arguments[2], (unsigned)arguments[3]);
			break;
		}
		case SYS_WRITE:
		{
			get_arg(esp_array, 3, arguments);
			valid_buffer((void*)arguments[2], (unsigned)arguments[3]);
			arguments[2] = (int)pagedir_get_page(thread_current()->pagedir, (const void*)arguments[2]);
			if(!arguments[2]){
				handle_exit(-1);
			}

			f->eax = handle_write_file(arguments[1], (const void*)arguments[2], (unsigned)arguments[3]);
			break;
		}
		case SYS_SEEK:
		{
			get_arg(esp_array, 2, arguments);
			handle_seek_file(arguments[1], (unsigned)arguments[2]);
			break;
		}
		case SYS_CLOSE:
		{
			get_arg(esp_array, 1, arguments);
			handle_close_file(arguments[1]);
			break;
		}
		case SYS_MMAP:
		{
			get_arg(esp_array, 2, arguments);
			if(arguments[2] == 0 || arguments[1] == 0 || arguments[1] == 1){
					f->eax = -1;
					break;
			}
			handle_mmap(arguments[1], arguments[2]);
			break;
		}
		case SYS_MUNMAP:
		{
			get_arg(esp_array, 1, arguments);
			handle_munmap(arguments[1]);
			break;
		}
	}
}


