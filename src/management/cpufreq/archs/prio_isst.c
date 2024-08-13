/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <metrics/common/isst.h>
#include <management/cpufreq/archs/prio_isst.h>

void mgt_prio_isst_load(topology_t *tp, mgt_prio_ops_t *ops)
{
    // Change by isst_test() one day
    if (state_fail(isst_init(tp))) {
        return;
    }
    apis_put(ops->get_api,            mgt_prio_isst_get_api);
    apis_put(ops->init,               mgt_prio_isst_init);
    apis_put(ops->dispose,            mgt_prio_isst_dispose);
    apis_put(ops->enable,             mgt_prio_isst_enable);
    apis_put(ops->disable,            mgt_prio_isst_disable);
    apis_put(ops->is_enabled,         mgt_prio_isst_is_enabled);
    apis_put(ops->get_available_list, mgt_prio_isst_get_available_list);
    apis_put(ops->set_available_list, mgt_prio_isst_set_available_list);
    apis_put(ops->get_current_list,   mgt_prio_isst_get_current_list);
    apis_put(ops->set_current_list,   mgt_prio_isst_set_current_list);
    apis_put(ops->set_current,        mgt_prio_isst_set_current);
    apis_put(ops->data_count,         mgt_prio_isst_data_count);
}

void mgt_prio_isst_get_api(uint *api)
{
    *api = API_ISST;
}

state_t mgt_prio_isst_init()
{
    return EAR_SUCCESS;
}

state_t mgt_prio_isst_dispose()
{
    return isst_dispose();
}

state_t mgt_prio_isst_enable()
{
    return isst_enable();
}

state_t mgt_prio_isst_disable()
{
    return isst_disable();
}

int mgt_prio_isst_is_enabled()
{
    return isst_is_enabled();
}

state_t mgt_prio_isst_get_available_list(cpuprio_t *prio_list)
{
    return isst_clos_get_list((clos_t *) prio_list);
}

state_t mgt_prio_isst_set_available_list(cpuprio_t *prio_list)
{
    return isst_clos_set_list((clos_t *) prio_list);
}

state_t mgt_prio_isst_get_current_list(uint *idx_list)
{
    return isst_assoc_get_list(idx_list);
}

state_t mgt_prio_isst_set_current_list(uint *idx_list)
{
    return isst_assoc_set_list(idx_list);
}

state_t mgt_prio_isst_set_current(uint idx, int cpu)
{
    return isst_assoc_set(idx, cpu);
}

void mgt_prio_isst_data_count(uint *prios_count, uint *idxs_count)
{
    isst_clos_count(prios_count);
    isst_assoc_count(idxs_count);
}
