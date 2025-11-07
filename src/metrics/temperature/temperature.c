/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <assert.h>
#include <common/output/debug.h>
#include <metrics/common/apis.h>
#include <metrics/temperature/archs/dummy.h>
#include <metrics/temperature/archs/eard.h>
#include <metrics/temperature/archs/hwmon.h>
#include <metrics/temperature/archs/intel63.h>
#include <metrics/temperature/temperature.h>
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint is_loaded;
static apinfo_t info;
static temp_ops_t ops;

void temp_load(topology_t *tp, uint force_api)
{
    while (pthread_mutex_trylock(&lock));
    if (is_loaded) {
        goto leave;
    }
    if (API_IS(force_api, API_DUMMY)) {
        goto dummy;
    }
#if WF_SUPPORT
    temp_eard_load(tp, &ops, force_api);
#endif
    temp_hwmon_load(tp, &ops, force_api);
    temp_intel63_load(tp, &ops, force_api);
dummy:
    temp_dummy_load(tp, &ops, force_api);
    temp_get_info(&info);
    is_loaded = 1;
leave:
    pthread_mutex_unlock(&lock);
}

void temp_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    ops.get_info(info);
}

state_t temp_init()
{
    return ops.init();
}

void temp_dispose()
{
    ops.dispose();
}

// Getters
state_t temp_read(llong *temp, llong *average)
{
    return ops.read(temp, average);
}

state_t temp_read_copy(llong *t2, llong *t1, llong *tD, llong *average)
{
    state_t s = temp_read(t2, average);
    temp_data_copy(tD, t2);
    return s;
}

// Data
void temp_data_alloc(llong **temp)
{
    *temp = (llong *) calloc(info.devs_count, sizeof(llong));
    assert(*temp != NULL);
}

void temp_data_copy(llong *tempD, llong *tempS)
{
    memcpy((void *) tempD, (const void *) tempS, sizeof(llong) * info.devs_count);
}

void temp_data_diff(llong *temp2, llong *temp1, llong *tempD, llong *average)
{
    int i;
    temp_data_copy(tempD, temp2);
    if (average != NULL) {
        *average = 0;
        for (i = 0; i < info.devs_count; ++i) {
            *average += temp2[i];
        }
        *average /= (llong) info.devs_count;
    }
}

void temp_data_free(llong **temp)
{
    if (*temp != NULL) {
        free(*temp);
        *temp = NULL;
    }
}

char *temp_data_tostr(llong *list, llong avrg, char *buffer, int length)
{
    ullong a = 0;
    int i;

    for (i = 0; i < info.devs_count; ++i) {
        a += sprintf(&buffer[a], "%lld ", list[i]);
    }
    sprintf(&buffer[a], "!%lld\n", avrg);
    return buffer;
}

void temp_data_print(llong *list, llong avrg, int fd)
{
    char buffer[1024];
    temp_data_tostr(list, avrg, buffer, 1024);
    dprintf(fd, "%s", buffer);
}

#if TEST
static topology_t tp;
static llong t1[128];
static llong t2[128];
static llong tD[128];
static llong tA;

int main(int argc, char **argv)
{
    topology_init(&tp);
    temp_load(&tp, API_FREE);
    temp_read(t1, &tA);
    sleep(1);
    temp_read_copy(t2, t1, tD, &tA);
    temp_data_print(tD, tA, verb_channel);
    return 0;
}
#endif