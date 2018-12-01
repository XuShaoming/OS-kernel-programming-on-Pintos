#include "threads/thread.h"
#include "userprog/processinfo.h"
#include "userprog/process.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include <list.h>
#include <hash.h>
#include <debug.h>

/* Use for share data between child process and parent process
   Destory data about children after parent thread exit
*/

/* List for tell exec program that callee with tid = ** has pid = ** */
static struct hash hash_process_infos;
static struct lock process_info_lock;

struct process_info
{
  tid_t tid;                             /* Identify process */

  pid_t pid;                             /* Data for syscall_exec() */
  bool finish_pid;
  struct condition pid_cond;         

  int exit_status; 					     /* Data for syscall_exit() */
  bool finish_exit;                      
  struct condition exit_cond;

  struct hash_elem helem;                /* Use for hash map */
};

static unsigned process_info_hash (const struct hash_elem *p_, void *aux UNUSED);
static bool process_info_less (const struct hash_elem *a_, const struct hash_elem *b_,
	void *aux UNUSED);
/*
  Function use for hashmap
*/
static unsigned 
process_info_hash (const struct hash_elem *p_, void *aux UNUSED)
{
	struct process_info *p = hash_entry (p_, struct process_info, helem);
	return hash_int (p->tid);
}

static bool 
process_info_less (const struct hash_elem *a_, const struct hash_elem *b_,
	void *aux UNUSED)
{
	const struct  process_info *a = hash_entry (a_, struct process_info, helem);
	const struct  process_info *b = hash_entry (b_, struct process_info, helem);
	return a->tid < b->tid;
}

static void 
hash_free_function (struct hash_elem *element, void *aux UNUSED)
{
  struct process_info *entry = hash_entry (element, struct process_info, helem);
  free (entry);
}

void
process_info_init ()
{
  lock_init (&process_info_lock);
  hash_init (&hash_process_infos, process_info_hash, process_info_less, NULL);
}

/* 
   Called in thread_create
   Create condition that syscall_exec call wait on
*/
void 
add_process_info (tid_t tid)
{
	lock_acquire (&process_info_lock);

	struct process_info *new_process_info = malloc (sizeof (struct process_info));

	new_process_info->tid = tid;

	new_process_info->pid = -1;
	new_process_info->finish_pid = false;
	cond_init (&new_process_info->pid_cond);

	new_process_info->exit_status = -1;
	new_process_info->finish_exit = false;
	cond_init (&new_process_info->exit_cond);

	hash_insert (&hash_process_infos, &new_process_info->helem);

	lock_release (&process_info_lock);
}

/* Called in syscall_exec
   Get the pid of the process it create
   If the process haven't finish load, release the lock
   and wait on condition
*/
pid_t 
get_pid (tid_t tid)
{
	lock_acquire (&process_info_lock);
	struct process_info search_key;
	search_key.tid = tid;

	struct hash_elem *value_;
	value_ = hash_find (&hash_process_infos, &search_key.helem);
	struct process_info* value = hash_entry (value_, struct process_info, helem);
	while (!value->finish_pid)
		cond_wait (&value->pid_cond, &process_info_lock);
	
	int result = value->pid;
	lock_release (&process_info_lock);
	return result;
}

/* Called when process finish load
   Update the pid after load result 
   Signal caller process that is waiting */
void 
update_pid (tid_t tid, pid_t pid)
{
	lock_acquire (&process_info_lock);
	struct process_info search_key;
	search_key.tid = tid;

	struct hash_elem *value_;
	value_ = hash_find (&hash_process_infos, &search_key.helem);
	struct process_info *value = hash_entry (value_, struct process_info, helem);

	value->finish_pid = true;
	value->pid = pid;

	cond_signal (&value->pid_cond, &process_info_lock);
	lock_release (&process_info_lock);
}

/* Called in thread_wait
   Get the exit status of the thread it create
   If the process haven't finish exit, release the lock
   and wait on condition
*/
int 
get_exit_stauts (tid_t tid)
{
	lock_acquire (&process_info_lock);
	struct process_info search_key;
	search_key.tid = tid;

	struct hash_elem *value_;
	value_ = hash_find (&hash_process_infos, &search_key.helem);
	struct process_info* value = hash_entry (value_, struct process_info, helem);

	while (!value->finish_exit)
		cond_wait (&value->exit_cond, &process_info_lock);
	
	/* To avoid process_wait () call agian for same tid, set exit_status = -1 */
	int result = value->exit_status;
	value->exit_status = -1;

	lock_release (&process_info_lock);
	return result;
}

/* Called in syscall_exit()
   Update the exit result
*/
void
update_exit_status (tid_t tid, int status)
{
	lock_acquire (&process_info_lock);

	struct process_info search_key;
	search_key.tid = tid;
	struct hash_elem *value_ = hash_find (&hash_process_infos, &search_key.helem);
	struct process_info *value = hash_entry (value_, struct process_info, helem);

	value->exit_status = status;

	lock_release (&process_info_lock);
}

/* Called in thread_exit ()
   Signal caller thread that is waiting for current thread
*/
void
singal_exit_status (tid_t tid)
{
	lock_acquire (&process_info_lock);

	struct process_info search_key;
	search_key.tid = tid;
	struct hash_elem *value_ = hash_find (&hash_process_infos, &search_key.helem);
	struct process_info *value = hash_entry (value_, struct process_info, helem);

	value->finish_exit = true;
	cond_signal (&value->exit_cond, &process_info_lock);

	lock_release (&process_info_lock);
}

/* 
   Called when parent process exit
   Destroy all info about child 
   Free resources
*/
void destroy_process_info (tid_t tid)
{
  lock_acquire (&process_info_lock);

  struct process_info search_key;
  search_key.tid = tid;
  struct hash_elem *value_ = hash_delete (&hash_process_infos, &search_key.helem);
  struct process_info *value = hash_entry (value_, struct process_info, helem);

  free(value);
  lock_release (&process_info_lock);
}

/*
  Called before initial process finish
  Free resources use for this hash map
*/
void destroy_hash_process_infos ()
{
  hash_destroy (&hash_process_infos, hash_free_function);
}
