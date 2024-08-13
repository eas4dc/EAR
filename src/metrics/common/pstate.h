/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
