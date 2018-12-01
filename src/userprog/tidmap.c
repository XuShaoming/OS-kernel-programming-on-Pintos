#include "threads/thread.h"
#include "userprog/tidmap.h"
#include "userprog/processinfo.h"
#include "threads/malloc.h"
#include <debug.h>
#include <hash.h>

struct tidmap_entry // to contruct the hash_elem
{
  tid_t tid;
  struct hash_elem helem;
};

/* Hash funcition */
static unsigned
tidmap_entry_hash (const struct hash_elem *p_, void *aux UNUSED)
{
	const struct tidmap_entry *p = hash_entry (p_, struct tidmap_entry, helem);
	return hash_int (p->tid);
}

/* Hash function */
static bool
tidmap_entry_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
	const struct tidmap_entry *a = hash_entry (a_, struct tidmap_entry, helem);
	const struct tidmap_entry *b = hash_entry (b_, struct tidmap_entry, helem);

	return a->tid < b->tid;
}

/*
  Use in hash_destroy
  Free every element in hashmap
  Free its resources in process info 
*/
static void 
hash_free_func (struct hash_elem *element, void *aux UNUSED)
{
  struct tidmap_entry *entry = hash_entry (element, struct tidmap_entry, helem);
  destroy_process_info (entry->tid);
  free (entry);
}

/* 
  Create tidmap
  Save all tid that create by call process_create()
*/
struct hash*
create_tidmap ()
{
	struct hash *tidmap = malloc (sizeof (struct hash));
	hash_init (tidmap, tidmap_entry_hash, tidmap_entry_less, NULL);
	return tidmap;
}

/* 
  Add a tid to tidmap
  Called in process_create()
*/
struct hash*
add_tidmap (struct hash *tidmap, tid_t tid)
{
  if (tidmap == NULL) tidmap = create_tidmap ();
	struct tidmap_entry *entry = malloc (sizeof (struct tidmap_entry));
	entry->tid = tid;
	hash_insert (tidmap, &entry->helem);
  return tidmap;
}

/* 
  Check if tid contains in tidmap
  Called in process_wait() to check if that tid is a child 
  of current process
*/
bool
contains_tidmap (struct hash *tidmap, tid_t tid)
{
	if (tidmap == NULL) return false;
  struct tidmap_entry key;
	key.tid = tid;
	return hash_find (tidmap, &key.helem) != NULL;
}

/* 
  Called in thread_exit ()
  Destory map and free all resource about that tid in exec_result and exit_result
*/
void 
destroy_tidmap (struct hash *tidmap)
{
  if (tidmap == NULL) return;
	hash_destroy (tidmap, hash_free_func);
	free(tidmap);
}

