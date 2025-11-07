/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _HW_INFO_H_
#define _HW_INFO_H_

#include <common/hardware/cpuid.h>
#include <common/hardware/topology.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/system/file.h>
#include <common/types/generic.h>
#include <sched.h>

/** Returns the number of packages detected. */
int detect_packages(int **package_map);

/** Sets all bits of `mask` to one. */
state_t cpumask_all_cpus(cpu_set_t *mask);

/** Computes the number of cpus in a given mask.
 * Returns -1 if \p my_mask is a NULL pointer. */
int cpumask_count(cpu_set_t *my_mask);

/** Fills \p cpu_list with the numbers of CPUs set in \p my_mask.
 * Returns 0 on success, -1 on error.
 * \param my_mask An allocated affinity mask pointer.
 * \param n_cpus The number of CPUs known to be in \p my_mask.
 * \param cpu_list The output list. This pointer must be allocated with \p n_cpus elements. */
int cpumask_getlist(cpu_set_t *my_mask, uint n_cpus, uint *cpu_list);

/** Add cpus set in src to dst. */
void cpumask_aggregate(cpu_set_t *dst, cpu_set_t *src);

/** Removes CPUs set in src from dst. */
void cpumask_remove(cpu_set_t *dst, cpu_set_t *src);

/** Sets in dst CPUs not in src. */
void cpumask_not(cpu_set_t *dst, cpu_set_t *src);

/** Computes the cpu_set mask of the process with PID \p pid and all its
 * threads. */
state_t cpumask_get_processmask(cpu_set_t *dst, pid_t pid);

/** Prints the affinity mask of the current process. */
void print_affinity_mask(topology_t *topo);

/** If the verbose level is greather or equal than \p verb_lvl, prints the affinity
 * mask stored at \p mask. CPUs from \p cpus - 1 to 0 will be printed. */
state_t verbose_affinity_mask(int verb_lvl, const cpu_set_t *mask, uint cpus);

/** Checks if process with pid=pid has some cpu forbiden to run , then is_set is set to 1 */
state_t is_affinity_set(topology_t *topo, int pid, int *is_set, cpu_set_t *my_mask);

#if 0
/** dest results from doing the OR between dest and src. cpu_count is used. */
state_t add_affinity(topology_t *topo, cpu_set_t *dest, cpu_set_t *src);
#endif

/** Returns whether \p cpu is in \p mask. -1 if an error occurred.
 * This function just wraps CPU_ISSET macro. */
int cpu_isset(int cpu, cpu_set_t *mask);

/* Fill cpu_list with value when cpu is set in my_mask and no_value when it's not */
void fill_cpufreq_list(cpu_set_t *my_mask, uint n_cpus, uint value, uint no_value, uint *cpu_list);
#endif
