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

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/utils/serial_buffer.h>
#include <daemon/local_api/eard_api_rpc.h>
#include <management/cpufreq/archs/prio_eard.h>

static uint prios_count;
static uint devs_count;
static uint eard_api;

void mgt_prio_eard_load(topology_t *tp, mgt_prio_ops_t *ops, int eard)
{
    char *buffer;
    size_t size;
    state_t s;

    if (!eard) {
        debug("EARD (daemon) not required");
        return;
    }
    if (apis_loaded(ops)) {
        debug("Other API already loaded");
        return;
    }
    if (!eards_connected()) { 
        debug("EARD (daemon) not connected");
        return;
    }
    debug("EARD (daemon) connected");
    if (state_fail(s = eard_rpc_buffered(RPC_MGT_PRIO_GET_STATIC_VALUES, NULL, 0, &buffer, &size))) {
        return;
    }
    serial_copy_elem((wide_buffer_t *) &buffer, (char *) &eard_api, NULL);
    serial_copy_elem((wide_buffer_t *) &buffer, (char *) &prios_count, NULL);
    serial_copy_elem((wide_buffer_t *) &buffer, (char *) &devs_count, NULL);
    debug("EARD api   : %d", eard_api);    
    debug("EARD #devs : %d", devs_count);
    debug("EARD #prios: %d", prios_count);
    if (eard_api == API_NONE || eard_api == API_DUMMY) {
        debug("Detected DUMMY API");
        return;
    }
    debug("EARD API loaded");
    apis_put(ops->get_api,       mgt_prio_eard_get_api);
    apis_put(ops->init,          mgt_prio_eard_init);
    apis_put(ops->dispose,       mgt_prio_eard_dispose);
    apis_put(ops->enable,        mgt_prio_eard_enable);
    apis_put(ops->disable,       mgt_prio_eard_disable);
    apis_put(ops->is_enabled,    mgt_prio_eard_is_enabled);
    apis_put(ops->get_available_list, mgt_prio_eard_get_available_list);
    apis_put(ops->set_available_list, mgt_prio_eard_set_available_list);
    apis_put(ops->get_current_list,   mgt_prio_eard_get_current_list);
    apis_put(ops->set_current_list,   mgt_prio_eard_set_current_list);
    apis_put(ops->set_current,   mgt_prio_eard_set_current);
    apis_put(ops->data_count,    mgt_prio_eard_data_count);
}

void mgt_prio_eard_get_api(uint *api)
{
    *api = API_EARD;
}

state_t mgt_prio_eard_init()
{
    return EAR_SUCCESS;
}

state_t mgt_prio_eard_dispose()
{
    return EAR_SUCCESS;
}

state_t mgt_prio_eard_enable()
{
    return eard_rpc(RPC_MGT_PRIO_ENABLE, NULL, 0, NULL, 0);
}

state_t mgt_prio_eard_disable()
{
    return eard_rpc(RPC_MGT_PRIO_DISABLE, NULL, 0, NULL, 0);
}

int mgt_prio_eard_is_enabled()
{
    int enabled = 0;
    eard_rpc(RPC_MGT_PRIO_IS_ENABLED, NULL, 0, (char *) &enabled, sizeof(int));
    return enabled;
}

#define SIZE_PRIOS sizeof(cpuprio_t)*prios_count
#define SIZE_DEVS  sizeof(uint)*devs_count

state_t mgt_prio_eard_get_available_list(cpuprio_t *prio_list)
{
    memset((void *) prio_list, 0, SIZE_PRIOS);
    return eard_rpc(RPC_MGT_PRIO_GET_AVAILABLE, NULL, 0, (char *) prio_list, SIZE_PRIOS);
}

state_t mgt_prio_eard_set_available_list(cpuprio_t *prio_list)
{
    return eard_rpc(RPC_MGT_PRIO_SET_AVAILABLE, (char *) prio_list, SIZE_PRIOS, NULL, 0);
}

state_t mgt_prio_eard_get_current_list(uint *idx_list)
{
    memset((void *) idx_list, 0, SIZE_DEVS);
    return eard_rpc(RPC_MGT_PRIO_GET_CURRENT, NULL, 0, (char *) idx_list, SIZE_DEVS);
}

state_t mgt_prio_eard_set_current_list(uint *idx_list)
{
    return eard_rpc(RPC_MGT_PRIO_SET_CURRENT_LIST, (char *) idx_list, SIZE_DEVS, NULL, 0);
}

state_t mgt_prio_eard_set_current(uint idx, int cpu)
{
    struct data_s { uint idx; int cpu; } data = {.idx = idx, .cpu = cpu};
    return eard_rpc(RPC_MGT_PRIO_SET_CURRENT, (char *) &data, sizeof(data), NULL, 0);
}

void mgt_prio_eard_data_count(uint *prios_count_in, uint *idxs_count_in)
{
    if (prios_count_in != NULL) {
        *prios_count_in = prios_count;       
    }
    if (idxs_count_in != NULL) {
        *idxs_count_in = devs_count;
    }
}
