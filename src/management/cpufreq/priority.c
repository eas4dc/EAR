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
#include <stdlib.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <management/cpufreq/priority.h>
#include <management/cpufreq/archs/prio_eard.h>
#include <management/cpufreq/archs/prio_isst.h>
#include <management/cpufreq/archs/prio_dummy.h>

static char            *strprio[2]  = { "high", "low" };
static pthread_mutex_t  lock = PTHREAD_MUTEX_INITIALIZER;
static mgt_prio_ops_t   ops;
static uint             init;
static uint             prios_count;
static uint             idxs_count;

void mgt_cpufreq_prio_load(topology_t *tp, int eard)
{
    while (pthread_mutex_trylock(&lock));
    if (init) {
        goto leave;
    }
    mgt_prio_isst_load(tp, &ops);
    mgt_prio_eard_load(tp, &ops, eard);
    mgt_prio_dummy_load(tp, &ops);
    init = 1;
leave:
    pthread_mutex_unlock(&lock);
}

void mgt_cpufreq_prio_get_api(uint *api)
{
    return ops.get_api(api);
}

state_t mgt_cpufreq_prio_init()
{
    state_t s;
    if (state_fail(s = ops.init())) {
        return s;
    }
    ops.data_count(&prios_count, &idxs_count);
    return s;
}

state_t mgt_cpufreq_prio_dispose()
{
    return ops.dispose();
}

state_t mgt_cpufreq_prio_enable()
{
    return ops.enable();
}

state_t mgt_cpufreq_prio_disable()
{
    return ops.disable();
}

int mgt_cpufreq_prio_is_enabled()
{
    return ops.is_enabled();
}

state_t mgt_cpufreq_prio_get_available_list(cpuprio_t *prio_list)
{
    return ops.get_available_list(prio_list);
}

state_t mgt_cpufreq_prio_set_available_list(cpuprio_t *prio_list)
{
    return ops.set_available_list(prio_list);
}

state_t mgt_cpufreq_prio_set_available_list_by_freqs(ullong *max_khz, ullong *min_khz)
{
    static cpuprio_t *prios = NULL;
    static uint prios_count = 0;
    state_t s;
    int i;

    if (prios == NULL) {
        mgt_cpufreq_prio_data_alloc(&prios, NULL);
        mgt_cpufreq_prio_data_count(&prios_count, NULL);
    }
    if (state_fail(s = mgt_cpufreq_prio_get_available_list(prios))) {
        return s;
    }
    for (i = 0; i < prios_count; ++i) {
        if (max_khz != NULL) { prios[i].max_khz = max_khz[i]; }
        if (min_khz != NULL) { prios[i].min_khz = min_khz[i]; }
    }
    return mgt_cpufreq_prio_set_available_list(prios);
}

state_t mgt_cpufreq_prio_get_current_list(uint *idx_list)
{
    return ops.get_current_list(idx_list);
}

state_t mgt_cpufreq_prio_set_current_list(uint *idx_list)
{
    return ops.set_current_list(idx_list);
}

state_t mgt_cpufreq_prio_set_current(uint prio, int cpu)
{
    return ops.set_current(prio, cpu);
}

void mgt_cpufreq_prio_data_count(uint *prios_count, uint *idxs_count)
{
    return ops.data_count(prios_count, idxs_count);
}

void mgt_cpufreq_prio_data_alloc(cpuprio_t **prio_list, uint **idx_list)
{
    *prio_list = calloc(prios_count, sizeof(cpuprio_t));
    *idx_list = calloc(idxs_count, sizeof(uint));
}

void mgt_cpufreq_prio_data_print(cpuprio_t *prio_list, uint *idx_list, int fd)
{
    char buffer[8192]; // AMD can have 256 CPUs
    mgt_cpufreq_prio_data_tostr(prio_list, idx_list, buffer, 1024);
    dprintf(fd, "%s", buffer);
}

void mgt_cpufreq_prio_data_tostr(cpuprio_t *prio_list, uint *idx_list, char *buffer, int length)
{
    int acc = 0;
    int c;

    debug("total prios_count %d", prios_count);
    for (c = 0; prio_list && c < prios_count; ++c)
    {
        debug("Processing prio %d", c); 
        if (prio_list[c].max_khz == PRIO_FREQ_MAX) {
            acc += sprintf(&buffer[acc], "PRIO%d: MAX GHZ - %.1lf GHz (%s)\n", c,
               ((double) prio_list[c].min_khz) / 1000000.0, strprio[prio_list[c].low]);
        } else {
            acc += sprintf(&buffer[acc], "PRIO%d: %.1lf GHz - %.1lf GHz (%s)\n", c,
               KHZ_TO_GHZ((double) prio_list[c].max_khz), KHZ_TO_GHZ((double) prio_list[c].min_khz), strprio[prio_list[c].low]);
        }
    }
    for (c = 0; idx_list && c < idxs_count; ++c) {
        if ((c != 0) && (c % 8) == 0) acc += sprintf(&buffer[acc], "\n");
        acc += sprintf(&buffer[acc], "[%03d,%2d] ", c, idx_list[c]);
    }
    acc += sprintf(&buffer[acc], "\n");
}

#if TEST
// /opt/rh/devtoolset-7/root/bin/gcc -I ../.. -o test_prio priority.c ./archs/prio_dummy.o ./archs/prio_isst.o ./archs/prio_eard.o ../../daemon/local_api/libeard.a ../../metrics/libmetrics.a ../../common/libcommon.a -lpthread -lm
#include <daemon/local_api/eard_api_rpc.h>

int main(int argc, char *argv[])
{
    cpuprio_t *prios;
    topology_t tp;
    uint *devs;
    state_t s;
    uint api;

    topology_init(&tp);

    eards_connection();

    mgt_cpufreq_prio_load(&tp, 1);
    mgt_cpufreq_prio_init();
    mgt_cpufreq_prio_get_api(&api);
    apis_print(api, "PRIO API: ");

    mgt_cpufreq_prio_data_alloc(&prios, &devs);
    if (state_fail(s = mgt_cpufreq_prio_get_available_list(prios))) {
        serror("mgt_cpufreq_prio_get_available_list");
    }
    if (state_fail(s = mgt_cpufreq_prio_get_current_list(devs))) {
        serror("mgt_cpufreq_prio_get_current_list");
    }
    mgt_cpufreq_prio_data_print(prios, devs, 0);
    
    #if 0 
    if (state_fail(s = mgt_cpufreq_prio_set_current(0, all_cpus))) {
        serror("mgt_cpufreq_prio_set_current");
    }
    #endif
    devs[ 0] = devs[ 1] = devs[ 2] = devs[ 3] = 0;
    devs[64] = devs[65] = devs[66] = devs[67] = 0;
    if (state_fail(s = mgt_cpufreq_prio_set_current_list(devs))) {
        serror("mgt_cpufreq_prio_set_current");
    }
    if (state_fail(s = mgt_cpufreq_prio_get_current_list(devs))) {
        serror("mgt_cpufreq_prio_get_current_list");
    }
    mgt_cpufreq_prio_data_print(NULL, devs, 0);
    
    mgt_cpufreq_prio_dispose();
 
    return 0;
}
#endif
