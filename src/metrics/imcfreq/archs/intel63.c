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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/imcfreq/archs/intel63.h>

#define GLBL_CMD_UNF    0x2000000000000000
#define UBOX_CMD_STA	0x400000 //
#define UBOX_CMD_STO	0x000000
#define SHIFT           22

// In this capacity, the UBox acts as the central unit for a variety of functions.
// Means just one register per socket?
// See 'Additional IMC Performance Monitoring' in 'Intel Xeon Processor Scalable
// Memory Family Uncore Performance Monitoring Reference Manual'
// Several unit counter control registers are still 32b, some 64b. All are
// addressable as 64b registers.
static off_t     address_global_ctl;
static ullong    command_global_ctl;
static ullong    mascara_global_ctl;
static off_t     address_unit_ctl;
static off_t     address_unit_ctr;
static ullong    command_unit_ctl;
static ullong    mascara_unit_ctl;
static topology_t tp;

void imcfreq_intel63_load(topology_t *tp_in, imcfreq_ops_t *ops)
{
    if (state_fail(imcfreq_intel63_ext_load_addresses(tp_in))) {
        return;
    }
    if (state_fail(msr_test(tp_in, MSR_WR))) {
        return;
    }
    if(state_fail(topology_select(tp_in, &tp, TPSelect.socket, TPGroup.merge, 0))) {
        return;
    }
    apis_set(ops->get_api,       imcfreq_intel63_get_api);
    apis_set(ops->init,          imcfreq_intel63_init);
    apis_set(ops->dispose,       imcfreq_intel63_dispose);
    apis_set(ops->count_devices, imcfreq_intel63_count_devices);
    apis_set(ops->read,          imcfreq_intel63_read);
    debug("Loaded Intel63");
}

void imcfreq_intel63_get_api(uint *api, uint *api_intern)
{
    *api = API_INTEL63;
    if (api_intern) {
        *api_intern = API_NONE;
    }
}

state_t imcfreq_intel63_ext_load_addresses(topology_t *tp)
{
    if (tp->vendor != VENDOR_INTEL) {
        debug("Detected vendor is not Intel");
        return EAR_ERROR;
    }
    // We don't know the details in Tiger Lake. Maybe the same SR addresses?
    if (tp->model == MODEL_TIGERLAKE) {
        debug("Detected vendor is Tiger Lake");
        return EAR_ERROR;
    }
    if (tp->model >= MODEL_SAPPHIRE_RAPIDS) {
        debug("Detected vendor is Sapphire Rapids");
        address_global_ctl = 0x2FF0; // Global Control
        command_global_ctl = 0x0000;
        mascara_global_ctl = 0x0000;
        address_unit_ctl   = 0x2FDE; // UCLK Counter Control
        address_unit_ctr   = 0x2FDF; // UCLK Counter
        command_unit_ctl   = 0x400000;
        mascara_unit_ctl   = 0xFFFFFFFFFFBFFFFF;
    } else {
        debug("Detected vendor is Haswell or greater");
        address_global_ctl = 0x0700; // U_MSR_PMON_GLOBAL_CTL
        command_global_ctl = 0x2000000000000000; // Bit 61
        mascara_global_ctl = 0x5FFFFFFFFFFFFFFF;
        address_unit_ctl   = 0x0703; // U_MSR_PMON_FIXED_CTL
        address_unit_ctr   = 0x0704; // U_MSR_PMON_FIXED_CTR
        command_unit_ctl   = 0x400000; // Bit 22
        mascara_unit_ctl   = 0xFFFFFFFFFFBFFFFF;
    }
    return EAR_SUCCESS;
}

static int fix_global_command(ullong *command)
{
    ullong mask = 0LLU;
    mask = *command & mascara_global_ctl;
    mask = mask | command_global_ctl;
    return 1;
}

state_t imcfreq_intel63_ext_enable_cpu(int cpu)
{
    ullong aux;
    state_t s;
    
    debug("CPU%d: unfreezing", cpu);
    if (state_fail(s = msr_open(cpu, MSR_WR))) {
        debug("msr_open failed: %s", state_msg);
        return s;
    }
    // Get global controller configuration
    if (state_fail(s = msr_read(cpu, &aux, sizeof(ullong), address_global_ctl))) {
        debug("U_MSR_PMON_GLOBAL_CTL%d: read failed '%s' (address 0x%lx)", cpu, state_msg, address_global_ctl);
        return s;
    }
    // Writting global controller configuration
    debug("U_MSR_PMON_GLOBAL_CTL%d: read 0x%llx (address 0x%lx)", cpu, aux, address_global_ctl);
    aux = (aux & mascara_global_ctl) | command_global_ctl;
    debug("U_MSR_PMON_GLOBAL_CTL%d: writting new command 0x%llx (address 0x%lx)", cpu, aux, address_global_ctl);
    if (state_fail(s = msr_write(cpu, &aux, sizeof(ullong), address_global_ctl))) {
        debug("U_MSR_PMON_GLOBAL_CTL%d: writing 0x%llx failed '%s' (address 0x%lx)", cpu, command_global_ctl, state_msg, address_global_ctl);
        return s;
    }
    // Get unit controller configuration
    if (state_fail(s = msr_read(cpu, &aux, sizeof(ullong), address_unit_ctl))) {
        debug("U_MSR_PMON_FIXED_CTL%d: read failed '%s' (address 0x%lx)", cpu, state_msg, address_unit_ctl);
        return s;
    }
    // Writting unit controllar configuration
    aux = (aux & mascara_unit_ctl) | command_unit_ctl; 
    debug("U_MSR_PMON_FIXED_CTL%d: writting new command 0x%llx (address 0x%lx)", cpu, aux, address_unit_ctl);
    if (state_fail(s = msr_write(cpu, &aux, sizeof(ullong), address_unit_ctl))) {
        debug("U_MSR_PMON_FIXED_CTL%d: write failed '%s' (address 0x%lx)", cpu, state_msg, address_unit_ctl);
        return s;
    }
    return EAR_SUCCESS;
}

state_t imcfreq_intel63_ext_read_cpu(int cpu, ulong *freq)
{
    return msr_read(cpu, freq, sizeof(ulong), address_unit_ctr);
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
		if (state_fail(s = imcfreq_intel63_ext_enable_cpu(tp.cpus[cpu].id))) {
			return s;
		}
	}
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
		i[cpu].time = i[0].time;
		if (state_fail(s = imcfreq_intel63_ext_read_cpu(tp.cpus[cpu].id, &i[cpu].freq))) {
			return s;
		}
        debug("U_MSR_PMON_FIXED_CTR%d: read %lu (address 0x%lx)", tp.cpus[cpu].id, i[cpu].freq, address_unit_ctr);
		i[cpu].error = state_fail(s);
	}

	return EAR_SUCCESS;
}
