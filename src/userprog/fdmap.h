#ifndef _FDMAP_
#define _FDMAP_

struct hash* fdmap_add (struct hash* fdmap, int fd, struct file* file_);
struct file* fdmap_get (struct hash* fdmap, int fd);
void fdmap_remove (struct hash* fdmap, int fd);
void fdmap_destroy (struct hash* fdmap);

#endif