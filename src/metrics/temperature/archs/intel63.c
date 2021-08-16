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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#include <stdlib.h>
#include <pthread.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/temperature/archs/intel63.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static topology_t tp;

#define IA32_THERM_STATUS               0x19C
#define IA32_PKG_THERM_STATUS           0x1B1
#define MSR_TEMPERATURE_TARGET          0x1A2

state_t temp_intel63_status(topology_t *_tp)
{
	state_t s = EAR_SUCCESS;
	if (_tp->vendor != VENDOR_INTEL || _tp->model < MODEL_HASWELL_X) {
		return EAR_ERROR;
	}
	while (pthread_mutex_trylock(&lock));
	if (tp.cpus == NULL) {
		s = topology_select(_tp, &tp, TPSelect.socket, TPGroup.merge, 0);
	}
	pthread_mutex_unlock(&lock);
	return s;
}

state_t temp_intel63_init(ctx_t *c)
{
	llong *throt;
	llong data;
	state_t s;
	int i;

	//
	if (c == NULL) {
        return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
    }
	if ((c->context = calloc(tp.cpu_count, sizeof(ullong))) == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
	//
	throt = (llong *) c->context;
	for (i = 0; i < tp.cpu_count; ++i) {
		debug("accessing to CPU %d", tp.cpus[i].id);
		if (xtate_fail(s, msr_open(tp.cpus[i].id))) {
			return s;
		}
		if (xtate_fail(s, msr_read(tp.cpus[i].id, &data, sizeof(llong), MSR_TEMPERATURE_TARGET))) {
			return s;
		}
		throt[i] = (data >> 16);
	}
	//
	return EAR_SUCCESS;
}

static state_t get_context(ctx_t *c, llong **h)
{
	if (c == NULL || c->context == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	*h = (llong *) c->context;
	return EAR_SUCCESS;
}

state_t temp_intel63_dispose(ctx_t *c)
{
	llong *throt;
	state_t s;
	int i;

	if (xtate_fail(s, get_context(c, &throt))) {
		return s;
	}
	for (i = 0; i < tp.cpu_count; ++i) {
		if (xtate_fail(s, msr_close(tp.cpus[i].id))) {
			return s;
		}
	}
	c->context = NULL;
	free(throt);
	return EAR_SUCCESS;
}

// Data
state_t temp_intel63_count_devices(ctx_t *c, uint *count)
{
	llong *throt;
	state_t s;
	if (xtate_fail(s, get_context(c, &throt))) {
		return s;
	}
	if (count != NULL) {
		*count = tp.cpu_count;
	}
	return EAR_SUCCESS;
}

// Getters
state_t temp_intel63_read(ctx_t *c, llong *temp, llong *average)
{
	llong aux1, aux2;
	llong *throt;
	llong data;
	state_t s;
	int i;

	if (xtate_fail(s, get_context(c, &throt))) {
		return s;
	}
	for (i = 0, aux1 = aux2 = 0; i < tp.cpu_count && temp != NULL; ++i) {
        temp[i] = 0;
    }
	for (i = 0; i < tp.cpu_count; ++i) {
		if (xtate_fail(s, msr_read(tp.cpus[i].id, &data, sizeof(llong), IA32_PKG_THERM_STATUS))) {
			return s;
		}
		debug("read %s", data);
		aux1  = throt[i] - ((data >> 16) & 0xff);
		aux2 += aux1;
		if (temp != NULL) {
			temp[i] = aux1;
		}
	}
    if (average != NULL) {
        *average = aux2 / (llong) tp.cpu_count;
    }

	return EAR_SUCCESS;
}
