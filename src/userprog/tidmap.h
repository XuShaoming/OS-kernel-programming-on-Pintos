#ifndef _TIDMAP_
#define _TIDMAP_

struct hash* create_tidmap (void);
struct hash* add_tidmap (struct hash *tidmap, tid_t tid);
bool contains_tidmap (struct hash *tidmap, tid_t tid);
void destroy_tidmap (struct hash *tidmap);

#endif