#ifndef _PIDMAP_
#define _PIDMAP_

struct hash* add_pid_map (struct hash* pidmap, pid_t pid, tid_t tid);
tid_t get_tid_from_pidmap (struct hash* pidmap, pid_t pid);
void remove_pid_map (struct hash* pidmap, pid_t pid);
void destroy_pidmap (struct hash* pidmap);

#endif