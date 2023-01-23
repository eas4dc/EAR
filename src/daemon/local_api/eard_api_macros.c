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

#include <daemon/local_api/eard_api_fresh.h>

#define FUNC_S(name, call, type, suf) \
state_t name (type val, size_t size) \
{ \
    return eard_rpc(call, (char **) suf val, size, NULL, 0); \
}

#define FUNC_R(name, call, type, suf) \
state_t name (type val, size_t *size) \
{ \
    return eard_rpc_buffered(call, NULL, 0, (char **) val, size); \
}

FUNC_R(eard_cpufreq_get_api      , RPC_GET_API         ,     uint * ,  );
FUNC_R(eard_cpufreq_get_available, RPC_GET_AVAILABLE   , pstate_t **,  );
FUNC_R(eard_cpufreq_get_current  , RPC_GET_CURRENT     , pstate_t **,  );
FUNC_R(eard_cpufreq_get_nominal  , RPC_GET_NOMINAL     ,     uint *,   );
FUNC_R(eard_cpufreq_get_governor , RPC_GET_GOVERNOR    ,     uint * ,  );
FUNC_S(eard_cpufreq_set_current  , RPC_SET_CURRENT_LIST, pstate_t * ,  );
FUNC_S(eard_cpufreq_set_governor , RPC_SET_GOVERNOR    ,     uint   , &);

// In construct:
//  - mgt_cpufreq_eard_set_current