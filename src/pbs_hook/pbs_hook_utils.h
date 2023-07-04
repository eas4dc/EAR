#define _GNU_SOURCE
#include <sched.h>
#include <common/types/generic.h>

uint pbs_util_get_ID(int job_id, int step_id);
int pbs_util_get_eard_port(char *ear_tmp);
int pbs_util_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask, int *errno_sym);
#if 0
int pbs_util_print_affinity_mask(pid_t pid, cpu_set_t *mask);
#endif
