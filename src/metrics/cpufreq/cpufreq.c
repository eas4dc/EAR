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
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/cpufreq/archs/eard.h>
#include <metrics/cpufreq/archs/dummy.h>
#include <metrics/cpufreq/archs/intel63.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static cpufreq_ops_t ops;
static uint cpu_count;
static uint api;

state_t cpufreq_load(topology_t *tp, int force_api)
{
	state_t s;
	while (pthread_mutex_trylock(&lock));
	if (api != API_NONE) {
		pthread_mutex_unlock(&lock);
		return EAR_SUCCESS;
	}
	api = API_DUMMY;
    if (API_IS(force_api, API_DUMMY)) {
        goto dummy;
    }
	if (state_ok(s = cpufreq_eard_status(tp, &ops, API_IS(force_api, API_EARD)))) {
		api = API_EARD;
		debug("Loaded EARD");
	}else{
		debug("Not loaded EARD: %s (%d)", state_msg, s);
	}
	if (state_ok(s = cpufreq_intel63_status(tp, &ops))) {
		api = API_INTEL63;
		debug("Loaded INTEL63");
	}else{
		debug("Not loaded INTEL63: %s (%d)", state_msg, s);
	}
dummy:
	if (state_ok(s = cpufreq_dummy_status(tp, &ops))) {}
	debug("Loaded metrics/cpufreq/DUMMY %p", ops.data_diff);
	cpu_count = tp->cpu_count;
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t cpufreq_get_api(uint *api_in)
{
	*api_in = api;
	return EAR_SUCCESS;
}

state_t cpufreq_init(ctx_t *c)
{
	preturn(ops.init, c);
}

state_t cpufreq_dispose(ctx_t *c)
{
	preturn(ops.dispose, c);
}

state_t cpufreq_count_devices(ctx_t *c, uint *dev_count)
{
	preturn(ops.count_devices, c, dev_count);
}

state_t cpufreq_read(ctx_t *c, cpufreq_t *ef)
{
	debug("cpufreq_read");
	preturn(ops.read, c, ef);
}

state_t cpufreq_read_diff(ctx_t *c, cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	state_t s;
	if (state_fail(s = cpufreq_read(c, f2))) {
		return s;
	}
	return cpufreq_data_diff(f2, f1, freqs, average);
}

state_t cpufreq_read_copy(ctx_t *c, cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	state_t s;
	if (state_fail(s = cpufreq_read_diff(c, f2, f1, freqs, average))) {
		return s;
	}
	return cpufreq_data_copy(f1, f2);
}

// Helpers
state_t cpufreq_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
	debug("cpufreq_data_diff %p", ops.data_diff);
	if (ops.data_diff != NULL){
		return ops.data_diff(f2, f1, freqs, average);
	}
	return EAR_SUCCESS;
}

state_t cpufreq_data_alloc(cpufreq_t **f, ulong **freqs)
{
	if (f != NULL) {
		*f = calloc(cpu_count, sizeof(cpufreq_t));
	}
	if (freqs != NULL) {
		*freqs = calloc(cpu_count, sizeof(ulong));
	}
	return EAR_SUCCESS;
}

state_t cpufreq_data_copy(cpufreq_t *dst, cpufreq_t *src)
{
	memcpy(dst, src, sizeof(cpufreq_t) * cpu_count);
	return EAR_SUCCESS;
}

state_t cpufreq_data_free(cpufreq_t **f, ulong **freqs)
{
	if (freqs != NULL && *freqs != NULL) {
		free(*freqs);
		*freqs = NULL;
	}
	if (f != NULL && *f != NULL) {
		free(*f);
		*f = NULL;
	}
	return EAR_SUCCESS;
}

void cpufreq_data_print(ulong *freqs, ulong average, int fd)
{
    char buffer[SZ_BUFFER];
    cpufreq_data_tostr(freqs, average, buffer, sizeof(buffer));
    dprintf(fd, "%s", buffer);
}

char *cpufreq_data_tostr(ulong *freqs, ulong average, char *buffer, size_t length)
{
    static int mod = 8;
    double freq_ghz;
    int acc = 0;
    int i;

    if (cpu_count >= 128) {
        mod = 16;
    }
    for (i = 0; i < cpu_count; ++i) {
        if ((i != 0) && (i % mod == 0)) {
            acc += sprintf(&buffer[acc], "\n");
        }
        freq_ghz = ((double) freqs[i]) / 1000000.0;
        acc += sprintf(&buffer[acc], "%0.1lf ", freq_ghz);
    }
    freq_ghz = ((double) average) / 1000000.0;
    acc += sprintf(&buffer[acc], "!%0.1lf", freq_ghz);
    //acc += sprintf(&buffer[acc], " (GHz)\n");
    return buffer;
}
