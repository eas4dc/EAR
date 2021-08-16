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

#define SHOW_DEBUGS 0

#include <stdlib.h>
#include <metrics/common/msr.h>
#include <management/imcfreq/archs/intel63.h>

#define U_MSR_UNCORE_RATIO_LIMIT    0x000620
#define U_MSR_UNCORE_RL_MASK_MAX    0x00007F
#define U_MSR_UNCORE_RL_MASK_MIN    0x007F00
#define HUNDRED_MHZ                 100000LLU

static topology_t tp_static;
static pstate_t  *available_copy;
static pstate_t  *available_list;
static uint       available_count;
static ullong     max_khz;
static ullong     min_khz;

static state_t intel63_read(uint cpu, ullong *max, ullong *min)
{
	off_t address = U_MSR_UNCORE_RATIO_LIMIT;
	ullong aux = 0;
	state_t s;

	*max = 0;
	*min = 0;
	debug("Reading MSR for CPU %d", cpu);
	if ((s = msr_read(cpu, &aux, sizeof(uint64_t), address)) != EAR_SUCCESS) {
		return s;
	}
	*max = (aux & U_MSR_UNCORE_RL_MASK_MAX) >> 0;
	*min = (aux & U_MSR_UNCORE_RL_MASK_MIN) >> 8;
	*max = *max * HUNDRED_MHZ;
	*min = *min * HUNDRED_MHZ;
	debug("Read values: %llu, min_khz: %llu", *max, *min);

	return EAR_SUCCESS;
}

static state_t mgt_imcfreq_intel63_build_list()
{
	ullong max_khz_aux = 0;
	ullong min_khz_aux = 0;
	ullong aux = 0;
	state_t s;
	int i;

	// First scan, getting the maximum and the minimum.
	for (i = 0; i < tp_static.cpu_count; ++i) {
		// Opening
		if (state_fail(s = msr_open(tp_static.cpus[i].id))) {
			return s;
		}
		// Reading
		if (state_fail(s = intel63_read(tp_static.cpus[i].id, &max_khz_aux, &min_khz_aux))) {
			debug("Failed during IMC read: %s (%d)", state_msg, s);
		}
		// Saving general maximum and minimum
		if (max_khz == 0 || max_khz_aux > max_khz) {
			max_khz = max_khz_aux;
		}
		if ((min_khz == 0 || min_khz_aux < min_khz) && min_khz_aux != 0) {
			min_khz = min_khz_aux;
		}
	}
	if (max_khz == 0 || min_khz == 0) {
		return_msg(EAR_ERROR, "Can't build a frequency list");
	}
	// Hardcoding the values
	max_khz = 24 * HUNDRED_MHZ;
	min_khz = 12 * HUNDRED_MHZ;
	// Building the list
	available_count = (max_khz / HUNDRED_MHZ) - (min_khz / HUNDRED_MHZ) + 1;
	available_list = calloc(available_count, sizeof(pstate_t));
	available_copy = calloc(available_count, sizeof(pstate_t));

	for (i = 0, aux = 0LLU; i < available_count; ++i, aux += HUNDRED_MHZ) {
		available_list[i].idx = i;
		available_list[i].khz = max_khz - aux;
	}

	return EAR_SUCCESS;
}

state_t mgt_imcfreq_intel63_load(topology_t *tp, mgt_imcfreq_ops_t *ops)
{
	state_t s;
	if (max_khz != 0) {
		return EAR_SUCCESS;
	}
	if (tp->vendor != VENDOR_INTEL || tp->model < MODEL_HASWELL_X) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	if (state_fail(s = msr_test(tp))) {
		return s;
	}
	if (state_fail(s = topology_select(tp, &tp_static, TPSelect.socket, TPGroup.merge, 0))) {
		return s;
	}
	if (state_fail(s = mgt_imcfreq_intel63_build_list())) {
		return s;
	}
	replace_ops(ops->init,               mgt_imcfreq_intel63_init);
	replace_ops(ops->dispose,            mgt_imcfreq_intel63_dispose);
	replace_ops(ops->get_available_list, mgt_imcfreq_intel63_get_available_list);
	replace_ops(ops->get_current_list,   mgt_imcfreq_intel63_get_current_list);
	replace_ops(ops->set_current_list,   mgt_imcfreq_intel63_set_current_list);
	replace_ops(ops->set_current,        mgt_imcfreq_intel63_set_current);
	replace_ops(ops->set_auto,           mgt_imcfreq_intel63_set_auto);
	replace_ops(ops->get_current_ranged_list, mgt_imcfreq_intel63_get_current_ranged_list);
	replace_ops(ops->set_current_ranged_list, mgt_imcfreq_intel63_set_current_ranged_list);
	return s;
}

state_t mgt_imcfreq_intel63_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_intel63_dispose(ctx_t *c)
{
    c->context = NULL;
    return EAR_SUCCESS;
}

state_t mgt_imcfreq_intel63_get_available_list(ctx_t *c, const pstate_t **list, uint *count)
{
	uint i;
	for (i = 0; i < available_count; ++i) {
		available_copy[i].khz = available_list[i].khz;
		available_copy[i].idx = i;
	}
	*list = available_copy;
	if (count != NULL) {
		*count = available_count;
	}
	return EAR_SUCCESS;
}

static uint khz_to_idx(ullong khz)
{
	int i;
	for (i = 0; i < available_count; ++i) {
		if (available_list[i].khz <= khz) {
			return i;
		}
	}
	return 0;
}

state_t get_current_list(pstate_t *max_list, pstate_t *min_list)
{
	ullong max_khz_aux = 0;
	ullong min_khz_aux = 0;
	state_t s;
	int i;

	for (i = 0; i < tp_static.cpu_count; ++i)
	{
		max_khz_aux = 0;
		min_khz_aux = 0;

		if (state_fail(s = intel63_read(tp_static.cpus[i].id, &max_khz_aux, &min_khz_aux))) {
			return s;
		}
		if (max_list != NULL) {	
			max_list[i].idx = khz_to_idx(max_khz_aux);
			max_list[i].khz = available_list[max_list[i].idx].khz;
			debug("DEV%d read MAX PS%u (%llu KHz)", i, max_list[i].idx, max_list[i].khz);
		}
		if (min_list != NULL) {
			min_list[i].idx = khz_to_idx(min_khz_aux);
			min_list[i].khz = available_list[min_list[i].idx].khz;
			debug("DEV%d read MIN PS%u (%llu KHz)", i, min_list[i].idx, min_list[i].khz);
		}
	}
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_intel63_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	return get_current_list(pstate_list, NULL);
}

static int read_required(uint *max_list, uint *min_list)
{
	int i;

	if (max_list == NULL || min_list == NULL) {
		return 1;
	}
	for (i = 0; i < tp_static.cpu_count; ++i) {
		if (max_list[i] == ps_nothing || min_list[i] == ps_nothing) {
			return 1;
		}
	}

	return 0;
}

// Internaly, max is top frequency and min is bottom frequency.
static state_t set_current_list(uint *max_list, uint *min_list)
{
	off_t address = U_MSR_UNCORE_RATIO_LIMIT;
	pstate_t buffer_max[128];
	pstate_t buffer_min[128];
	ullong set0 = 0;
	ullong set1 = 0;
	ullong aux_max;
	ullong aux_min;
	int max_valid;
	int min_valid;
	state_t r;
	int i;

	// if read is required
	if (read_required(max_list, min_list)) {
		if (state_fail(r = get_current_list(buffer_max, buffer_min))) {
			return r;
		}
	}
	for (i = 0; i < tp_static.cpu_count; ++i)
	{
		debug("-- Device %d -------------------------------", i);
		debug("Max/min target : %u/%u", max_list[i], min_list[i]);
		debug("Max/min current: %u/%u", buffer_max[i].khz, buffer_min[i].khz);
		// Valid ranks
		if (max_list != NULL) max_valid = (max_list[i] < available_count);  
		if (min_list != NULL) min_valid = (min_list[i] < available_count);  
		// By default, if ps_auto or not valid 
		aux_max = max_khz;
		aux_min = min_khz;
		// Else
		if      (max_list    == NULL)       aux_max = buffer_max[i].khz;
		else if (max_list[i] == ps_nothing) aux_max = buffer_max[i].khz;
		else if (max_valid)                 aux_max = available_list[max_list[i]].khz;
		if      (min_list    == NULL)       aux_min = buffer_min[i].khz;
		else if (min_list[i] == ps_nothing) aux_min = buffer_min[i].khz;
		else if (min_valid)                 aux_min = available_list[min_list[i]].khz;
		// Converting to MHZ
		aux_max = aux_max / HUNDRED_MHZ;
		aux_min = aux_min / HUNDRED_MHZ;
		// Combining both values in the register
		debug("DEV%d write MAX %llu KHz", i, aux_max);
		debug("DEV%d write MIN %llu KHz", i, aux_min);
		set0 = (aux_min << 8) & U_MSR_UNCORE_RL_MASK_MIN;
		set1 = (aux_max << 0) & U_MSR_UNCORE_RL_MASK_MAX;
		set0 = set0 | set1;
		//
		if ((r = msr_write(tp_static.cpus[i].id, &set0, sizeof(ullong), address)) != EAR_SUCCESS) {
			return r;
		}
	}

	return EAR_SUCCESS;
}

state_t mgt_imcfreq_intel63_set_current_list(ctx_t *c, uint *index_list)
{
	return set_current_list(index_list, NULL);
}

state_t mgt_imcfreq_intel63_set_current(ctx_t *c, uint pstate_index, int socket)
{
    uint buffer[128];
	int i;
	for (i = 0; i < tp_static.cpu_count; ++i) {
		buffer[i] = ps_nothing;
		if (i != socket && socket != all_sockets) {
			continue;
		}
		buffer[i] = pstate_index;
	}
	return set_current_list(buffer, NULL);
}

state_t mgt_imcfreq_intel63_set_auto(ctx_t *c)
{
	return mgt_imcfreq_intel63_set_current(c, ps_auto, all_sockets);
}

state_t mgt_imcfreq_intel63_get_current_ranged_list(ctx_t *c, pstate_t *ps_min_list, pstate_t *ps_max_list)
{
	return get_current_list(ps_min_list, ps_max_list);
}

state_t mgt_imcfreq_intel63_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list)
{
	return set_current_list(id_min_list, id_max_list);
}
