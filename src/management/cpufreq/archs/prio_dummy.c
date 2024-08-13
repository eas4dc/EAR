/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdlib.h>
#include <common/output/debug.h>
#include <metrics/common/isst.h>
#include <management/cpufreq/cpufreq.h>
#include <management/cpufreq/archs/prio_dummy.h>

#define N_PRIOS 1

static cpuprio_t       prio_list[N_PRIOS];
static uint           *assoc_list;
static uint            cpus_count;
static int             enabled;
#if 0
static const pstate_t *ps_list;
static uint            ps_count;
#endif

void mgt_prio_dummy_load(topology_t *tp, mgt_prio_ops_t *ops)
{
    cpus_count = tp->cpu_count;
    // Filling the only slot 
    prio_list[0].max_khz = PRIO_FREQ_MAX;
    prio_list[0].min_khz = 0LLU;
    prio_list[0].high    = 1;
    prio_list[0].low     = 0;
    prio_list[0].idx     = 0;
    #if 0
    // Dummy priorities
    if (state_ok(mgt_cpufreq_get_available_list(no_ctx, &ps_list, &ps_count))) {
        avail.min_khz = ps_list[ps_count-1].khz;
    }
    #endif
    assoc_list = calloc(cpus_count, sizeof(uint));
    // Ops dummified
    apis_put(ops->get_api,       mgt_prio_dummy_get_api);
    apis_put(ops->init,          mgt_prio_dummy_init);
    apis_put(ops->dispose,       mgt_prio_dummy_dispose);
    apis_put(ops->enable,        mgt_prio_dummy_enable);
    apis_put(ops->disable,       mgt_prio_dummy_disable);
    apis_put(ops->is_enabled,    mgt_prio_dummy_is_enabled);
    apis_put(ops->get_available_list, mgt_prio_dummy_get_available_list);
    apis_put(ops->set_available_list, mgt_prio_dummy_set_available_list);
    apis_put(ops->get_current_list,   mgt_prio_dummy_get_current_list);
    apis_put(ops->set_current_list,   mgt_prio_dummy_set_current_list);
    apis_put(ops->set_current,   mgt_prio_dummy_set_current);
    apis_put(ops->data_count,    mgt_prio_dummy_data_count);
}

void mgt_prio_dummy_get_api(uint *api)
{
    *api = API_DUMMY;
}

state_t mgt_prio_dummy_init()
{
    return EAR_SUCCESS;
}

state_t mgt_prio_dummy_dispose()
{
    return EAR_SUCCESS;
}

state_t mgt_prio_dummy_enable()
{
    enabled = 1;
    return EAR_SUCCESS;
}

state_t mgt_prio_dummy_disable()
{
    enabled = 0;
    return EAR_SUCCESS;
}

int mgt_prio_dummy_is_enabled()
{
    return enabled;
}

state_t mgt_prio_dummy_get_available_list(cpuprio_t *prio_list_in)
{
    memcpy(prio_list_in, prio_list, sizeof(cpuprio_t) * N_PRIOS); 
    return EAR_SUCCESS;
}

state_t mgt_prio_dummy_set_available_list(cpuprio_t *prio_list)
{
    return EAR_SUCCESS;
}

state_t mgt_prio_dummy_get_current_list(uint *idx_list)
{
    memset((void *) idx_list, 0, sizeof(uint)*cpus_count);
    return EAR_SUCCESS;
}

state_t mgt_prio_dummy_set_current_list(uint *idx_list)
{
    return EAR_SUCCESS;
}

state_t mgt_prio_dummy_set_current(uint idx, int cpu)
{
    return EAR_SUCCESS;
}

void mgt_prio_dummy_data_count(uint *prios_count, uint *idxs_count)
{
    if (prios_count != NULL)  *prios_count = N_PRIOS;
    if (idxs_count  != NULL)  *idxs_count  = cpus_count;
}
