#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/processinfo.h"
#include "userprog/pidmap.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "userprog/fdmap.h"

static void syscall_handler (struct intr_frame *);

static void check_user_vaddr (void *vaddr, size_t);
static void check_user_vaddr_single (void *vaddr);
static void syscall_halt (void);
static void syscall_exit (struct intr_frame *);
static void syscall_exec (struct intr_frame *);
static void syscall_wait (struct intr_frame *);
static void syscall_create (struct intr_frame *);
static void syscall_remove (struct intr_frame *);
static void syscall_open (struct intr_frame *f);
static void syscall_filesize (struct intr_frame *f);
static void syscall_read (struct intr_frame *f);
static void syscall_write (struct intr_frame *);
static void syscall_seek (struct intr_frame *f);
static void syscall_tell (struct intr_frame *f);
static void syscall_close (struct intr_frame *f);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  check_user_vaddr (f->esp, sizeof(int)); //check the invalid pointers passed by user.
  int *call_number_addr = (int *)f->esp;  //the system call number is in the 32-bit word at the caller's stack pointer
  int call_number = *call_number_addr;
  switch (call_number) {
  	case SYS_HALT:
  	  syscall_halt ();
      break;
  	case SYS_EXIT:
  		syscall_exit (f);
  		break;
  	case SYS_EXEC:
      syscall_exec (f);
      break;
    case SYS_WAIT:
    	syscall_wait (f);
    	break;
    case SYS_CREATE:                 /* Create a file. */
    	syscall_create (f);
    	break;
    case SYS_REMOVE:                 /* Delete a file. */
    	syscall_remove (f);
    	break;
    case SYS_OPEN:                   /* Open a file. */
    	syscall_open (f);
    	break;
    case SYS_FILESIZE:               /* Obtain a file's size. */
    	syscall_filesize (f);
    	break;
    case SYS_READ:                   /* Read from a file. */
    	syscall_read (f);
    	break;
    case SYS_WRITE:                  /* Write to a file. */
    	syscall_write (f);
    	break;
    case SYS_SEEK:                   /* Change position in a file. */
    	syscall_seek (f);
    	break;
    case SYS_TELL:                   /* Report current position in a file. */
    	syscall_tell (f);
    	break;
    case SYS_CLOSE:                  /* Close a file. */
    	syscall_close (f);
    	break;
  	default:
  	  thread_exit ();
  	  break;
  }
}

/*
   Check if pointer given by user is valid, or exit the thread and release its resources
*/
static void 
check_user_vaddr (void *vaddr, size_t size)
{
	for (size_t i = 0; i < size; i++) 
	{
		check_user_vaddr_single (vaddr);
		vaddr++;
	}
}

static void 
check_user_vaddr_single (void *vaddr)
{
	if (vaddr == NULL || is_kernel_vaddr(vaddr)) 
	{
		thread_exit ();
	}

	struct thread* t = thread_current ();
	uint32_t *addr = pagedir_get_page (t->pagedir, vaddr);
	if (addr == NULL)
		thread_exit ();
}

static void
syscall_halt ()
{
	shutdown_power_off ();
}

static void 
syscall_exit (struct intr_frame *f UNUSED)
{
	int *status_addr = (int *)(f->esp+4);  //the first argument is in the 32-bit word at the next higher address
	check_user_vaddr (status_addr, sizeof(int));
	int status = *status_addr;

	struct thread* t = thread_current ();

	update_exit_status (t->tid, status);
	t->exit_status = status;

	f->eax = status;		// return the status.

	thread_exit ();
}

static void
syscall_exec (struct intr_frame *f)
{
	char **cmd_line_addr = (char **)(f->esp+4);
	check_user_vaddr (cmd_line_addr, sizeof(char*));
	char *cmd_line = *cmd_line_addr;

	if (cmd_line == NULL) 
		thread_exit ();

	check_user_vaddr (cmd_line, 2);

	tid_t child_tid = process_execute (cmd_line);
	pid_t pid = get_pid (child_tid);
	struct thread* t = thread_current ();
	if (pid != -1)
		t->pidmap = add_pid_map (t->pidmap, pid, child_tid);  // correlate the child_tid with pid(child)

	f->eax = pid;
}

static void
syscall_wait (struct intr_frame *f)
{
	struct thread* t = thread_current ();
	tid_t tid = -1;

	pid_t* pid_addr = (pid_t *)(f->esp+4);
	check_user_vaddr (pid_addr, sizeof(pid_t));
	pid_t pid = *pid_addr;

	if ((tid = get_tid_from_pidmap (t->pidmap, pid)) != TID_ERROR) 
	{
		int result = process_wait (tid);
		remove_pid_map (t->pidmap, pid);
		f->eax = result;
	}
	else //pid does not refer to a direct child of the calling process
	{
		f->eax = -1;
	}
	
}

static void
syscall_create (struct intr_frame *f)
{
	char **file_ = f->esp+4;
	check_user_vaddr (file_, sizeof(char *));
	char* file = *file_;

	if (file == NULL) 
		thread_exit ();
	/* Ignore other char after first one, Should add*/
	check_user_vaddr (file, 1);

	unsigned *initial_size_ = f->esp+8;
	check_user_vaddr (initial_size_, sizeof(unsigned));
	unsigned initial_size = *initial_size_;

	filesys_lock_acquire ();  // In process.c
	bool success = filesys_create (file, initial_size);
	filesys_lock_release ();

	f->eax = success;
}

static void
syscall_remove (struct intr_frame *f)
{
	char **file_ = f->esp+4;
	check_user_vaddr (file_, sizeof(char *));
	char* file = *file_;

	if (file == NULL) 
		thread_exit ();
	/* Ignore other char after first one, Should add*/
	check_user_vaddr (file, 1);

	filesys_lock_acquire ();
	filesys_remove (file);
	filesys_lock_release ();
}

static void 
syscall_open (struct intr_frame *f)
{
	char **file_name_ = f->esp+4;
	check_user_vaddr (file_name_, 4);
	char *file_name = *file_name_;

	if (file_name == NULL)
		thread_exit ();
	check_user_vaddr (file_name, 1);

	filesys_lock_acquire ();
	struct file* file = filesys_open (file_name);
	filesys_lock_release ();

	if (file == NULL)
	{
		f->eax = -1;
		return;
	}

	struct thread *t = thread_current ();
	int fd = t->fd_base++;
	t->fdmap = fdmap_add (t->fdmap, fd, file);
	f->eax = fd;

}

static void
syscall_filesize (struct intr_frame *f)
{
	int* fd_ = f->esp+4;
	check_user_vaddr (fd_, sizeof(int));
	int fd = *fd_;

	struct file* file = fdmap_get (thread_current()->fdmap, fd);
	if (file == NULL)
		thread_exit ();

	int length = 0;
	filesys_lock_acquire ();
	length = file_length (file);
	filesys_lock_release ();

	f->eax = length;
}

static void
syscall_read (struct intr_frame *f)
{
	int *fd_addr = (int *)(f->esp+4);
	check_user_vaddr (fd_addr, sizeof(int));
	int fd = *fd_addr;

	void **buffer_addr = (void **)(f->esp+8);
	check_user_vaddr (buffer_addr, sizeof (void*));
	void *buffer  = *buffer_addr;

	if (buffer == NULL)
		thread_exit ();

	check_user_vaddr (buffer, 1);

	int *size_addr = (int *)(f->esp+12);
	check_user_vaddr (size_addr, sizeof (int));
	int size = *size_addr;

	if (fd == 0)
	{
		int count;
		for (count = 0; count < size; count++)
		{
			check_user_vaddr_single (buffer);
			uint8_t c = input_getc ();
			memcpy (buffer++, &c, 1);
		}
		f->eax = count;

	} else {
		struct file* file = fdmap_get (thread_current()->fdmap, fd);
		if (file == NULL) 
		{
			f->eax = -1;
		} else {
			filesys_lock_acquire ();
			int result = file_read (file, buffer, size);
			filesys_lock_release ();
			f->eax = result;
		}
	}
}

static void 
syscall_write (struct intr_frame *f)
{
	int *fd_addr = (int *)(f->esp+4);
	check_user_vaddr (fd_addr, sizeof(int));
	int fd = *fd_addr;

	void **buffer_addr = (void **)(f->esp+8);
	check_user_vaddr (buffer_addr, sizeof (void*));
	void *buffer  = *buffer_addr;

	if (buffer == NULL)
		thread_exit ();

	check_user_vaddr (buffer, 1);

	int *size_addr = (int *)(f->esp+12);
	check_user_vaddr (size_addr, sizeof (int));
	int size = *size_addr;

	if (fd == 1)
	{
		putbuf(buffer, size);
		f->eax = size;
	} else {

		struct file* file = fdmap_get (thread_current()->fdmap, fd);
		if (file == NULL) 
		{
			f->eax = -1;
		} else {
			filesys_lock_acquire ();
			int result = file_write (file, buffer, size);
			filesys_lock_release ();
			f->eax = result;
		}
	} 

	
}

static void
syscall_seek (struct intr_frame *f)
{
	int *fd_addr = (int *)(f->esp+4);
	check_user_vaddr (fd_addr, sizeof(int));
	int fd = *fd_addr;

	unsigned *position_ = f->esp+8;
	check_user_vaddr (position_, sizeof(unsigned));
	unsigned position = *position_;

	struct file* file = fdmap_get (thread_current()->fdmap, fd);
	if (file == NULL) 
	{
		thread_exit ();
	} else {
		filesys_lock_acquire ();
		file_seek (file, position);
		filesys_lock_release ();
	}

}

static void
syscall_tell (struct intr_frame *f)
{
	int *fd_addr = (int *)(f->esp+4);
	check_user_vaddr (fd_addr, sizeof(int));
	int fd = *fd_addr;

	struct file* file = fdmap_get (thread_current()->fdmap, fd);
	if (file == NULL) 
	{
		thread_exit ();
	} else {
		filesys_lock_acquire ();
		int result = file_tell (file);
		filesys_lock_release ();

		f->eax = result;
	}
}

static void
syscall_close (struct intr_frame *f)
{
	int* fd_ = f->esp+4;
	check_user_vaddr (fd_, sizeof(int));
	int fd = *fd_;

	fdmap_remove (thread_current()->fdmap, fd);
}

