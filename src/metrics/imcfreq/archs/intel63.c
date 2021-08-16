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

//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/imcfreq/archs/intel63.h>

#define UBOX_CTR_OFS	0x000704
#define UBOX_CTL_OFS	0x000703
#define UBOX_CMD_STA	0x400000
#define UBOX_CMD_STO	0x000000
#define SHIFT           22

// In this capacity, the UBox acts as the central unit for a variety of functions.
// Means just one register per socket?
// See 'Additional IMC Performance Monitoring' in 'Intel Xeon Processor Scalable
// Memory Family Uncore Performance Monitoring Reference Manual'

static topology_t tp;

void imcfreq_intel63_load(topology_t *tp_in, imcfreq_ops_t *ops)
{
	debug("loading");
	if (tp_in->vendor != VENDOR_INTEL || tp_in->model < MODEL_HASWELL_X) {
		return;
	}
	debug("testing MSR");
	if (state_fail(msr_test(tp_in))) {
		return;
	}
	debug("testing MSR finalized");
	if(state_fail(topology_select(tp_in, &tp, TPSelect.socket, TPGroup.merge, 0))) {
		return;
	}

	apis_set(ops->get_api,       imcfreq_intel63_get_api);
	apis_set(ops->init,          imcfreq_intel63_init);
	apis_set(ops->dispose,       imcfreq_intel63_dispose);
	apis_set(ops->count_devices, imcfreq_intel63_count_devices);
	apis_set(ops->read,          imcfreq_intel63_read);

	debug("INTEL63 loaded full API");
}

void imcfreq_intel63_get_api(uint *api, uint *api_intern)
{
	*api = API_INTEL63;
	if (api_intern) {
		*api_intern = API_NONE;
	}
}

static state_t imcfreq_intel63_enable()
{
	ulong com, com1;
	state_t s;
	int cpu;

	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		if (state_fail(s = msr_read(tp.cpus[cpu].id, &com, sizeof(ulong), UBOX_CTL_OFS))) {
			return s;
		}
		com1 = ((com & UBOX_CMD_STA) >> SHIFT);
		if (!com1) {
			com = com | UBOX_CMD_STA;
			if (state_fail(s = msr_write(tp.cpus[cpu].id, &com, sizeof(ulong), UBOX_CTL_OFS))) {
				return s;
			}
		}
	}
	return EAR_SUCCESS;
}

state_t imcfreq_intel63_init(ctx_t *c)
{
	static int init = 0;
	state_t s;
	int cpu;
	
	if (init) {
		return EAR_SUCCESS;
	}	
	// Opening MSR
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		if (state_fail(s = msr_open(tp.cpus[cpu].id))) {
			return s;
		}
	}
	debug("INTEL63 opened MSRs correctly");
	#if 1
	// Enabling
	if (state_fail(s = imcfreq_intel63_enable())) {
		return s;
	}
	#endif
	debug("INTEL63 MSRs enabled correctly");
	init = 1;

	return EAR_SUCCESS;
}

state_t imcfreq_intel63_dispose(ctx_t *c)
{
	int cpu;
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		msr_close(tp.cpus[cpu].id);
	}
	return EAR_SUCCESS;
}

state_t imcfreq_intel63_count_devices(ctx_t *c, uint *dev_count)
{
	if (dev_count == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	*dev_count = tp.cpu_count;
	return EAR_SUCCESS;
}

static void data_clean(imcfreq_t *i)
{
	int cpu;
	// Cleaning
	memset(i, 0, tp.cpu_count * sizeof(imcfreq_t));
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		i[cpu].error = 1;
	}
}

state_t imcfreq_intel63_read(ctx_t *c, imcfreq_t *i)
{
	//int enabled;
	state_t s;
	int cpu;

	if (i == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	// Cleaning
	data_clean(i);
	// Time is required to compute hertzs.
	timestamp_getfast(&i[0].time);
	// Iterating per socket.
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		debug("Reading IMC MSR for device %d",tp.cpus[cpu].id);
		i[cpu].time = i[0].time;
		if ((s = msr_read(tp.cpus[cpu].id, &i[cpu].freq, sizeof(ulong), UBOX_CTR_OFS)) != EAR_SUCCESS) {
			return s;
		}
		debug("Freq for cpu %d is %lu",tp.cpus[cpu].id,i[cpu].freq);
		i[cpu].error = state_fail(s);
	}

	return EAR_SUCCESS;
}
