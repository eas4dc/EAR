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

#include <metrics/common/pstate.h>

state_t pstate_pstofreq(pstate_t *ps_list, uint ps_count, uint ps_idx, ullong *khz_ret)
{
    if (ps_idx >= ps_count) {
        return_msg(EAR_ERROR, Generr.not_found);
    }
    *khz_ret = ps_list[ps_idx].khz;
    return EAR_SUCCESS;
}

state_t pstate_freqtops(pstate_t *ps_list, uint ps_count, ullong ps_khz, pstate_t *ps_ret)
{
    int i;
    for (i = 0; i < ps_count; ++i) {
        if (ps_list[i].khz == ps_khz) {
            memcpy(ps_ret, &ps_list[i], sizeof(pstate_t));
            return EAR_SUCCESS;
        }
    }
    return_msg(EAR_ERROR, Generr.not_found);
}

state_t pstate_freqtops_upper(pstate_t *ps_list, uint ps_count, ulong ps_khz, pstate_t *ps_ret)
{
    int i;
    if (ps_list[0].khz < ps_khz) {
        memcpy(ps_ret, &ps_list[0], sizeof(pstate_t));
        return_msg(EAR_ERROR, Generr.not_found);
    }
    for (i = 1; i < ps_count; ++i) {
        if (ps_list[i].khz < ps_khz){
            memcpy(ps_ret, &ps_list[i-1], sizeof(pstate_t));
            return EAR_SUCCESS;
        }
    }
    memcpy(ps_ret, &ps_list[ps_count-1], sizeof(pstate_t));
    return_msg(EAR_ERROR, Generr.not_found);
}

void pstate_freqtoidx_nearest(pstate_t *ps_list, uint ps_count, ullong ps_khz, uint *idx_ret, uint mode)
{
	int i;

	if (mode == MODE_UPPER && ps_list[0].khz <= ps_khz) {
		*idx_ret = 0;
	}
	for (i = 0; i < ps_count; ++i) {
		if (mode == MODE_UPPER && ps_list[i].khz <= ps_khz) {
			*idx_ret = i-1;
		}
		if (mode == MODE_LOWER && ps_list[i].khz < ps_khz) {
			*idx_ret = i;
		}
		if (ps_list[i].khz == ps_khz) {
			*idx_ret = i;
		}
	}
	*idx_ret = ps_count-1;
}

void pstate_freqtoavg(cpu_set_t mask, ulong *freq_list, uint freq_count, ulong *freq_avg, ulong *cpus_count)
{
    ulong total_cpus = 0;
    ulong total_freq = 0;
    int c;

    if ((freq_list == NULL) || (freq_avg == NULL) || (cpus_count == NULL)) {
        return;
    }
    *freq_avg = 0;
    for (c = 0 ; c < freq_count; c++){
        if (CPU_ISSET(c, &mask)){
            total_freq += freq_list[c];
            total_cpus++;
        }
    }
    if (total_cpus > 0) {
        *freq_avg = total_freq / total_cpus;
    }
    *cpus_count = total_cpus;
}

void pstate_print(pstate_t *pstate_list, uint pstate_count, int fd)
{
	char buffer[SZ_BUFFER];
	dprintf(fd, "%s", pstate_tostr(pstate_list, pstate_count, buffer, SZ_BUFFER));
}

char *pstate_tostr(pstate_t *pstate_list, uint pstate_count, char *buffer, int length)
{
	int already = 0;
	int printed;
	int i;

	for (i = 0; i < pstate_count && length > 0; ++i) {
		printed = snprintf(&buffer[already], length, "PS%d: id%d, %llu KHz\n",
						i, pstate_list[i].idx, pstate_list[i].khz);
		length  = length  - printed;
		already = already + printed;
	}
	return buffer;
}
