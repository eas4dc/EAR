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
//#define FAKE_GPUS   1

#include <stdlib.h>
#include <common/output/debug.h>
#include <metrics/gpu/archs/dummy.h>

#if FAKE_GPUS
#define FAKE_N 4 
#endif

void gpu_dummy_load(gpu_ops_t *ops)
{
    apis_put(ops->get_api,       gpu_dummy_get_api);
    apis_put(ops->init,          gpu_dummy_init);
    apis_put(ops->dispose,       gpu_dummy_dispose);
    apis_put(ops->get_devices,   gpu_dummy_get_devices);
    apis_put(ops->count_devices, gpu_dummy_count_devices);
    apis_put(ops->read,          gpu_dummy_read);
    apis_put(ops->read_raw,      gpu_dummy_read_raw);
}

void gpu_dummy_get_api(uint *api)
{
    #if FAKE_GPUS
    *api = API_FAKE;
    #else
    *api = API_DUMMY;
    #endif
}

state_t gpu_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t gpu_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t gpu_dummy_get_devices(ctx_t *c, gpu_devs_t **devs, uint *devs_count)
{
    #if FAKE_GPUS
    *devs = calloc(4, sizeof(gpu_devs_t));
    (*devs)[0].serial = (*devs)[0].index = 0;
    (*devs)[1].serial = (*devs)[1].index = 1;
    (*devs)[2].serial = (*devs)[2].index = 2;
    (*devs)[3].serial = (*devs)[3].index = 3;
    #else
    *devs = calloc(1, sizeof(gpu_devs_t));
    #endif
    return gpu_dummy_count_devices(c, devs_count);
}

state_t gpu_dummy_count_devices(ctx_t *c, uint *devs_count)
{
	if (devs_count != NULL) {
        #if FAKE_GPUS
        *devs_count = FAKE_N;
        #else
        *devs_count = 1;
        #endif
	}
	return EAR_SUCCESS;
}

#if FAKE_GPUS
static state_t fake_read(gpu_t *data)
{
    timestamp_t time;
    int r;

    timestamp_getfast(&time);

    for (r = 0; r < FAKE_N; ++r) {
    data[r].time      = time;
    data[r].samples  += 1;
    data[r].freq_gpu += r+1;
    data[r].freq_mem += r+1;
    data[r].util_gpu += r+20;
    data[r].util_mem += r+20;
    data[r].temp_gpu += r+70;
    data[r].temp_mem += r+70;
    data[r].energy_j  = 0;
    data[r].power_w  += r+100;
    data[r].working   = 1;
    data[r].correct   = 1;
    }
    return EAR_SUCCESS;
}
#endif

state_t gpu_dummy_read(ctx_t *c, gpu_t *data)
{
    #if FAKE_GPUS
    return fake_read(data);
    #else
	return gpu_data_null(data);
    #endif
}

state_t gpu_dummy_read_raw(ctx_t *c, gpu_t *data)
{
    #if FAKE_GPUS
    return fake_read(data);
    #else
	return gpu_data_null(data);
    #endif
}
