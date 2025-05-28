/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <pthread.h>
#include <common/states.h>
#include <metrics/gpu/gpuprof.h>
#include <metrics/gpu/archs/gpuprof_nvml.h>
#include <metrics/gpu/archs/gpuprof_dcgmi.h>
#include <metrics/gpu/archs/gpuprof_dummy.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static gpuprof_ops_t ops;
static uint          ok_load;

void gpuprof_load(int force_api)
{
    while (pthread_mutex_trylock(&lock));
    if (ok_load) {
        goto unlock_load;
    }
    if (API_IS(force_api, API_DUMMY)) {
        goto dummy;
    }
    //gpuprof_rocm_load(&ops, force_api);
    gpuprof_nvml_load(&ops);
    gpuprof_dcgmi_load(&ops);
dummy:
    gpuprof_dummy_load(&ops);
    ok_load = 1;
unlock_load:
    pthread_mutex_unlock(&lock);
}

GPUPROF_F_INIT(gpuprof_init)
{
	if (ok_load)
	{
		if (ops.init)
		{
			return ops.init();
		} else
		{
			return EAR_SUCCESS;
		}
	} else
	{
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
}

void gpuprof_get_info(apinfo_t *info)
{
    info->layer = "GPUPROF";
    ops.get_info(info);
}

GPUPROF_F_EVENTS_GET(gpuprof_events_get)
{
    ops.events_get(evs, evs_count);
}

GPUPROF_F_EVENTS_SET(gpuprof_events_set)
{
    ops.events_set(dev, evs);
}

GPUPROF_F_EVENTS_UNSET(gpuprof_events_unset)
{
    ops.events_unset(dev);
}

state_t gpuprof_read(gpuprof_t *data)
{
    return ops.read(data);
}

state_t gpuprof_read_raw(gpuprof_t *data)
{
    return ops.read_raw(data);
}

state_t gpuprof_read_diff(gpuprof_t *data2, gpuprof_t *data1, gpuprof_t *data_diff)
{
    state_t s = EAR_SUCCESS;
    if (state_fail(s = gpuprof_read(data2))) {
        return s;
    }
    gpuprof_data_diff(data2, data1, data_diff);
    return s;
}

state_t gpuprof_read_copy(gpuprof_t *data2, gpuprof_t *data1, gpuprof_t *data_diff)
{
    state_t s = EAR_SUCCESS;
    if (state_fail(s = gpuprof_read_diff(data2, data1, data_diff))) {
        return s;
    }
    gpuprof_data_copy(data1, data2);
    return s;
}

GPUPROF_F_DATA_DIFF(gpuprof_data_diff)
{
    return ops.data_diff(data2, data1, dataD);
}

GPUPROF_F_DATA_ALLOC(gpuprof_data_alloc)
{
    return ops.data_alloc(data);
}

GPUPROF_F_DATA_COPY(gpuprof_data_copy)
{
    return ops.data_copy(dataD, dataS);
}

int gpuprof_compare_events(const void *gpuprof_ev_ptr1, const void *gpuprof_ev_ptr2)
{
	const gpuprof_evs_t *ev1 = gpuprof_ev_ptr1;
	const gpuprof_evs_t *ev2 = gpuprof_ev_ptr2;

	return ev1->id - ev2->id;
}

#ifdef TEST_EXAMPLE
#include <stdlib.h>
#include <metrics/common/apis.h>
#include <common/system/monitor.h>

static gpuprof_t            *m1;
static gpuprof_t            *m2;
static gpuprof_t            *mD;
static const gpuprof_evs_t *evs;
static uint                 evs_count;
static apinfo_t             info;

static int is(char *s1, const char *s2)
{
    return strcasecmp(s1, s2) == 0;
}

int main(int argc, char *argv[])
{
    static char command[128];
    int i, m, d;

    monitor_init();
    while(1)
    {
        printf("Enter command: ");
        scanf("%s", command);
        if (is(command, "load")) {
            gpuprof_load(0);
            gpuprof_get_info(&info);
            apinfo_tostr(&info);
            printf("LOADED: %s %s %s %d\n", info.layer, info.api_str, info.granularity_str, info.devs_count);
        } else if (is(command, "alloc")) {
            gpuprof_data_alloc(&m1);
            gpuprof_data_alloc(&m2);
            gpuprof_data_alloc(&mD);
            printf("ALLOCATED DATA\n");
        } else if (is(command, "get-events")) {
            gpuprof_events_get(&evs, &evs_count);
            for (i = 0; i < evs_count; ++i) {
                printf("EVENT_%d: id.%02u %s\n", i, evs[i].id, evs[i].name);
            }
        } else if (is(command, "set-events")) {
            char *option = calloc(128, sizeof(char));
            scanf("%s", option);
            gpuprof_events_set(0, option);
            printf("EVENTS SET: %s\n", option);
        } else if (is(command, "read")) {
            gpuprof_read(m1);
            sleep(4);
            gpuprof_read_diff(m2, m1, mD);
            for (d = 0; d < info.devs_count; ++d) {
                for (m = 0; m < mD[d].values_count; ++m) {
                    printf("METRIC_D%d_M%d: %lf\n", d, m, mD[d].values[m]);
                }
            }
        }
    }
    return 0;
}
#endif
