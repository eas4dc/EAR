/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef MANAGEMENT_PRIORITY_H
#define MANAGEMENT_PRIORITY_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>
#include <metrics/common/isst.h>

// PRIORITY is a subsystem of CPUFREQ. It can take the control over the
// frequency by using the enable function. Also, you can set the maximum
// and minimum frequency per priority.
//
// | Arch.     | Scope                                                            |
// |-----------|------------------------------------------------------------------|
// | Ice Lake  | Core, a change in a specific thread affects also to its sibling. |

#define PRIO_FREQ_MAX      ISST_MAX_FREQ // Max boosted frequency
#define PRIO_FREQ_MAX_NB  (PRIO_FREQ_MAX-1) // Max non-boosted frequency
#define PRIO_HIGH         (UINT_MAX-2)
#define PRIO_LOW          (UINT_MAX-3)
#define PRIO_SAME          ISST_SAME_PRIO // Do not change the priority, use the same

typedef struct cpuprio_s {
    ullong max_khz;
    ullong min_khz;
    uint high; // Current version only supports modify max and min khz.
    uint low;
    uint idx;
} cpuprio_t;

typedef struct mgt_prio_ops_s
{
    void    (*get_api) (uint *api);
    state_t (*init)    ();
    state_t (*dispose) ();
    state_t (*enable)  ();
    state_t (*disable) ();
    int     (*is_enabled) ();
    state_t (*get_available_list) (cpuprio_t *prio_list);
    state_t (*set_available_list) (cpuprio_t *prio_list);
    state_t (*get_current_list)   (uint *list_idx);
    state_t (*set_current_list)   (uint *list_idx);
    state_t (*set_current)        (uint idx, int cpu);
    void    (*data_count)         (uint *prio_count, uint *idx_count);
} mgt_prio_ops_t;

/* PRIORITY is a CPUFREQ subsystem, loading CPUFREQ also loads PRIORITY automatically. */
void mgt_cpufreq_prio_load(topology_t *tp, int eard);

void mgt_cpufreq_prio_get_api(uint *api);

state_t mgt_cpufreq_prio_init();

state_t mgt_cpufreq_prio_dispose();

/* Enables the PRIORITY subsystem, taking the control of the frequency. */
state_t mgt_cpufreq_prio_enable();

state_t mgt_cpufreq_prio_disable();

int mgt_cpufreq_prio_is_enabled();
/* Returns the list of priorities, ej: in intel CLOS0 to CLOS3. Its sorted from high priority to low. */
state_t mgt_cpufreq_prio_get_available_list(cpuprio_t *prio_list);
/* Edit priorities. By now just the min-max frequency fields of the cpuprio_t are modified. */
state_t mgt_cpufreq_prio_set_available_list(cpuprio_t *prio_list);
/* Sets the maximum and minimum frequency of the prio list. */
state_t mgt_cpufreq_prio_set_available_list_by_freqs(ullong *max_khz, ullong *min_khz);
/* Returns the list of the current CPU priorities, in index format (0 for CLOS0, 1 for CLOS1, and so on). */
state_t mgt_cpufreq_prio_get_current_list(uint *list_idx);
/* Changes CPUs priorities using a list of indexes. You can also use SAME_PRIO to avoid changes. */
state_t mgt_cpufreq_prio_set_current_list(uint *list_idx);
/* Sets a CPU to a specific priority CLOS. You can use also all_cpus. */
state_t mgt_cpufreq_prio_set_current(uint idx, int cpu);
/* Gets the number of priorities (ex: CLOS0 to CLOS3), and number of devices (CPUs in this case). */
void mgt_cpufreq_prio_data_count(uint *prio_count, uint *devs_count);
/* Allocating lists. */
void mgt_cpufreq_prio_data_alloc(cpuprio_t **prio_list, uint **idx_list);
    
void mgt_cpufreq_prio_data_print(cpuprio_t *prio_list, uint *idx_list, int fd);

void mgt_cpufreq_prio_data_tostr(cpuprio_t *prio_list, uint *idx_list, char *buffer, int length);

// Short names
#define mgt_cpuprio_load               mgt_cpufreq_prio_load
#define mgt_cpuprio_init               mgt_cpufreq_prio_init
#define mgt_cpuprio_dispose            mgt_cpufreq_prio_dispose
#define mgt_cpuprio_get_api            mgt_cpufreq_prio_get_api
#define mgt_cpuprio_count_devices(c,n) mgt_cpufreq_prio_data_count(NULL,n)
#define mgt_cpuprio_enable             mgt_cpufreq_prio_enable
#define mgt_cpuprio_disable            mgt_cpufreq_prio_disable
#define mgt_cpuprio_is_enabled         mgt_cpufreq_prio_is_enabled
#define mgt_cpuprio_get_available_list mgt_cpufreq_prio_get_available_list
#define mgt_cpuprio_set_available_list mgt_cpufreq_prio_set_available_list
#define mgt_cpuprio_get_current_list   mgt_cpufreq_prio_get_current_list
#define mgt_cpuprio_set_current_list   mgt_cpufreq_prio_set_current_list
#define mgt_cpuprio_set_current        mgt_cpufreq_prio_set_current
#define mgt_cpuprio_data_count         mgt_cpufreq_prio_data_count
#define mgt_cpuprio_data_alloc         mgt_cpufreq_prio_data_alloc
#define mgt_cpuprio_data_print         mgt_cpufreq_prio_data_print
#define mgt_cpuprio_data_tostr         mgt_cpufreq_prio_data_tostr 

#endif //MANAGEMENT_PRIORITY_H
