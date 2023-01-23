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
#include <metrics/cpufreq/archs/eard.h>
#include <daemon/local_api/eard_api_rpc.h>

static uint cpu_count;

state_t cpufreq_eard_status(topology_t *tp, cpufreq_ops_t *ops, int eard)
{
	uint eard_api;
	state_t s;

	if (!eard) {
		return_msg(EAR_ERROR, "EARD (daemon) not required");
	}
	if (!eards_connected()) {
		return_msg(EAR_ERROR, "EARD (daemon) not connected");
	}
	if (state_fail(s = eard_rpc(RPC_MET_CPUFREQ_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
		debug("RPC RPC_MET_CPUFREQ_GET_API returned: %s (%d)", state_msg, s);
		return s;
	}
	if (eard_api == API_NONE || eard_api == API_DUMMY) {
		return_msg(EAR_ERROR, "EARD (daemon) has loaded DUMMY/NONE API");
	}
	//
	cpu_count = tp->cpu_count;
	//
	replace_ops(ops->read, cpufreq_eard_read);
	
	return EAR_SUCCESS;
}

state_t cpufreq_eard_read(ctx_t *c, cpufreq_t *list)
{
    memset((void *) list, 0, sizeof(cpufreq_t)*cpu_count);
    return eard_rpc(RPC_MET_CPUFREQ_GET_CURRENT, NULL, 0, (char *) list, sizeof(cpufreq_t)*cpu_count);
}
