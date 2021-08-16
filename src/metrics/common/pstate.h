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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef METRICS_COMMON_PSTATE_H
#define METRICS_COMMON_PSTATE_H

#include <common/types.h>
#include <common/states.h>

#define all_devs   -1
#define ps_nothing (uint) -1
#define ps_auto    (uint) -2

typedef struct pstate_s {
	uint   idx; // P_STATE index
	ullong khz; // P_STATE frequency (in KHz)
} pstate_t;

#define MODE_ABOVE 1
#define MODE_BELOW 0

/* It requires descending sorted lists. */
state_t pstate_get_index(pstate_t *ps_list, uint ps_count, ullong khz, uint *index);

/* It requires descending sorted lists. Returns the closer P_STATE. */
void pstate_get_index_closer(pstate_t *ps_list, uint ps_count, ullong khz, uint *index, uint mode);

void pstate_print(pstate_t *pstate_list, uint pstate_count, int fd);

char *pstate_tostr(pstate_t *pstate_list, uint pstate_count, char *buffer, int length);

// Provisional
state_t avgfreq_to_pstate(pstate_t *pstate_list, uint pstate_count,ulong khz, pstate_t *p);

state_t pstate_to_freq(pstate_t *pstate_list, uint pstate_count,int pstate_idx, ulong *khz);

state_t from_freq_to_pstate(pstate_t *pstate_list, uint pstate_count,ulong khz, pstate_t *p);

#endif //METRICS_COMMON_PSTATE_H
