/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <metrics/common/msr.h>
#include <common/output/debug.h>
#include <common/utils/keeper.h>
#include <common/utils/stress.h>
#include <common/math_operations.h>
#include <metrics/imcfreq/archs/intel63.h>
#include <management/imcfreq/archs/intel63.h>

#define U_MSR_UNCORE_RATIO_LIMIT    0x000620
#define U_MSR_UNCORE_RL_MASK_MAX    0x00007F
#define U_MSR_UNCORE_RL_MASK_MIN    0x007F00
#define HUNDRED_MHZ                 100000LLU

static topology_t tp_static;
static pstate_t  *available_copy;
static pstate_t  *available_list;
static uint       available_count;
static ullong     max_khz_dyn;
static ullong     max_khz;
static ullong     min_khz;

static state_t intel63_write(uint cpu, ullong max, ullong min)
{
	off_t address = U_MSR_UNCORE_RATIO_LIMIT;
	ullong set0 = 0;
	ullong set1 = 0;
        state_t s;
		
	max = max / HUNDRED_MHZ;
	min = min / HUNDRED_MHZ;
	// Combining both values in the register
	set0 = (min << 8) & U_MSR_UNCORE_RL_MASK_MIN;
	set1 = (max << 0) & U_MSR_UNCORE_RL_MASK_MAX;
	set0 = set0 | set1;
	debug("IMC%d written value: 0x%llx", cpu, set0);
	if ((s = msr_write(cpu, &set0, sizeof(ullong), address)) != EAR_SUCCESS) {
		return s;
	}
	return EAR_SUCCESS;
}

static state_t intel63_read(uint cpu, ullong *max, ullong *min)
{
	off_t address = U_MSR_UNCORE_RATIO_LIMIT;
	ullong aux = 0;
	state_t s;

	*max = 0;
	*min = 0;
	if ((s = msr_read(cpu, &aux, sizeof(uint64_t), address)) != EAR_SUCCESS) {
		return s;
	}
	*max = (aux & U_MSR_UNCORE_RL_MASK_MAX) >> 0;
	*min = (aux & U_MSR_UNCORE_RL_MASK_MIN) >> 8;
	*max = *max * HUNDRED_MHZ;
	*min = *min * HUNDRED_MHZ;
	debug("IMC%d read values: %llu - %llu MHz", cpu, *max, *min);
	return EAR_SUCCESS;
}

static void *calculate_frequency(void *arg)
{
    ullong freq2, freq1;
    state_t s;
    
    *((ullong *) arg) = 0;
    // Reading content
    if (state_fail(s = imcfreq_intel63_ext_read_cpu(0, (ulong *) &freq1))) {
        return NULL;
    }
    // Spinning during 0,5 seconds
    stress_spin(499LLU);
    // Reading the content again
    if (state_fail(s = imcfreq_intel63_ext_read_cpu(0, (ulong *) &freq2))) {
        return NULL;
    }
    // Mult per 2 to convert 0,5 seconds to 1,0 seconds
    freq2 = overflow_magic_u64(freq2, freq1, MAXBITS48) * 2LLU;
    freq2 = ceil_magic_u64(freq2, 9) / (ullong) 1E3;
    // Returning the data
    *((ullong *) arg) = freq2;
 
    return NULL;
}

static state_t dynamic_clean(state_t s, char *msg)
{
    intel63_write(0, max_khz, min_khz);
    return_msg(s, msg);
}

static state_t dynamic_detection(topology_t *tp, ullong *freq_khz)
{
    pthread_t thread1;
    pthread_t thread2;
    ullong freq1;
    ullong freq2;
    ullong freq3;
    state_t s;
    int err;

    // Opening MSR
    if (state_fail(s = imcfreq_intel63_ext_load_addresses(tp))) {
        return s;
    }
    if (state_fail(s = imcfreq_intel63_ext_enable_cpu(0))) {
        return s;
    }
    // Hacking frequency (absurdly high frequency for both min and max)
    if (state_fail(s = intel63_write(0, 6300000, 6300000))) {
        return s;
    }
    // Creating threads
    if ((err = pthread_create(&thread1, NULL, calculate_frequency, (void *) &freq1)) != 0) {
        return dynamic_clean(EAR_ERROR, strerror(err));
    }
    if ((err = pthread_create(&thread2, NULL, calculate_frequency, (void *) &freq2)) != 0) {
        return dynamic_clean(EAR_ERROR, strerror(err));
    }
    calculate_frequency((void *) &freq3);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    // Selecting a frequency among the 3 detected frequencies
    debug("Detected max IMC frequency: %llu %llu %llu KHz", freq1, freq2, freq3);
    if ((freq1 == freq2 || freq1 == freq3) && freq1 != 0LLU) {
        *freq_khz = freq1;
    } else if (freq2 == freq3 && freq2 != 0LLU) {
        *freq_khz = freq2;
    } else if (freq3 != 0LLU) {
        *freq_khz = freq3;
    } else {
        return dynamic_clean(EAR_ERROR, "Bad frequency");
    }
    return dynamic_clean(EAR_SUCCESS, "");
}

static state_t mgt_imcfreq_intel63_build_list(topology_t *tp, int eard, my_node_conf_t *conf)
{
    ullong aux = 0;
    state_t s;
    int i;

    // [1] scan, getting the maximum and the minimum.
    for (i = 0; i < tp_static.cpu_count; ++i) {
        // Opening
        if (state_fail(s = msr_open(tp_static.cpus[i].id, MSR_WR))) {
            return s;
        }
        // Reading
        if (state_fail(s = intel63_read(tp_static.cpus[i].id, &max_khz, &min_khz))) {
            debug("Failed during IMC read: %s (%d)", state_msg, s);
        }
    }
    // [2.1] Saving what found in MAX FREQ
    if (!keeper_load_uint64("ImcFreqMax", &max_khz)) {
        if (max_khz > 0) {
            keeper_save_uint64("ImcFreqMax", max_khz);
            debug("Keeper saved ImcFreqMax %llu", max_khz);
        }
    } else {
        debug("Keeper read ImcFreqMax %llu", max_khz);
    }
    // [2.2] Saving what found in MIN FREQ
    if (!keeper_load_uint64("ImcFreqMin", &min_khz)) {
        if (min_khz > 0) {
            keeper_save_uint64("ImcFreqMin", min_khz);
            debug("Saved ImcFreqMin %llu", min_khz);
        }
    } else {
        debug("Keeper read ImcFreqMin %llu", min_khz);
    }
    // [2.3] Saving what found in MAX FREQ DYN
    //     The procedure is the following:
    //         1) Set an absurdly high IMC frequency in socket 0
    //         2) Calculate the frequency by consulting UCLK in 3 threads (1+2)
    //            during 0.5 seconds.
    //         3) Check if the 3 thread match to assume its correct.
    if (!keeper_load_uint64("ImcFreqMaxDyn", &max_khz_dyn)) {
        if (state_fail(s = dynamic_detection(tp, &max_khz_dyn))) {
            debug("dynamic_detection() returned: %s", state_msg);
        } else {
            debug("Dynamic detection found: %llu KHz (and saved)", max_khz_dyn);
            keeper_save_uint64("ImcFreqMaxDyn", max_khz_dyn);
        }
    } else {
        debug("Keeper read ImcFreqMaxDyn %llu", max_khz_dyn);
    }
    // [3] Letting DYN to build the list.
    //    If maximum frequency is below 1 GHz or over 3 GHz, we can
    //    conclude that the IMC frequency is not correct. Then we check
    //    if the maximum dynamic frequency could be a replacement.
    if (max_khz <= 1000000LLU || max_khz >= 3000000LLU) {
        if (max_khz_dyn > 1000000LLU && max_khz_dyn < 3000000LLU) {
            max_khz = max_khz_dyn; 
        } else {
            return_msg(EAR_ERROR, "Can't build a frequency list");
	}
    }
    if (min_khz == max_khz || min_khz > max_khz || min_khz == 0LLU) {
        min_khz = 800000LLU;
    }
    // [4] Printing final data
    debug("Final MAX %llu KHz", max_khz);
    debug("Final MIN %llu KHz", min_khz);
    debug("Final DYN %llu KHz", max_khz_dyn);
    // Building the list
    available_count = (max_khz / HUNDRED_MHZ) - (min_khz / HUNDRED_MHZ) + 1;
    available_list = calloc(available_count, sizeof(pstate_t));
    available_copy = calloc(available_count, sizeof(pstate_t));

    for (i = 0, aux = 0LLU; i < available_count; ++i, aux += HUNDRED_MHZ) {
        available_list[i].idx = i;
        available_list[i].khz = max_khz - aux;
        debug("PS%d: %llu KHz", i, available_list[i].khz);
    }
    return EAR_SUCCESS;
}

state_t mgt_imcfreq_intel63_load(topology_t *tp, mgt_imcfreq_ops_t *ops, int eard, my_node_conf_t *conf)
{
	state_t s;
	if (max_khz != 0) {
		return EAR_SUCCESS;
	}
	if (tp->vendor != VENDOR_INTEL || tp->model < MODEL_HASWELL_X) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	if (state_fail(s = msr_test(tp, MSR_WR))) {
		return s;
	}
	if (state_fail(s = topology_select(tp, &tp_static, TPSelect.socket, TPGroup.merge, 0))) {
		return s;
	}
	if (state_fail(s = mgt_imcfreq_intel63_build_list(tp, eard, conf))) {
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
			debug("IMC%d copied MAX PS%u (%llu KHz)", i, max_list[i].idx, max_list[i].khz);
		}
		if (min_list != NULL) {
			min_list[i].idx = khz_to_idx(min_khz_aux);
			min_list[i].khz = available_list[min_list[i].idx].khz;
			debug("IMC%d copied MIN PS%u (%llu KHz)", i, min_list[i].idx, min_list[i].khz);
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
	pstate_t buffer_max[128];
	pstate_t buffer_min[128];
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
		debug("IMC%d, setting frequency to (%llu, %llu)", i, aux_max, aux_min);
		if ((r = intel63_write(tp_static.cpus[i].id, aux_max, aux_min))) {
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
    #if SHOW_DEBUGS
    int i;
    for (i = 0; i < tp_static.cpu_count; ++i) {
        debug("IMC%d: setting frequency from %u to %u",
            i, id_min_list[i], id_max_list[i]);
    }
    #endif
    return set_current_list(id_min_list, id_max_list);
}
