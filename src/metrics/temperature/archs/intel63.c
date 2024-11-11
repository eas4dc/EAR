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

#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/temperature/archs/intel63.h>

#define IA32_THERM_STATUS               0x19C
#define IA32_PKG_THERM_STATUS           0x1B1
#define MSR_TEMPERATURE_TARGET          0x1A2

static topology_t self_tp;
static llong     *throt;

TEMP_F_LOAD(intel63)
{
	state_t s = EAR_SUCCESS;
    ullong data;
    int i;

	if (tp->vendor != VENDOR_INTEL || tp->model < MODEL_HASWELL_X) {
		return;
	}
    if (state_fail(s = msr_test(tp, MSR_RD))) {
        return;
    }
    topology_select(tp, &self_tp, TPSelect.socket, TPGroup.merge, 0);
    // New bits
	for (i = 0; i < self_tp.cpu_count; ++i) {
		if (state_fail(s = msr_open(self_tp.cpus[i].id, MSR_RD))) {
			return;
		}
		if (state_fail(s = msr_read(self_tp.cpus[i].id, &data, sizeof(ullong), IA32_THERM_STATUS))) {
			return;
		}
        debug("IA32_THERM_STATUS of CPU%d bits: 0x%llx", self_tp.cpus[i].id, data);
    }
    apis_set(ops->get_info, temp_intel63_get_info);
    apis_set(ops->init,     temp_intel63_init);
    apis_set(ops->dispose,  temp_intel63_dispose);
    apis_set(ops->read,     temp_intel63_read);
}

TEMP_F_GET_INFO(intel63)
{
    info->api         = API_INTEL63;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = self_tp.cpu_count;
}

TEMP_F_INIT(intel63)
{
	llong data;
	state_t s;
	int i;

	throt = calloc(self_tp.cpu_count, sizeof(ullong));
    //
	for (i = 0; i < self_tp.cpu_count; ++i) {
		if (state_fail(s = msr_read(self_tp.cpus[i].id, &data, sizeof(llong), MSR_TEMPERATURE_TARGET))) {
			return s;
		}
        debug("MSR_TEMPERATURE_TARGET of CPU%d bits: 0x%llx", self_tp.cpus[i].id, data);
        // In the future we might have problems. Because 23:16 is temperature target (8 bits).
        // But target offset is 29:24 (6 bits). And the PROCHOT is target + offset, but by now
        // we always have found the offset is 0 (lucky).
		throt[i] = (data >> 16) & 0xFF;
	}
	return EAR_SUCCESS;
}

TEMP_F_DISPOSE(intel63)
{
    // Empty
}

TEMP_F_READ(intel63)
{
    llong aux1 = 0LL;
    llong aux2 = 0LL;
	llong data = 0LL;
	state_t s;
	int i;

	for (i = 0; i < self_tp.cpu_count && temp != NULL; ++i) {
        temp[i] = 0;
    }
	for (i = 0; i < self_tp.cpu_count; ++i) {
		if (state_fail(s = msr_read(self_tp.cpus[i].id, &data, sizeof(llong), IA32_PKG_THERM_STATUS))) {
			return s;
		}
        debug("IA_PKG_THERM_STATUS of CPU%d bits: 0x%llx", self_tp.cpus[i].id, data);
        // Data is from bits 22:16, then there are 7 bits
		aux1  = throt[i] - ((data >> 16) & 0x7F);
		aux2 += aux1;
		if (temp != NULL) {
			temp[i] = aux1;
		}
	}
    if (average != NULL) {
        *average = aux2 / (llong) self_tp.cpu_count;
    }

	return EAR_SUCCESS;
}
