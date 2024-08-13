/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

// #define SHOW_DEBUGS 1

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/popen.h>
#include <common/string_enhanced.h>
#include <common/math_operations.h>
#include <metrics/common/nvml.h>
#include <metrics/gpu/archs/gpuprof_dcgmi.h>

// DCGMI limits all GPUs to monitor the same events. Because under the hood we
// are using POPEN which opens an additional process. If we wanted to add
// different events in different GPUs, we would have to open multiple POPEN
// processes.

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static gpuprof_evs_t *events;
static uint events_count;
static popen_t p;
static gpuprof_t *pool;
static double *pool_aux_buffer;	//auxiliar buffer
static char command_ids[256];
static ullong *serials;
static uint devs_count;
static uint user_devs_count;
static int *user_devs;

static int get_user_devices()
{
	char command[128];
	char buffer[128];
	char *args[5];
	int d;

	// We suppose this is correct by now
	user_devs_count = devs_count;
	serials = calloc(devs_count, sizeof(ullong));
	user_devs = calloc(devs_count, sizeof(ullong));

	for (d = 0; d < devs_count; ++d) {
		user_devs[d] = d;
	}
	// NVML is not working so, it's not our problem
	if (state_fail(nvml_open(NULL))) {
		return 1;
	}
	if (state_fail(nvml_get_devices(NULL, &user_devs_count))) {
		return 1;
	}
	// If is the same number of GPUs, we don't have any problem.
	if (user_devs_count == devs_count || user_devs_count == 0) {
		return 1;
	}
	// Counting again by serial codes
	user_devs_count = 0;
	//
	for (d = 0; d < devs_count; ++d) {
		serials[d] = 0LLU;
		// Negative means the device is not allocated by the user
		user_devs[d] = -1;
		// In dmon is -i or --gpu-id instead --gpuid
		sprintf(command, "dcgmi discovery -i a -v --gpuid %d 2>&1", d);
		popen_t p_tmp;
		if (state_fail(popen_open(command, 3, 1, &p_tmp))) {
			continue;
		}
		while (popen_read
		       (&p_tmp, "sssssa", &args[0], &args[1], &args[2],
			&args[3], &args[4])) {
			if (strcmp(args[1], "Serial") == 0) {
				serials[d] = (ullong) atoll(args[4]);
				debug("D%d detected serial: %llu", d,
				      serials[d]);
			}
		}
		popen_close(&p_tmp);
		// Editing flag -i
		debug("serial %llu", serials[d]);
		if (nvml_is_serial(serials[d])) {
			strcpy(buffer, command_ids);
			if (strlen(buffer)) {
				sprintf(command_ids, "%s,%u", buffer, d);
			} else {
				sprintf(command_ids, "-i %u", d);
			}
			user_devs[d] = user_devs_count;
			++user_devs_count;
		}
	}
	// If no user GPUs... We have a problem
	if (!user_devs_count) {
		return 0;
	}
	debug("Detected user GPUs: %d", user_devs_count);
	debug("IDs command: '%s'", command_ids);
	// New devices counter is the number of detected user devices
	devs_count = user_devs_count;
	return 1;
}

GPUPROF_F_LOAD(gpuprof_dcgmi_load)
{
	char *event_name;
	int event_id;
	char *count;
	state_t s;

	//dcgmi dmon --list
	if (apis_loaded(ops)) {
		return;
	}
	if (state_fail(s = popen_test("dcgmi"))) {
		debug("Failed popen_test: %s", state_msg);
		return;
	}
	// Getting the number of GPUS
	if (state_fail(s = popen_open("dcgmi discovery -l", 0, 1, &p))) {
		debug("Failed popen_open: %s", state_msg);
		return;
	}
	popen_read(&p, "s", &count);
	if (strstr(strtolow(count), "error") != NULL) {
		debug
		    ("Failed while reading dcgmi discovery: DCGMI returned error");
		popen_close(&p);
		return;
	}
	devs_count = (uint) atoi(count);
	popen_close(&p);
	if (devs_count == 0) {
		debug("No GPUs found");
		return;
	}
	// If NVML can be initialized, we check for the user serial GPUs
	if (!get_user_devices()) {
		debug("A problem is detected with user allocated GPUs");
		return;
	}
	// Events
	if (state_fail(s = popen_open("dcgmi dmon --list", 3, 1, &p))) {
		debug("Failed popen_open: %s", state_msg);
		return;
	}
	// Allocating
	if (!popen_read(&p, "sai", &event_name, &event_id)) {
		debug("Failed popen_read: %s", state_msg);
		return;
	}
	if ((events_count = popen_count_read(&p)) == 0) {
		debug("No events found");
		return;
	}
	events = calloc(events_count, sizeof(gpuprof_evs_t));
	events_count = 0;
	// Saving events
	do {
		strcpy(events[events_count].name, event_name);
		events[events_count].id = (uint) event_id;
		debug("EV%d: %s", event_id, event_name);
		++events_count;
	}
	while (popen_read(&p, "sai", &event_name, &event_id));

	popen_close(&p);

	// Allocating pool and an auxiliar buffer
	gpuprof_dcgmi_data_alloc(&pool);
	pool_aux_buffer = calloc(events_count, sizeof(double));
	//
	apis_put(ops->init, gpuprof_dcgmi_init);
	apis_put(ops->dispose, gpuprof_dcgmi_dispose);
	apis_put(ops->get_info, gpuprof_dcgmi_get_info);
	apis_put(ops->events_get, gpuprof_dcgmi_events_get);
	apis_put(ops->events_set, gpuprof_dcgmi_events_set);
	apis_put(ops->events_unset, gpuprof_dcgmi_events_unset);
	apis_put(ops->read, gpuprof_dcgmi_read);
	apis_put(ops->read_raw, gpuprof_dcgmi_read_raw);
	apis_put(ops->data_diff, gpuprof_dcgmi_data_diff);
	apis_put(ops->data_alloc, gpuprof_dcgmi_data_alloc);
	apis_put(ops->data_copy, gpuprof_dcgmi_data_copy);
	debug("Loaded DCGMI");
}

GPUPROF_F_INIT(gpuprof_dcgmi_init)
{
	// Implement a monitor to clean the POPEN buffer
	return EAR_SUCCESS;
}

GPUPROF_F_DISPOSE(gpuprof_dcgmi_dispose)
{
}

GPUPROF_F_GET_INFO(gpuprof_dcgmi_get_info)
{
	info->api = API_DCGMI;
	info->scope = SCOPE_NODE;
	info->granularity = GRANULARITY_PERIPHERAL;
	info->devs_count = user_devs_count;
}

GPUPROF_F_EVENTS_GET(gpuprof_dcgmi_events_get)
{
	*evs_count = events_count;
	*evs = events;
}

static void invalidate(gpuprof_t * data, int dev)
{
	data[dev].hash = NULL;
	data[dev].samples_count = 1.0;
	data[dev].values_count = 0U;
	memset(data[dev].values, 0, events_count * sizeof(double));
}

static void set(int dev, char *evs, uint ids_count)
{
	if (pool[dev].hash == evs) {
		return;
	}
	// If different reference, unset
	invalidate(pool, dev);
	// Changing pool values
	pool[dev].hash = evs;
	pool[dev].values_count = ids_count;
}

GPUPROF_F_EVENTS_SET(gpuprof_dcgmi_events_set)
{
	char command[512];
	uint ids_count;
	state_t s;
	uint *ids;
	int d;

	if (evs == NULL) {
		return;
	}
	// Getting the number of events to monitor
	ids = (uint *) strtoat(evs, ',', NULL, &ids_count, ID_UINT);
	free(ids);
	// Filling the pool
	while (pthread_mutex_trylock(&lock)) ;
	// New popen every 2 seconds
	sprintf(command, "dcgmi dmon -e %s %s -d 2000 2>&1", evs, command_ids);
	debug("Command: '%s'", command);
	if (state_fail(s = popen_open(command, 2, 0, &p))) {
		debug("Failed popen_open: %s", state_msg);
		goto leave1;
	}
	// Any device means all devices for DCGMI
	for (d = 0; d < devs_count; ++d) {
		set(d, evs, ids_count);
	}
 leave1:
	pthread_mutex_unlock(&lock);
}

static void unset()
{
	int d;
	popen_close(&p);
	for (d = 0; d < devs_count; ++d) {
		invalidate(pool, d);
	}
}

GPUPROF_F_EVENTS_UNSET(gpuprof_dcgmi_events_unset)
{
	while (pthread_mutex_trylock(&lock)) ;
	unset();
	pthread_mutex_unlock(&lock);
}

GPUPROF_F_READ(gpuprof_dcgmi_read)
{
	char *buffer;
	int dev;
	int m;

	while (pthread_mutex_trylock(&lock)) ;
	// Pooling
	while (popen_read(&p, "siD", &buffer, &dev, pool_aux_buffer)) {
		debug("Popen returned '%s'", buffer);
		if (strstr(buffer, "Error") != NULL) {
			unset();
			goto leave2;
		}
		// Converting a GPU in user GPU
		debug("D%d -> D%d", dev, user_devs[dev]);
		// If not a user GPU
		if (user_devs[dev] == -1) {
			continue;
		}
		dev = user_devs[dev];
		// If its a correct entry...
		if (strstr(buffer, "GPU") != NULL) {
			for (m = 0; m < pool[dev].values_count; ++m) {
				pool[dev].values[m] += pool_aux_buffer[m];
				debug
				    ("D%d_M%d: %04.2lf += %04.2lf, %0.lf samples, %p hash",
				     dev, m, pool[dev].values[m],
				     pool_aux_buffer[m],
				     pool[dev].samples_count, pool[dev].hash);
			}
			pool[dev].samples_count += 1.0;
			timestamp_getfast(&pool[dev].time);
		}
	}
	// Once pooling is over, we copy the values to data
 leave2:
	gpuprof_dcgmi_data_copy(data, pool);
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

GPUPROF_F_READ_RAW(gpuprof_dcgmi_read_raw)
{
	return EAR_SUCCESS;
}

GPUPROF_F_DATA_DIFF(gpuprof_dcgmi_data_diff)
{
	int dev, m;
	// Float time in seconds with micro seconds precission
	dataD->time_s =
	    timestamp_fdiff(&data2[0].time, &data1[0].time, TIME_SECS,
			    TIME_USECS);
	for (dev = 0; dev < devs_count; ++dev) {
		if (data2[dev].hash != data1[dev].hash) {
			invalidate(dataD, dev);
			continue;
		}
		// Cleaning values
		memset(dataD[dev].values, 0, events_count * sizeof(double));
		// Copying relevant data
		dataD[dev].hash = data2[dev].hash;
		dataD[dev].values_count = data2[dev].values_count;
		// Computing values
		if (data2[dev].samples_count > data1[dev].samples_count) {
			dataD[dev].samples_count =
			    data2[dev].samples_count - data1[dev].samples_count;
			for (m = 0; m < pool[dev].values_count; ++m) {
				dataD[dev].values[m] =
				    overflow_zeros_f64(data2[dev].values[m],
						       data1[dev].values[m]);
				dataD[dev].values[m] =
				    dataD[dev].values[m] /
				    dataD[dev].samples_count;
			}
		}
	}
}

GPUPROF_F_DATA_ALLOC(gpuprof_dcgmi_data_alloc)
{
	gpuprof_t *aux;
	int d;
	aux = calloc(devs_count, sizeof(gpuprof_t));
	// We allocate all values contiguously
	aux[0].values = calloc(devs_count * events_count, sizeof(double));
	// Setting pointers
	for (d = 0; d < devs_count; ++d) {
		aux[d].values = &aux[0].values[d * events_count];
	}
	*data = aux;
}

GPUPROF_F_DATA_COPY(gpuprof_dcgmi_data_copy)
{
	int dev;

	// The data[0] can access to all the allocated values
	memcpy(dataD[0].values, dataS[0].values,
	       devs_count * events_count * sizeof(double));
	// Copying index by index
	for (dev = 0; dev < devs_count; ++dev) {
		dataD[dev].samples_count = dataS[dev].samples_count;
		dataD[dev].values_count = dataS[dev].values_count;
		dataD[dev].hash = dataS[dev].hash;
		dataD[dev].time = dataS[dev].time;
	}
}
