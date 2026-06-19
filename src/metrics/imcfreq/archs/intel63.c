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

#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/imcfreq/archs/intel63.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GLBL_CMD_UNF 0x2000000000000000
#define UBOX_CMD_STA 0x400000 //
#define UBOX_CMD_STO 0x000000
#define SHIFT        22
// See 'Additional IMC Performance Monitoring' in 'Intel Xeon Processor Scalable
// Memory Family Uncore Performance Monitoring Reference Manual'. Several unit
// counter control registers are still 32b, some 64b. All are addressable as 64b
// registers.
static off_t address_global_ctl;
static off_t address_unit_ctl;
static off_t address_unit_ctr;
static ullong command_global_ctl;
static ullong command_unit_ctl;
static ullong mask_global_ctl;
static ullong mask_unit_ctl;
static topology_t tp;

static void close_msrs(int max_cpu)
{
    int cpu;
    for (cpu = 0; cpu < max_cpu; ++cpu) {
        msr_close(tp.cpus[cpu].id);
    }
}

IMCFREQ_F_LOAD(intel63)
{
    state_t s;
    int cpu;

    if (state_fail(msr_test(tp_in, MSR_WR))) {
        return;
    }
    topology_select(tp_in, &tp, TPSelect.socket, TPGroup.merge, 0);
    // Opening MSR and setting addresses
    if (state_fail(imcfreq_intel63_ext_load_addresses(tp_in))) {
        return;
    }
    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
        if (state_fail(s = msr_open(cpu, MSR_WR))) {
            debug("msr_open failed: %s", state_msg);
            close_msrs(cpu);
            return;
        }
    }
    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
        if (state_fail(imcfreq_intel63_ext_enable_cpu(tp.cpus[cpu].id))) {
            close_msrs(tp.cpu_count);
            return;
        }
    }
    apis_put(ops->unload, imcfreq_intel63_unload);
    apis_put(ops->get_info, imcfreq_intel63_get_info);
    apis_set(ops->read, imcfreq_intel63_read);
    debug("Loaded Intel63");
}

IMCFREQ_F_UNLOAD(intel63)
{
    close_msrs(tp.cpu_count);
    topology_close(&tp);
}

IMCFREQ_F_GET_INFO(intel63)
{
    info->api         = API_INTEL63;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = tp.cpu_count;
}

IMCFREQ_F_READ(intel63)
{
    state_t s;
    int cpu;
    // Cleaning
    memset(list, 0, tp.cpu_count * sizeof(imcfreq_t));
    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
        list[cpu].error = 1;
    }
    // Time is required to compute hertzs.
    timestamp_getfast(&list[0].time);
    // Iterating per socket.
    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
        list[cpu].time = list[0].time;
        if (state_fail(s = imcfreq_intel63_ext_read_cpu(tp.cpus[cpu].id, &list[cpu].freq))) {
            return s;
        }
        debug("U_MSR_PMON_FIXED_CTR%d: read %lu (address 0x%lx)", tp.cpus[cpu].id, list[cpu].freq, address_unit_ctr);
        list[cpu].error = state_fail(s);
    }
    return EAR_SUCCESS;
}

/*
 *
 * External functions
 *
 */

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
        mask_global_ctl    = 0x0000;
        address_unit_ctl   = 0x2FDE; // UCLK Counter Control
        address_unit_ctr   = 0x2FDF; // UCLK Counter
        command_unit_ctl   = 0x400000;
        mask_unit_ctl      = 0xFFFFFFFFFFBFFFFF;
    } else {
        debug("Detected vendor is Haswell or greater");
        address_global_ctl = 0x0700;             // U_MSR_PMON_GLOBAL_CTL
        command_global_ctl = 0x2000000000000000; // Bit 61
        mask_global_ctl    = 0x5FFFFFFFFFFFFFFF;
        address_unit_ctl   = 0x0703;   // U_MSR_PMON_FIXED_CTL
        address_unit_ctr   = 0x0704;   // U_MSR_PMON_FIXED_CTR
        command_unit_ctl   = 0x400000; // Bit 22
        mask_unit_ctl      = 0xFFFFFFFFFFBFFFFF;
    }
    return EAR_SUCCESS;
}

state_t imcfreq_intel63_ext_enable_cpu(int cpu)
{
    ullong aux;
    state_t s;

    debug("CPU%d: unfreezing", cpu);
    // Get global controller configuration
    if (state_fail(s = msr_read(cpu, &aux, sizeof(ullong), address_global_ctl))) {
        debug("U_MSR_PMON_GLOBAL_CTL%d: read failed '%s' (address 0x%lx)", cpu, state_msg, address_global_ctl);
        return s;
    }
    // Writting global controller configuration
    debug("U_MSR_PMON_GLOBAL_CTL%d: read 0x%llx (address 0x%lx)", cpu, aux, address_global_ctl);
    aux = (aux & mask_global_ctl) | command_global_ctl;
    debug("U_MSR_PMON_GLOBAL_CTL%d: writting new command 0x%llx (address 0x%lx)", cpu, aux, address_global_ctl);
    if (state_fail(s = msr_write(cpu, &aux, sizeof(ullong), address_global_ctl))) {
        debug("U_MSR_PMON_GLOBAL_CTL%d: writing 0x%llx failed '%s' (address 0x%lx)", cpu, command_global_ctl, state_msg,
              address_global_ctl);
        return s;
    }
    // Get unit controller configuration
    if (state_fail(s = msr_read(cpu, &aux, sizeof(ullong), address_unit_ctl))) {
        debug("U_MSR_PMON_FIXED_CTL%d: read failed '%s' (address 0x%lx)", cpu, state_msg, address_unit_ctl);
        return s;
    }
    // Writting unit controllar configuration
    aux = (aux & mask_unit_ctl) | command_unit_ctl;
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
