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

#ifndef METRICS_COMMON_PSTATE_H
#define METRICS_COMMON_PSTATE_H

#define _GNU_SOURCE
#include <sched.h>
#include <common/types.h>
#include <common/states.h>

#define ps_nothing (uint) -1
#define ps_auto    (uint) -2

typedef struct pstate_s {
	uint   idx; // P_STATE index
	ullong khz; // P_STATE frequency (in KHz)
} pstate_t;

#define MODE_UPPER 1
#define MODE_LOWER 0

state_t pstate_pstofreq(pstate_t *ps_list, uint ps_count, uint ps_idx, ullong *khz_ret);

state_t pstate_freqtops(pstate_t *ps_list, uint ps_count, ullong ps_khz, pstate_t *ps_ret);

state_t pstate_freqtops_upper(pstate_t *ps_list, uint ps_count, ulong ps_khz, pstate_t *ps_ret);

void pstate_freqtoavg(cpu_set_t mask, ulong *freq_list, uint freq_count, ulong *freq_avg, ulong *cpus_counted);

/* It requires descending sorted lists. Returns the closer P_STATE. */
void pstate_freqtoidx_nearest(pstate_t *ps_list, uint ps_count, ullong ps_khz, uint *idx_ret, uint mode);

void pstate_print(pstate_t *pstate_list, uint pstate_count, int fd);

char *pstate_tostr(pstate_t *pstate_list, uint pstate_count, char *buffer, int length);

#endif //METRICS_COMMON_PSTATE_H
