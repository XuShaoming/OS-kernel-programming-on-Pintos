#ifndef _PROC_INFO_
#define _PROC_INFO_

void process_info_init (void);
void add_process_info (tid_t tid);
pid_t get_pid (tid_t tid);
void update_pid (tid_t tid, pid_t pid);
int get_exit_stauts (tid_t tid);
void update_exit_status (tid_t tid, int status);
void destroy_process_info (tid_t tid);
void singal_exit_status (tid_t tid);
void destroy_hash_process_infos (void);

#endif