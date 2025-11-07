/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <management/cpufreq/archs/dummy.h>
#include <management/cpufreq/cpufreq_base.h>
#include <metrics/cpufreq/cpufreq_base.h>
#include <stdlib.h>

static uint cpu_count;
static cfb_t bf;

state_t mgt_cpufreq_dummy_load(topology_t *tp_in, mgt_ps_ops_t *ops)
{
    cpu_count = tp_in->cpu_count;
    // Getting base frequency and boost
    cpufreq_base_init(tp_in, &bf);
    //
    apis_put(ops->init, mgt_cpufreq_dummy_init);
    apis_put(ops->dispose, mgt_cpufreq_dummy_dispose);
    apis_put(ops->get_info, mgt_cpufreq_dummy_get_info);
    apis_put(ops->get_freq_details, mgt_cpufreq_dummy_get_freq_details);
    apis_put(ops->count_available, mgt_cpufreq_dummy_count_available);
    apis_put(ops->get_available_list, mgt_cpufreq_dummy_get_available_list);
    apis_put(ops->get_current_list, mgt_cpufreq_dummy_get_current_list);
    apis_put(ops->get_nominal, mgt_cpufreq_dummy_get_nominal);
    apis_put(ops->get_index, mgt_cpufreq_dummy_get_index);
    apis_put(ops->set_current_list, mgt_cpufreq_dummy_set_current_list);
    apis_put(ops->set_current, mgt_cpufreq_dummy_set_current);
    apis_put(ops->reset, mgt_cpufreq_dummy_reset);
    apis_put(ops->get_governor, mgt_cpufreq_dummy_governor_get);
    apis_put(ops->get_governor_list, mgt_cpufreq_dummy_governor_get_list);
    apis_put(ops->set_governor, mgt_cpufreq_dummy_governor_set);
    apis_put(ops->set_governor_mask, mgt_cpufreq_dummy_governor_set_mask);
    apis_put(ops->set_governor_list, mgt_cpufreq_dummy_governor_set_list);
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

void mgt_cpufreq_dummy_get_info(apinfo_t *info)
{
    info->api        = API_DUMMY;
    info->devs_count = 1;
}

void mgt_cpufreq_dummy_get_freq_details(freq_details_t *details)
{
    details->freq_base = bf.frequency;
    details->freq_max  = bf.frequency;
    details->freq_min  = bf.frequency;
}

state_t mgt_cpufreq_dummy_count_available(ctx_t *c, uint *pstate_count)
{
    *pstate_count = 1;
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_get_available_list(ctx_t *c, pstate_t *pstate_list)
{
    pstate_list[0].idx = 0;
    pstate_list[0].khz = bf.frequency;
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
    debug("mgt_cpufreq_dummy_get_current_list");
    int i;
    for (i = 0; i < cpu_count; ++i) {
        pstate_list[i].idx = 0;
        pstate_list[i].khz = bf.frequency;
    }
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_get_nominal(ctx_t *c, uint *pstate_index)
{
    *pstate_index = 0;
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest)
{
    *pstate_index = 0;
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_set_current_list(ctx_t *c, uint *pstate_index)
{
    debug("mgt_cpufreq_dummy_set_current_list");
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_set_current(ctx_t *c, uint pstate_index, int cpu)
{
    debug("mgt_cpufreq_dummy_set_current");
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_reset(ctx_t *c)
{
    debug("mgt_cpufreq_dummy_reset");
    return EAR_SUCCESS;
}

// Governors
state_t mgt_cpufreq_dummy_governor_get(ctx_t *c, uint *governor)
{
    debug("mgt_cpufreq_dummy_governor_get");
    *governor = Governor.userspace;
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_governor_get_list(ctx_t *c, uint *governors)
{
    debug("mgt_cpufreq_dummy_governor_get_list");
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_governor_set(ctx_t *c, uint governor)
{
    debug("mgt_cpufreq_dummy_governor_set");
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask)
{
    debug("mgt_cpufreq_dummy_governor_set_mask");
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_dummy_governor_set_list(ctx_t *c, uint *governors)
{
    debug("mgt_cpufreq_dummy_governor_set_list");
    return EAR_SUCCESS;
}

// ???
state_t mgt_cpufreq_dummy_data_print(pstate_t *pstate_list, uint pstate_count, int fd)
{
    int i;
    for (i = 0; i < pstate_count; ++i) {
        dprintf(fd, "CPU PS%d: id%d, %llu KHz\n", i, pstate_list[i].idx, pstate_list[i].khz);
    }
    return EAR_SUCCESS;
}
