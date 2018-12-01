#include <hash.h>
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pidmap.h"

struct pidmap_entry
{
	pid_t pid;
	tid_t tid;
	struct hash_elem helem;
};

static struct hash* create_pidmap (void);

/* Hash funcition */
static unsigned
pidmap_entry_hash (const struct hash_elem *p_, void *aux UNUSED)
{
	const struct pidmap_entry *p = hash_entry (p_, struct pidmap_entry, helem);
	return hash_int (p->pid);
}

/* Hash function */
static bool
pidmap_entry_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
	const struct pidmap_entry *a = hash_entry (a_, struct pidmap_entry, helem);
	const struct pidmap_entry *b = hash_entry (b_, struct pidmap_entry, helem);

	return a->pid < b->pid;
}

/*
  Use in hash_destroy
  Free every element in hashmap
*/
static void 
hash_free_func (struct hash_elem *element, void *aux UNUSED)
{
  struct pidmap_entry *entry = hash_entry (element, struct pidmap_entry, helem);
  free (entry);
}

/* 
   Create pidmap, 
   save all pid that successfully load by call exec() by current
   process
*/
static struct hash* 
create_pidmap (void)
{
	struct hash *pidmap = malloc (sizeof (struct hash));
	hash_init (pidmap, pidmap_entry_hash, pidmap_entry_less, NULL);
	return pidmap;
}

/* 
   Add (pid, tid) to pidmap
   Called in syscall_exec () and successfully run a process
   add_pid_map (t->pidmap, pid, child_tid)
*/
struct hash*
add_pid_map (struct hash* pidmap, pid_t pid, tid_t tid)
{
	if (pidmap == NULL) pidmap = create_pidmap ();
	struct pidmap_entry *entry = malloc (sizeof (struct pidmap_entry));
	entry->pid = pid;
	entry->tid = tid;
	hash_insert (pidmap, &entry->helem);
	return pidmap;
}

/* 
  Return tid map to pid
  Use in syscall_wait to call thread_wait
  If doesn't contains pid, return -1
*/
tid_t
get_tid_from_pidmap (struct hash* pidmap, pid_t pid)
{
	if (pidmap == NULL) return TID_ERROR;
	struct pidmap_entry search_key;
	search_key.pid = pid;
	struct hash_elem *value_ = hash_find (pidmap, &search_key.helem);
	if (value_ == NULL)
		return TID_ERROR;
	struct pidmap_entry *value = hash_entry (value_, struct pidmap_entry, helem);
	return value->tid;
}

/* 
  Remove (pid, tid) in pidmap
  Called in syscall_wait() finish
  For next time wait for same pid, directly return -1
*/
void
remove_pid_map (struct hash* pidmap, pid_t pid)
{
	if (pidmap == NULL) return;
	struct pidmap_entry search_key;
	search_key.pid = pid;

	struct hash_elem *value_ = hash_delete (pidmap, &search_key.helem);

	if (value_ != NULL)
	{
		struct pidmap_entry *value = hash_entry (value_, struct pidmap_entry, helem);
		free (value);
	}
}

/*
  Called in thread_exit()
  free every element in hash map and free map itself
*/
void
destroy_pidmap (struct hash* pidmap)
{
	if (pidmap == NULL) return;
	hash_destroy (pidmap, hash_free_func);
	free (pidmap);
}


