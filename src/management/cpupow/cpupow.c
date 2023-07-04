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
#include <pthread.h>
#include <management/cpupow/cpupow.h>
#include <management/cpupow/archs/dummy.h>
#include <management/cpupow/archs/amd17.h>
#include <management/cpupow/archs/intel63.h>

static pthread_mutex_t  lock = PTHREAD_MUTEX_INITIALIZER;
static uint             loaded;
static mgt_cpupow_ops_t ops;

void mgt_cpupow_load(topology_t *tp, int eard)
{
	while (pthread_mutex_trylock(&lock));
	if (loaded) {
		goto ret;
	}
    mgt_cpupow_intel63_load(tp, &ops);
    mgt_cpupow_amd17_load(tp, &ops);
    mgt_cpupow_dummy_load(tp, &ops);
    loaded = 1;
ret:
	pthread_mutex_unlock(&lock);
}

void mgt_cpupow_get_info(apinfo_t *info)
{
    info->layer = "MGT_CPUPOW";
    return ops.get_info(info);
}

int mgt_cpupow_count_devices(int domain)
{
    return ops.count_devices(domain);
}

int mgt_cpupow_powercap_is_capable(int domain)
{
    return ops.powercap_is_capable(domain);
}

state_t mgt_cpupow_powercap_is_enabled(int domain, uint *enabled)
{
    return ops.powercap_is_enabled(domain, enabled);
}

state_t mgt_cpupow_powercap_get(int domain, uint *watts)
{
    return ops.powercap_get(domain, watts);
}

state_t mgt_cpupow_powercap_set(int domain, uint *watts)
{
    return ops.powercap_set(domain, watts);
}

state_t mgt_cpupow_powercap_reset(int domain)
{
    return ops.powercap_reset(domain);
}

state_t mgt_cpupow_tdp_get(int domain, uint *watts)
{
    return ops.tdp_get(domain, watts);
}

void mgt_cpupow_tdp_tostr(int domain, uint *watts, char *buffer, int length)
{
    int acc, i;
    for (acc = i = 0; i < mgt_cpupow_count_devices(domain); ++i) {
        acc += sprintf(&buffer[acc], "TDP%d: %u\n", i, watts[i]);
    }
}

void mgt_cpupow_data_alloc(int domain, uint **list)
{
    int count = mgt_cpupow_count_devices(domain);
    *list = calloc(count, sizeof(uint));
}
