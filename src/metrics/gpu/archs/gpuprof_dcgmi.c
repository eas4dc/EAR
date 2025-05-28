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

#define DCGMI_POOL_CACHE_SIZE 3

typedef struct dcgmi_pool_cache_s {
	short int dirty_pos;
	short int masks[DCGMI_POOL_CACHE_SIZE];
	uint hash_evs_count[DCGMI_POOL_CACHE_SIZE];
	/* Size of each hash/evs is at most 12 events x 4 chars each id + 11 commas */
	char hashes[DCGMI_POOL_CACHE_SIZE][64];
	char evs_supported[DCGMI_POOL_CACHE_SIZE][64];
} dcgmi_pool_cache_t;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static gpuprof_evs_t *events;
static uint events_count;
static popen_t p;
static gpuprof_t *pool;
static short int event_pool_mask;
static double *pool_aux_buffer;	//auxiliar buffer
static char command_ids[256];
static ullong *serials;
static uint devs_count;
static uint user_devs_count;
static int *user_devs;

// A cache to memorize already requested events list is used to avoid
// filtering for unsupported events every time new event set is requested.
static dcgmi_pool_cache_t pool_cache;

static unsigned char pool_cache_check(char *hash, char **evs, uint *evs_count, short int *mask);
static void pool_cache_update(char *hash, char *evs_supported, uint evs_count, short int mask);

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

// TODO: Return error to notify the API user
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
	if (state_fail(s = popen_open("dcgmi profile -l", 3, 1, &p))) {
		debug("Failed popen_open: %s", state_msg);
		return;
	}
	// Allocating
	if (!popen_read(&p, "aaaiasa", &event_id, &event_name)) {
		debug("Failed popen_read: %s", state_msg);
		return;
	}
	if (event_id == 0 || ((events_count = popen_count_read(&p)) == 0)) {
		debug("No events found");
		return;
	}
	events = calloc(events_count, sizeof(gpuprof_evs_t));
	events_count = 0;
	// Saving events
	/* popen_read reads the last line of dcgmi profile -l and formats it as event_id = 0,
	 * therefore we filter this last event. */
	do {
		strcpy(events[events_count].name, event_name);
		events[events_count].id = (uint) event_id;
		debug("EV%d: %s", event_id, event_name);
		++events_count;
	}
	while (popen_read(&p, "aaaiasa", &event_id, &event_name) && event_id != 0);

	popen_close(&p);

	// Sorting events for checking later whether a requested event is supported
	qsort(events, events_count, sizeof(gpuprof_evs_t), gpuprof_compare_events);

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

static short int filter_unsupported_events(char *req_evs_str, uint *req_evs_count, char **evs_supported)
{
	// The resulting mask indicates which ev_ids are filtered
	short int mask = 0;

	// If req_evs_str is in the cache, other arguments are filled
	if (pool_cache_check(req_evs_str, evs_supported, req_evs_count, &mask)) {
		return mask;
	}

	// If not in the cache, arguments are left untouched

	// Getting the number of events to monitor
	uint *ev_ids = (uint *) strtoat(req_evs_str, ',', NULL, req_evs_count, ID_UINT);

	// An auxiliar buffer to store the string representation of an event id
	char ev_str[8];

	// A boolean variable used to format the filtered list
	uint first = 1;

	size_t evs_supported_len = 0;

	for (int i = 0; i < *req_evs_count; i++) {

		/* Create a gpuprof_evs_t to search for an id */
		gpuprof_evs_t req_ev = {.id = ev_ids[i]};

		/* Search across events supported for the requested event id */
		if (bsearch(&req_ev, events, events_count, sizeof(gpuprof_evs_t), gpuprof_compare_events)) {

			/* Format the supported event depending on whether it is the first in the list */
			if (!first) {
				snprintf(ev_str, sizeof(ev_str), ",%u", req_ev.id);
			} else {
				snprintf(ev_str, sizeof(ev_str), "%u", req_ev.id);
				first = 0;
			}

			/* Concat the supported event to the result string */
			size_t ev_str_len = strlen(ev_str);
			memcpy((*evs_supported)+evs_supported_len, ev_str, ev_str_len);
			evs_supported_len += ev_str_len;

			/* Set event enabled in the mask */
			mask |= (1 << i);
		}
	}

	// Free strtoat allocated memory
	free(ev_ids);

	// Terminate the resulting string
	(*evs_supported)[evs_supported_len++] = '\0';

	// Update the cache
	pool_cache_update(req_evs_str, *evs_supported, *req_evs_count, mask);

	return mask;
}

GPUPROF_F_EVENTS_SET(gpuprof_dcgmi_events_set)
{
	char command[512];
	uint ids_count;
	state_t s;
	int d;

	if (evs == NULL) {
		return;
	}

	// Filtering those requested events not supported

	// TODO: Cache requested events

	/* Allocate a string to put events filtered. The string will have
	 * at most the same size as the requested events list (evs). */
	char *evs_supported = malloc((strlen(evs) + 1) * sizeof(char));

	while (pthread_mutex_trylock(&lock)) ;

	// Update the requested/supported events mask
	event_pool_mask = filter_unsupported_events(evs, &ids_count, &evs_supported);
	if (!event_pool_mask) {
		debug("No events supported");
		goto leave1;
	}

	// Filling the pool

	// New popen every 2 seconds
	sprintf(command, "dcgmi dmon -e %s %s -d 2000 2>&1", evs_supported, command_ids);
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
	free(evs_supported);
	pthread_mutex_unlock(&lock);
}

static void unset()
{
	int d;
	popen_close(&p);
	for (d = 0; d < devs_count; ++d) {
		invalidate(pool, d);
	}
	event_pool_mask = 0;
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
			int pool_aux_idx = 0;
			for (int pool_idx = 0; pool_idx < pool[dev].values_count; ++pool_idx) {
				/* Check whether the event idx is enabled */
				if (event_pool_mask & (1 << pool_idx)) {
					debug
							("D%d_M%d: %04.2lf += %04.2lf, %0.lf samples, %p hash",
							 dev, pool_idx, pool[dev].values[pool_idx],
							 pool_aux_buffer[pool_aux_idx],
							 pool[dev].samples_count, pool[dev].hash);
					pool[dev].values[pool_idx] += pool_aux_buffer[pool_aux_idx++];
					} else {
						pool[dev].values[pool_idx] = 0;
					}
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

static unsigned char pool_cache_check(char *hash, char **evs_supported, uint *evs_count, short int *mask)
{
	for (int i = 0; i < DCGMI_POOL_CACHE_SIZE; i++) {
		if (!strcmp(hash, pool_cache.hashes[i])) {
			debug("Requested event list (%s) cached: %s", hash, pool_cache.hashes[i]);
			strcpy(*evs_supported, pool_cache.evs_supported[i]);
			*evs_count = pool_cache.hash_evs_count[i];
			*mask = pool_cache.masks[i];
			return 1;
		}
	}
	return 0;
}

static void pool_cache_update(char *hash, char *evs_supported, uint evs_count, short int mask)
{
	strcpy(pool_cache.hashes[pool_cache.dirty_pos], hash);
	strcpy(pool_cache.evs_supported[pool_cache.dirty_pos], evs_supported);
	pool_cache.hash_evs_count[pool_cache.dirty_pos] = evs_count;
	pool_cache.masks[pool_cache.dirty_pos] = mask;
	debug("Pool cache updated: pos: %hd mask: %hx hash: %s hash events count: %u supported events list: %s", pool_cache.dirty_pos, pool_cache.masks[pool_cache.dirty_pos], pool_cache.hashes[pool_cache.dirty_pos], pool_cache.hash_evs_count[pool_cache.dirty_pos], pool_cache.evs_supported[pool_cache.dirty_pos]);
	pool_cache.dirty_pos = (pool_cache.dirty_pos + 1) % DCGMI_POOL_CACHE_SIZE;
}
