/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/utils/string.h>
#include <common/system/monitor.h>
#include <common/system/time.h>
#include <common/math_operations.h>
#include <metrics/common/nvml.h>
#include <metrics/gpu/archs/gpuprof_nvml.h>

static pthread_mutex_t      lock = PTHREAD_MUTEX_INITIALIZER;
static nvmlGpmMetricsGet_t *gpms; //N devices
static gpuprof_evs_t       *events;
static uint                 events_count;
static gpuprof_t           *pool;
static uint                 pooling;
static nvml_t               nvml;
static uint			        devs_count;
static nvmlDevice_t        *devs;
static suscription_t       *sus;

// Why this class uses the pooling instead multiple nvmlGpmMetricsGet_t?
//
// Because if you add a void pointer to gpuprof_t to keep nvmlGpmMetricsGet_t,
// you have to:
//  1) Remove the data1 or data2 from functions and use single data, because
//     nvmlGpmMetricsGet_t keeps the sample2 and sample1 itself, and you can't
//     allocate two gpuprof_t structs and know in advance to what other gpuprof_t
//     is related. But is not recommended to change the design just because one
//     intern API is different. Pooling solves that because internally we use
//     just one nvmlGpmMetricsGet_t.
//  2) If someone changes the events through events_set, the nvmlGpmMetricsGet_t 
//     will become invalid.
//

static state_t gpuprof_nvml_pool(void *p);

static int support_test()
{
    static nvmlGpmSupport_t support;
    nvmlReturn_t r;

    #ifndef NVML_GPM_SUPPORT_VERSION
    // Compiled without GPM header
    return 0;
    #endif
    support.version = NVML_GPM_METRICS_GET_VERSION;
    if (nvml.GpmQueryDeviceSupport == NULL || nvml.GpmMetricsGet == NULL) {
        debug("GPM symbols not found");
        return 0;
    }
    if ((r = nvml.GpmQueryDeviceSupport(devs[0], &support)) != NVML_SUCCESS) {
        debug("Error while checking NVML GPM support: %s", nvml.ErrorString(r));
        return 0;
    }

		return (support.isSupportedDevice) ? 1 : 0;
}

static int samples_alloc(nvmlGpmMetricsGet_t *sev)
{
    nvmlReturn_t r;
    
    sev->version = NVML_GPM_METRICS_GET_VERSION;
    if ((r = nvml.GpmSampleAlloc(&sev->sample1)) != NVML_SUCCESS) {
        debug("Error while allocating sample1: %s", nvml.ErrorString(r));
        return 0;
    }
    if ((r = nvml.GpmSampleAlloc(&sev->sample2)) != NVML_SUCCESS) {
        debug("Error while allocating sample2: %s", nvml.ErrorString(r));
        return 0;
    }
    return 1;
}

#if 0
static int samples_free(nvmlGpmMetricsGet_t *sev)
{
    return 0;
}
#endif

static int events_fill(nvmlGpmMetricsGet_t *sev, int i)
{
    nvmlReturn_t r;
    if (i >= NVML_GPM_METRIC_MAX) {
        return 0;
    }
    // Cleaning metric
    memset(&sev->metrics[0], 0, sizeof(nvmlGpmMetric_t));
    // 
    sev->numMetrics          = 1;
    sev->metrics[0].metricId = i;
    
    if ((r = nvml.GpmMetricsGet(sev)) != NVML_SUCCESS) {
        debug("Error while loading NVML metrics: %s", nvml.ErrorString(r));
        return 1;
    }
    if (sev->metrics[0].metricInfo.longName == NULL) {
        return 1;
    }
    strcpy(events[events_count].name, sev->metrics[0].metricInfo.longName);
    debug("EV%d: %s (%s)", i, events[events_count].name, sev->metrics[0].metricInfo.unit);
    events[events_count].id = (uint) i;
    events_count++;
    return 1;
}

GPUPROF_F_LOAD(gpuprof_nvml_load)
{
    int i = 0;

    if (apis_loaded(ops)) {
        return;
    }
    if (state_fail(nvml_open(&nvml))) {
        debug("nvml_open failed: %s", state_msg);
        return;
    }
    if (state_fail(nvml_get_devices(&devs, &devs_count))) {
        debug("nvml_get_devices failed: %s", state_msg);
        return;
    }
    if (!support_test()) {
        debug("Unsupported NVML profiling functions");
        return;
    }
    // Allocating space for NVML metrics for all devices + additional for testing
    gpms = calloc(devs_count + 1, sizeof(nvmlGpmMetricsGet_t));
    // Allocating space for our own event list (events)
    events = calloc(NVML_GPM_METRIC_MAX, sizeof(gpuprof_evs_t));
    // Allocating samples
    while (i < (devs_count+1)) {
        if (!samples_alloc(&gpms[i])) {
            return;
        }
        i++;
    }
    // Allocating space for pooling counters
    gpuprof_nvml_data_alloc(&pool);
    // Getting event names and ids
    i = 1;
		while(events_fill(&gpms[devs_count], i++));
		apis_put(ops->init,							gpuprof_nvml_init);
		apis_put(ops->get_info,         gpuprof_nvml_get_info);
		apis_put(ops->events_get,       gpuprof_nvml_events_get);
		apis_put(ops->events_set,       gpuprof_nvml_events_set);
		apis_put(ops->events_unset,     gpuprof_nvml_events_unset);
		apis_put(ops->read,             gpuprof_nvml_read);
		apis_put(ops->read_raw,         gpuprof_nvml_read_raw);
		apis_put(ops->data_diff,        gpuprof_nvml_data_diff);
		apis_put(ops->data_alloc,       gpuprof_nvml_data_alloc);
		apis_put(ops->data_copy,        gpuprof_nvml_data_copy);
    debug("Loaded NVML");
}

GPUPROF_F_INIT(gpuprof_nvml_init)
{
    if (pooling) {
        return EAR_SUCCESS;
    }

		debug("gpuprof_nvml_init");

    // Creating pooling thread
    sus = suscription();
    sus->call_main  = gpuprof_nvml_pool;
    sus->time_relax = 2000;
    sus->time_burst = 2000;
    sus->suscribe(sus);
		monitor_register(sus);
    pooling = 1;

    return EAR_SUCCESS;
}

GPUPROF_F_DISPOSE(gpuprof_nvml_dispose)
{
    if (pooling) {
        monitor_unregister(sus);
        pooling = 0;
    }
}


GPUPROF_F_GET_INFO(gpuprof_nvml_get_info)
{
    info->api         = API_NVML;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_PERIPHERAL;
    info->devs_count  = devs_count;
}

GPUPROF_F_EVENTS_GET(gpuprof_nvml_events_get)
{
    *evs = events;
    *evs_count = events_count;
}

static state_t gpuprof_nvml_pool(void *p)
{
    nvmlReturn_t r;
    int d, m;

    // nvmlGpmMetricsGet and nvmlGpmSampleGet overhead was measured between 0 and 3 ms.
    while (pthread_mutex_trylock(&lock));
    for (d = 0; d < devs_count; ++d) {
        // Reference is a pointer to the events list
        if (pool[d].hash == NULL) {
            continue;
        }
        // Getting sample2
        if ((r = nvml.GpmSampleGet(devs[d], gpms[d].sample2)) != NVML_SUCCESS) {
            debug("Error while loading NVML sample: %s", nvml.ErrorString(r));
            continue;
        }
        // Differences
        if ((r = nvml.GpmMetricsGet(&gpms[d])) != NVML_SUCCESS) {
            debug("Error while loading NVML metrics: %s", nvml.ErrorString(r));
            continue;
        }
        for (m = 0; m < gpms[d].numMetrics; ++m) {
            if (!isnan(gpms[d].metrics[m].value)) {
                pool[d].values[m] += gpms[d].metrics[m].value;
            }
            debug("%s: %lf.[%d][%d] += %lf %s (nvmlReturn %d: %s)",
                gpms[d].metrics[m].metricInfo.longName, pool[d].values[m], d, m,
                gpms[d].metrics[m].value, gpms[d].metrics[m].metricInfo.unit,
                gpms[d].metrics[m].nvmlReturn, nvml.ErrorString(gpms[d].metrics[m].nvmlReturn));
        }
        pool[d].samples_count += 1.0;
        // Getting sample 1
        //memcpy(gpms[d].sample1, gpms[d].sample2, sizeof(nvmlGpmSample_t));
        if ((r = nvml.GpmSampleGet(devs[d], gpms[d].sample1)) != NVML_SUCCESS) {
            debug("Error while loading NVML sample: %s", nvml.ErrorString(r));
            continue;
        }
    }
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

static void invalidate(gpuprof_t *data, int dev)
{
    data[dev].hash          = NULL;
    data[dev].samples_count = 1.0;
    data[dev].values_count  = 0U;
    memset(data[dev].values , 0, NVML_GPM_METRIC_MAX*sizeof(double));
}

static void unset(int dev)
{
    memset(gpms[dev].metrics, 0, sizeof(gpms[dev].metrics));
    invalidate(pool, dev);
}

static int set(int dev, char *evs, uint *ids, uint ids_count)
{
    nvmlReturn_t r;
    int i = 0;

    if (pool[dev].hash == evs) {
        return 1;
    }
    // If different reference, unset
    unset(dev);
    // Changing metrics values
    while (i < ids_count) {
        debug("D%d: setting event %u", dev, ids[i]);
        gpms[dev].metrics[i].metricId = ids[i];
        ++i;
    }
    gpms[dev].numMetrics = ids_count;
    // Enabling
    if ((r = nvml.GpmMetricsGet(&gpms[dev])) != NVML_SUCCESS) {
        debug("Error while loading NVML metrics: %s", nvml.ErrorString(r));
        return 0;
    }
    // Checking if correct
    i = 0;
    while (i < ids_count) {
        if (gpms[dev].metrics[i].nvmlReturn == NVML_ERROR_INVALID_ARGUMENT) {
						debug("nvmlReturn INVALID_ARGUMENT");
             return 0;
        }
        ++i;
    }
    if ((r = nvml.GpmSampleGet(devs[dev], gpms[dev].sample1)) != NVML_SUCCESS) {
        debug("Error while loading NVML sample: %s", nvml.ErrorString(r));
        return 0;
    }
    // Changing pool values
    pool[dev].hash = evs;
    pool[dev].values_count = ids_count;
    return 1;
}

GPUPROF_F_EVENTS_SET(gpuprof_nvml_events_set)
{
    uint ids_count;
    uint *ids;
    int d;

    if (evs == NULL) {
        return;
    }
    // String treatment (is a NULL terminated list)
    ids = (uint *) strtoat(evs, ',', NULL, &ids_count, ID_UINT);
    //
    while (pthread_mutex_trylock(&lock));
    if (dev == all_devs) {
        for (d = 0; d < devs_count; ++d) {
            if (!set(d, evs, ids, ids_count)) {
                unset(d);
            }
        }
    } else {
        if (!set(dev, evs, ids, ids_count)) {
            unset(dev);
        }
    }
    pthread_mutex_unlock(&lock);
    free(ids);
}

GPUPROF_F_EVENTS_UNSET(gpuprof_nvml_events_unset)
{
    int d;
    while (pthread_mutex_trylock(&lock));
    if (dev == all_devs) {
        for (d = 0; d < devs_count; ++d) {
            unset(d);
        }
    } else {
        unset(dev);
    }
    pthread_mutex_unlock(&lock);
}

GPUPROF_F_READ(gpuprof_nvml_read)
{
	// gpuprof_nvml_pool(NULL);
	while (pthread_mutex_trylock(&lock));

	gpuprof_nvml_data_copy(data, pool);
	timestamp_getfast(&data[0].time);
	for (int i = 1; i < devs_count; i++)
	{
		data[i].time = data[0].time;
	}

	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

GPUPROF_F_READ_RAW(gpuprof_nvml_read_raw)
{
    return EAR_SUCCESS;
}

GPUPROF_F_DATA_DIFF(gpuprof_nvml_data_diff)
{
    int dev, m;
    // Float time in seconds with micro seconds precission
    dataD->time_s = timestamp_fdiff(&data2[0].time, &data1[0].time, TIME_SECS, TIME_USECS);
    for (dev = 0; dev < devs_count; ++dev) {
        if (data2[dev].hash != data1[dev].hash) {
            invalidate(dataD, dev);
            continue;
        }
        // Copying relevant data
        dataD[dev].hash          = data2[dev].hash;
        dataD[dev].values_count  = data2[dev].values_count;
        // Cleaning values
        memset(dataD[dev].values, 0, NVML_GPM_METRIC_MAX*sizeof(double));
        // Computing values
        if (data2[dev].samples_count > data1[dev].samples_count) {
            dataD[dev].samples_count = data2[dev].samples_count - data1[dev].samples_count;
            for (m = 0; m < gpms[dev].numMetrics; ++m) {
                dataD[dev].values[m]     = overflow_zeros_f64(data2[dev].values[m], data1[dev].values[m]);
                dataD[dev].values[m]     = dataD[dev].values[m] / dataD[dev].samples_count;
            }
        }
    }
}

GPUPROF_F_DATA_ALLOC(gpuprof_nvml_data_alloc)
{
    gpuprof_t *aux;
    int d;
    aux = calloc(devs_count, sizeof(gpuprof_t));
    // We allocate all values contiguously
    aux[0].values = calloc(devs_count*NVML_GPM_METRIC_MAX, sizeof(double));
    // Setting pointers
    for (d = 0; d < devs_count; ++d) {
        aux[d].values = &aux[0].values[d*NVML_GPM_METRIC_MAX];
    }
    *data = aux;
}

GPUPROF_F_DATA_COPY(gpuprof_nvml_data_copy)
{
    int dev;

    // The data[0] can access to all the allocated values
    memcpy(dataD[0].values,  dataS[0].values, devs_count*NVML_GPM_METRIC_MAX*sizeof(double));
    // Copying index by index
    for (dev = 0; dev < devs_count; ++dev) {
        dataD[dev].samples_count = dataS[dev].samples_count;
        dataD[dev].values_count  = dataS[dev].values_count;
        dataD[dev].hash          = dataS[dev].hash;
        dataD[dev].time          = dataS[dev].time;
    }
}
