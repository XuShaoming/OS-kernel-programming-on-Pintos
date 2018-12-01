#include <hash.h>
#include "threads/malloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "userprog/fdmap.h"


struct fdmap_entry
{
	int fd;
	struct file* file;
	struct hash_elem helem;
};

static struct hash* fdmap_create (void);

/* Hash funcition */
static unsigned
fdmap_entry_hash (const struct hash_elem *p_, void *aux UNUSED)
{
	const struct fdmap_entry *p = hash_entry (p_, struct fdmap_entry, helem);
	return hash_int (p->fd);
}

/* Hash function */
static bool
fdmap_entry_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
	const struct fdmap_entry *a = hash_entry (a_, struct fdmap_entry, helem);
	const struct fdmap_entry *b = hash_entry (b_, struct fdmap_entry, helem);

	return a->fd < b->fd;
}

/*
  Use in hash_destroy
  Free every element in hashmap
*/
static void 
hash_free_func (struct hash_elem *element, void *aux UNUSED)
{
  struct fdmap_entry *entry = hash_entry (element, struct fdmap_entry, helem);
  file_close (entry->file);
  free (entry);
}

/* 
   Create fddmap 
*/
static struct hash* 
fdmap_create (void)
{
	struct hash *fdmap = malloc (sizeof (struct hash));
	hash_init (fdmap, fdmap_entry_hash, fdmap_entry_less, NULL);
	return fdmap;
}

/* 
   Add (fd, file*) to fdmap
*/
struct hash*
fdmap_add (struct hash* fdmap, int fd, struct file* file_)
{
  if (fdmap == NULL) fdmap = fdmap_create ();
  struct fdmap_entry *entry = malloc (sizeof (struct fdmap_entry));
  entry->fd = fd;
  entry->file = file_;
  hash_insert (fdmap, &entry->helem);
  return fdmap;
}

/* 
  Return file* map to fd
*/
struct file*
fdmap_get (struct hash* fdmap, int fd)
{
	if (fdmap == NULL) return NULL;
	struct fdmap_entry search_key;
	search_key.fd = fd;
	struct hash_elem *value_ = hash_find (fdmap, &search_key.helem);
	if (value_ == NULL)
		return NULL;
	struct fdmap_entry *value = hash_entry (value_, struct fdmap_entry, helem);
	return value->file;
}

/* 
  Remove (fd, file*) in fdmap
  close file*
*/
void
fdmap_remove (struct hash* fdmap, int fd)
{
	if (fdmap == NULL) return;
	struct fdmap_entry search_key;
	search_key.fd = fd;

	struct hash_elem *value_ = hash_delete (fdmap, &search_key.helem);

	if (value_ != NULL)
	{
		struct fdmap_entry *value = hash_entry (value_, struct fdmap_entry, helem);

		filesys_lock_acquire ();
		file_close (value->file);
		filesys_lock_release ();
		
		free (value);
	}
}

/*
  Called in thread_exit()
  free every element in hash map and free map itself
*/
void
fdmap_destroy (struct hash* fdmap)
{
	if (fdmap == NULL) return;

	filesys_lock_acquire ();
	hash_destroy (fdmap, hash_free_func);
	filesys_lock_release ();

	free (fdmap);
}
