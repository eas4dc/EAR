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
#include <common/sizes.h>
#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/temperature/archs/intel63.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static topology_t tp;
static llong *throt;

#define IA32_THERM_STATUS               0x19C
#define IA32_PKG_THERM_STATUS           0x1B1
#define MSR_TEMPERATURE_TARGET          0x1A2

state_t temp_intel63_status(topology_t *_tp)
{
	state_t s = EAR_SUCCESS;
    ullong data;
    int i;

	if (_tp->vendor != VENDOR_INTEL || _tp->model < MODEL_HASWELL_X) {
		return EAR_ERROR;
	}
	while (pthread_mutex_trylock(&lock));
	if (!tp.initialized) {
        if (state_fail(s = msr_test(_tp, MSR_RD))) {
            return EAR_ERROR;
        }
		s = topology_select(_tp, &tp, TPSelect.socket, TPGroup.merge, 0);
	}
    // New bits
	for (i = 0; i < tp.cpu_count; ++i) {
		if (state_fail(s = msr_open(tp.cpus[i].id, MSR_RD))) {
			return s;
		}
		if (state_fail(s = msr_read(tp.cpus[i].id, &data, sizeof(ullong), IA32_THERM_STATUS))) {
			return s;
		}
        debug("IA32_THERM_STATUS of CPU%d bits: 0x%llx", tp.cpus[i].id, data);
    }
	pthread_mutex_unlock(&lock);
	return s;
}

state_t temp_intel63_init(ctx_t *c)
{
	llong data;
	state_t s;
	int i;

	//
	if ((throt = calloc(tp.cpu_count, sizeof(ullong))) == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
	//
	for (i = 0; i < tp.cpu_count; ++i) {
		if (state_fail(s = msr_read(tp.cpus[i].id, &data, sizeof(llong), MSR_TEMPERATURE_TARGET))) {
			return s;
		}
        debug("MSR_TEMPERATURE_TARGET of CPU%d bits: 0x%llx", tp.cpus[i].id, data);
        // In the future we might have problems. Because 23:16 is temperature target (8 bits).
        // But target offset is 29:24 (6 bits). And the PROCHOT is target + offset, but by now
        // we always have found the offset is 0 (lucky).
		throt[i] = (data >> 16) & 0xFF;
	}
	//
	return EAR_SUCCESS;
}

state_t temp_intel63_dispose(ctx_t *c)
{
	state_t s;
	int i;

	for (i = 0; i < tp.cpu_count; ++i) {
		if (state_fail(s = msr_close(tp.cpus[i].id))) {
			return s;
		}
	}
	free(throt);
	return EAR_SUCCESS;
}

// Data
state_t temp_intel63_count_devices(ctx_t *c, uint *count)
{
    if (count != NULL) {
		*count = tp.cpu_count;
	}
	return EAR_SUCCESS;
}

// Getters
state_t temp_intel63_read(ctx_t *c, llong *temp, llong *average)
{
	llong aux1, aux2;
	llong data;
	state_t s;
	int i;

	for (i = 0, aux1 = aux2 = 0; i < tp.cpu_count && temp != NULL; ++i) {
        temp[i] = 0;
    }
	for (i = 0; i < tp.cpu_count; ++i) {
		if (state_fail(s = msr_read(tp.cpus[i].id, &data, sizeof(llong), IA32_PKG_THERM_STATUS))) {
			return s;
		}
        debug("IA_PKG_THERM_STATUS of CPU%d bits: 0x%llx", tp.cpus[i].id, data);
        // Data is from bits 22:16, then there are 7 bits
		aux1  = throt[i] - ((data >> 16) & 0x7F);
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
