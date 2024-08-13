/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>

// #define SHOW_DEBUGS 1

#include <common/config.h>
#include <common/output/debug.h>
#include <common/system/folder.h>
#include <common/hardware/topology.h>
#include <common/hardware/hardware_info.h>

#define INTEL_VENDOR_NAME       "GenuineIntel"


static topology_t topo1;
static topology_t topo2;
static int init;


int detect_packages(int **mypackage_map) 
{
	int num_cpus, num_cores, num_packages = 0;
	int *package_map;
	int i;

	if (!init)
	{
		topology_init(&topo1);
		topology_select(&topo1, &topo2, TPSelect.socket, TPGroup.merge, 0);
		init = 1;
	}
    
	num_cores    = topo1.core_count;
	num_packages = topo1.socket_count;
	num_cpus     = topo1.socket_count;
	
	if (num_cpus < 1 || num_cores < 1) {
        	return 0;
	}

	if (mypackage_map != NULL) {
		*mypackage_map = calloc(num_cores, sizeof(int));
		package_map = *mypackage_map;
	}

	for (i = 0; mypackage_map != NULL && i < topo2.cpu_count; ++i) {
		package_map[i] = topo2.cpus[i].id;
	}

	return num_packages;
}


state_t cpumask_all_cpus(cpu_set_t *mask)
{
	for (int i = 0; i < CPU_SETSIZE; i++)
	{
		CPU_SET(i, mask);
	}

	return EAR_SUCCESS;
}


int cpumask_count(cpu_set_t *my_mask)
{
	if (my_mask)
	{
		return CPU_COUNT(my_mask);
	} else {
		return -1;
	}
}


int cpumask_getlist(cpu_set_t *my_mask, uint n_cpus, uint *cpu_list)
{
    if (my_mask == NULL || cpu_list == NULL || n_cpus < 1) return -1;

    uint c, lcpus = 0;

    for (c = 0; c < MAX_CPUS_SUPPORTED && lcpus < n_cpus; c++) {
        if (CPU_ISSET(c, my_mask)) {
            cpu_list[lcpus] = c;
            lcpus++;
        }
    }

    if (lcpus < n_cpus) {
        return -1;
    }

    return 0;

}


void cpumask_aggregate(cpu_set_t *dst, cpu_set_t *src)
{
	CPU_OR(dst, dst, src);
}


void cpumask_remove(cpu_set_t *dst, cpu_set_t *src)
{
#if 0
	for (uint c=0; c < MAX_CPUS_SUPPORTED; c++){
		if (CPU_ISSET(c, src)) CPU_CLR(c, dst);
	}
#endif
	CPU_OR(dst, dst, src);
	CPU_XOR(dst, dst, src);
}


void cpumask_not(cpu_set_t *dst, cpu_set_t *src)
{
#if 0
	CPU_ZERO(dst);
	for (uint c=0; c < MAX_CPUS_SUPPORTED; c++){
		if (!CPU_ISSET(c, src)) CPU_SET(c, dst);
	}
#endif
	cpu_set_t ones;
	cpumask_all_cpus(&ones);
	CPU_XOR(dst, src, &ones);
}


state_t cpumask_get_processmask(cpu_set_t *dst, pid_t pid)
{
	// CPU_ZERO(dst);

	// We first get the affinity of the pid
	if (state_fail(sched_getaffinity(pid, sizeof(cpu_set_t), dst)))
	{
		return EAR_ERROR;
	}
#if SHOW_DEBUGS
	fprintf(stderr, "Thread %d: ", pid);
	verbose_affinity_mask(0, dst, MAX_CPUS_SUPPORTED);
#endif

	// Now we aggregate affinity mask for all its threads, if available
	char self_path[256];
 	snprintf(self_path, sizeof(self_path), "/proc/%d/task", pid);

	folder_t proc_task_dir;
	if (state_fail(folder_open(&proc_task_dir, self_path)))
	{
		return EAR_SUCCESS;
	}

	// Iterate across thread list
	char *tid_str;
	while ((tid_str = folder_getnextdir(&proc_task_dir, NULL, NULL)))
	{
		char *endptr;
		long tid = strtol(tid_str, &endptr, 10);
		debug("Next dir: %s, %ld", tid_str, tid);

		// filter invalid directories, i.e., `.` and `..`
		// and the process itself
		if (tid && (pid_t) tid != pid)
		{
			cpu_set_t thread_mask;
			if (!sched_getaffinity((pid_t) tid, sizeof(cpu_set_t), &thread_mask))
			{
				// Aggregate mask to dst
				cpumask_aggregate(dst, &thread_mask);

#if SHOW_DEBUGS
				fprintf(stderr, "Thread %d:" , (pid_t) tid);
				verbose_affinity_mask(0, &thread_mask, MAX_CPUS_SUPPORTED);
#endif
			}
		}
	}

#if SHOW_DEBUGS
	fprintf(stderr, "Final mask: ");
	verbose_affinity_mask(0, dst, MAX_CPUS_SUPPORTED);
#endif

	folder_close(&proc_task_dir);

	return EAR_SUCCESS;
}


void fill_cpufreq_list(cpu_set_t *my_mask, uint n_cpus, uint value, uint no_value, uint *cpu_list)
{
  if (my_mask == NULL || cpu_list == NULL || n_cpus < 1) return;
  for (uint c = 0; c < n_cpus; c++) {
    if (CPU_ISSET(c, my_mask)) {
      cpu_list[c] = value;
    }else{
      cpu_list[c] = no_value;
    }
  }
}


void print_affinity_mask(topology_t *topo) 
{
    cpu_set_t mask;
    int i;
    CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == -1) return;
    fprintf(stderr,"sched_getaffinity = ");
    for (i = 0; i < topo->cpu_count; i++) {
        fprintf(stderr,"CPU[%d]=%d ", i,CPU_ISSET(i, &mask));
    }
    fprintf(stderr,"\n");
}


state_t verbose_affinity_mask(int verb_lvl, const cpu_set_t *mask, uint cpus)
{
    if (mask) {

        if (VERB_ON(verb_lvl)) {

            verbosen(0, "mask affinity (n-1..0) =");

            for (int i = cpus-1; i >= 0; i -= 4) {
                verbosen(0, " %d%d%d%d", CPU_ISSET(i, mask), CPU_ISSET(i-1, mask),
                        CPU_ISSET(i-2, mask), CPU_ISSET(i-3, mask));
            }

            verbosen(0, "\n");

        }


    } else {
        return EAR_ERROR;
    }
    return EAR_SUCCESS;
}


state_t is_affinity_set(topology_t *topo, int pid, int *is_set, cpu_set_t *my_mask)
{
	cpu_set_t mask;
	*is_set=0;
	CPU_ZERO(&mask);
	CPU_ZERO(my_mask);
	if (sched_getaffinity(pid, sizeof(cpu_set_t), &mask) == -1) return EAR_ERROR;
	memcpy(my_mask,&mask,sizeof(cpu_set_t));
	int i;
	for (i = 0; i < topo->cpu_count; i++) {
		if (CPU_ISSET(i, &mask)) {
			*is_set=1;
			return EAR_SUCCESS;	
		}
	}
	return EAR_SUCCESS;
}


state_t set_affinity(int pid, cpu_set_t *mask)
{
	int ret;
	ret=sched_setaffinity(pid,sizeof(cpu_set_t),mask);
	if (ret <0 ) return EAR_ERROR;
	else return EAR_SUCCESS;
}

#if 0
state_t add_affinity(topology_t *topo, cpu_set_t *dest,cpu_set_t *src)
{
	state_t s = EAR_SUCCESS;
	if ((dest == NULL) || (src == NULL)) return EAR_ERROR;
	for (uint i = 0;i<topo->cpu_count; i++){
		if (CPU_ISSET(i,src)) CPU_SET(i,dest);
	}
	return s;	
}

static int file_is_accessible(const char *path)
{
	return (access(path, F_OK) == 0);
}

int cpu_isset(int cpu, cpu_set_t *mask)
{
    if (cpu < 0 || mask == NULL) {
        return -1;
    }
    return CPU_ISSET(cpu, mask);
}

#include <errno.h>

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("Usage: %s <PID>\n", program_invocation_name);
		return EXIT_FAILURE;
	}

	pid_t pid = (pid_t) atoi(argv[1]);
	cpu_set_t cpuset;

	if (state_ok(cpumask_get_processmask(&cpuset, pid)))
	{
		return EXIT_SUCCESS;
	} else
	{
		return EXIT_FAILURE;
	}
}
#endif
