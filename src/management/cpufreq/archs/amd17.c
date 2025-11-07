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

#define _GNU_SOURCE
#include <math.h>
#include <sched.h>
#include <stdlib.h>
#include <common/output/debug.h>
#include <common/utils/keeper.h>
#include <common/string_enhanced.h>
#include <common/hardware/bithack.h>
#include <common/environment_common.h>
#include <management/cpufreq/archs/amd17.h>
#include <metrics/common/msr.h>
#include <metrics/common/apis.h>
#include <metrics/common/hsmp.h>

typedef struct pss_s {
    ullong fid;
    ullong did;
    ullong cof;
    ullong mul;
} pss_t;

#define MAX_PSTATES       128
#define MAX_REGISTERS     8
#define REG_HWCONF        0xc0010015 // MSRC001_0015 Hardware Configuration (HWCR)
#define REG_LIMITS        0xc0010061
#define REG_CONTROL       0xc0010062
#define REG_STATUS        0xc0010063
#define REG_P0            0xc0010064
#define REG_P1            0xc0010065

#define p0_khz(cof)       ((cof * 1000LLU) + 1000LLU)
#define p1_khz(cof)       ((cof * 1000LLU))

#define READ_BOOST_LIMIT  0x0A // gets the frequency limit currently enforced or Fmax
#define WRITE_BOOST_LIMIT 0x08 // sets the frequency limit

static mgt_ps_driver_ops_t *driver;
static topology_t tp;
static ullong     regs[MAX_REGISTERS];
static pss_t      psss[MAX_PSTATES];
static uint       boost_enabled;
static uint       pss_nominal;
static uint       pss_count;
static uint       init;

//
static state_t set_pstate(uint cpu, uint ps);
static state_t set_pstate0(uint cpu);

// Modes:
//  - HSMP: using HSMP mailing system to change frequency. You can select
//  whatever frequency, even limit boost frequency, in the CPU you want. It has
//  no limits.
//  - Virtual: it changes P1 MSR register. You are limited to P0 and P1 per CPU,
//  and P1 frequency value is shared between all CPUs in a socket. It has a lot
//  of limitations. UPDATE: CURRENTLY VIRTUAL IS DELETED! It was not used
//  because 'default' API worked well and it is safer to use CPUFreq driver than
//  writing MSRs directly. We also found that only worked for ZEN1 and ZEN2
//  families because in ZEN3 there wwere too much differentiation between models
//  and its bits configuration.

state_t mgt_cpufreq_amd17_load(topology_t *tp_in, mgt_ps_ops_t *ops, mgt_ps_driver_ops_t *ops_driver)
{
    state_t s;
    int cond2;

    debug("testing AMD17 P_STATE control status");
    #if AMD_OSCPUFREQ
    return EAR_ERROR;
    #endif
    if (tp_in->vendor != VENDOR_AMD) {
        return_msg(EAR_ERROR, Generr.api_incompatible);
    }
    if (tp_in->family < FAMILY_ZEN) {
        return_msg(EAR_ERROR, Generr.api_incompatible);
    }
    // Driver is used to many things, without it this API can't do anything by now
    if (ops_driver->init == NULL) {
        return_msg(EAR_ERROR, "Driver is not available");
    }
    driver = ops_driver;
    // This is also an MSR class, without MSR this API can't do anything
    if (state_fail(s = msr_test(tp_in, MSR_WR))) {
        return s;
    }
    if (tp.cpu_count == 0) {
        if (state_fail(s = topology_copy(&tp, tp_in))) {
            return s;
        }
    }
    if (state_fail(s = hsmp_open(tp_in, HSMP_WR))) {
        return s;
    }
    // Conditional 1, if can set frequencies (old, with hsmp is always possible)
    // Conditional 2, if can set governor
    cond2 = (driver->set_governor != NULL);
    // Setting references (if driver set is not available, this API neither)
    apis_put(ops->init             , mgt_cpufreq_amd17_init);
    apis_put(ops->dispose          , mgt_cpufreq_amd17_dispose);
    apis_put(ops->get_info         , mgt_cpufreq_amd17_get_info);
    apis_put(ops->count_available  , mgt_cpufreq_amd17_count_available);
    apis_put(ops->get_available_list, mgt_cpufreq_amd17_get_available_list);
    apis_put(ops->get_current_list , mgt_cpufreq_amd17_get_current_list);
    apis_put(ops->get_nominal      , mgt_cpufreq_amd17_get_nominal);
    apis_put(ops->get_index        , mgt_cpufreq_amd17_get_index);
    apis_put(ops->set_current_list , mgt_cpufreq_amd17_set_current_list);
    apis_put(ops->set_current      , mgt_cpufreq_amd17_set_current);
    apis_put(ops->reset            , mgt_cpufreq_amd17_reset);
    apis_put(ops->get_governor     , mgt_cpufreq_amd17_governor_get);
    apis_put(ops->get_governor_list, mgt_cpufreq_amd17_governor_get_list);
    apis_pin(ops->set_governor     , mgt_cpufreq_amd17_governor_set     , cond2);
    apis_pin(ops->set_governor_mask, mgt_cpufreq_amd17_governor_set_mask, cond2);
    apis_pin(ops->set_governor_list, mgt_cpufreq_amd17_governor_set_list, cond2);
    return EAR_SUCCESS;
}

static state_t static_dispose(uint close_msr_up_to, state_t s, char *msg)
{
    int cpu;
    for (cpu = 0; cpu < close_msr_up_to; ++cpu) {
        msr_close(tp.cpus[cpu].id);
    }
    if (driver != NULL) {
        driver->dispose();
    }
    init = 0;
    return_msg(s, msg);
}

state_t mgt_cpufreq_amd17_dispose(ctx_t *c)
{
    return static_dispose(tp.cpu_count, EAR_SUCCESS, NULL);
}

#if SHOW_DEBUGS
static void print_registers()
{
    int i;

    // Look into 'PPR 17h, Models 31h.pdf' the MSRC001_006[4...B] register information.
    debug("-----------------------------------------------------------------------------------");
    tprintf_init(STDERR_FILENO, STR_MODE_DEF, "4 18 4 8 8 11 8 7 8 10 10");
    tprintf("#||reg||en||IddDiv||IddVal||IddPerCore|||CpuVid|||CpuFid||  Mul||CpuDfsId||CoreCof");
    tprintf("-||---||--||------||------||----------|||------|||------||-----||--------||-------");

    for (i = 0; i < MAX_REGISTERS; ++i) {
        // FAMILY_ZEN
        ullong en        = getbits64(regs[i], 63, 63);
        ullong idd_div   = getbits64(regs[i], 31, 30);
        ullong idd_value = getbits64(regs[i], 29, 22);
        double idd_core  = ((double) idd_value) / pow(10.0, (double) idd_div);
        ullong vdd_cpu   = getbits64(regs[i], 21, 14); // CpuVid
        // double pow_core  = 1.350 - (((double) CpuVid) * 0.00625);
        ullong freq_did  = getbits64(regs[i], 13,  8); // CpuDfsId
        ullong freq_fid  = getbits64(regs[i],  7,  0); // CpuFid
        ullong freq_mul  = 200LLU;
        ullong freq_cof  = 0LLU;
        // In FAMILY_ZEN3 or 19 documents there is a contradiction. It says
        // both things:
        // - CpuFid[7:0]: core frequency ID. Specifies the core frequency
        //   multiplier. The core COF is defined as CpuFid * 5MHz.
        // - CoreCOF = (PStateDef[CpuFid[7:0]]/PStateDef[CpuDfsId])*200.
        if (MODEL_IS_ZEN4(tp.model) || tp.family >= FAMILY_ZEN5) {
            vdd_cpu  = getbits64(regs[i], 21, 14) | (getbits64(regs[i], 32, 32) << 8);
        }
        if (tp.family >= FAMILY_ZEN5) {
		    freq_fid = getbits64(regs[i], 11,  0);
            freq_mul = 5LLU;
            freq_did = 1LLU;
        }
        if (freq_did > 0) {
            freq_cof = (freq_fid * freq_mul) / freq_did;
        }
        tprintf("%d||%LX||%llu||%llu||%llu||%0.2lf|||%llu|||%llu||* %llu||/ %llu||= %llu", i, regs[i], en, idd_div,
                idd_value, idd_core, vdd_cpu, freq_fid, freq_mul, freq_did, freq_cof);
    }
    debug("-----------------------------------------------------------------------------------");
}
#endif

static void print_pstate_list()
{
    int i;
    debug("Final PSS object list: ");
    for (i = 0; i < pss_count; ++i) {
        debug("P%d: %llu MHz", i, psss[i].cof);
    }
}

static state_t build_pstate_list()
{
    const ullong *freqs_available;
    uint freq_count;
    ullong min_mhz;
    state_t s;
    ullong p;
    int i;

    // Getting the minimum frequency specified by the driver. In the past it was 1000.
    if (state_fail(s = driver->get_available_list(&freqs_available, &freq_count))) {
        return static_dispose(0, s, state_msg);
    }
    min_mhz = freqs_available[freq_count - 1] / 1000LLU;
    // Getting P0(b) information from the first of a list of registers. Registers are read
    // during init phase.
    psss[0].did = getbits64(regs[0], 13, 8); // CpuDfsId
    psss[0].fid = getbits64(regs[0],  7, 0); // CpuFid
    psss[0].mul = 200LLU;
    if (tp.family >= FAMILY_ZEN5) {
        psss[0].fid = getbits64(regs[0], 11, 0);
        psss[0].did = 1LLU;
        psss[0].mul = 5LLu;
    }
    if (psss[0].did == 0) {
        return_msg(EAR_ERROR, "the frequency divisor is 0");
    }
    // Getting the P0(b) Core Frequency
    psss[0].cof = (psss[0].fid * psss[0].mul) / psss[0].did;
    // If boost is enabled, P0 is copied in P1. P1 then will have the same COF.
    i = 1;
    if (boost_enabled) {
        psss[1].cof = psss[0].cof;
        psss[1].fid = psss[0].fid;
        psss[1].did = psss[0].did;
        psss[1].mul = psss[0].mul;
        i           = 2;
    }
    // Getting P2 (or P1 if boost is not enabled) Core Frequency (intervals of 100 MHz).
    if ((psss[0].cof % 100LLU) != 0) {
        p = psss[0].cof + (100LLU - (psss[0].cof % 100LLU)) - 100LLU;
    } else {
        p = psss[0].cof - 100LLU;
    }
    // Getting PSS objects from P1 to P7
    for (; p >= min_mhz && i < MAX_PSTATES; p -= 100, ++i) {
        psss[i].cof = p;
        debug("BUILT PS%d: %llu MHz", i, psss[i].cof);
    }
    pss_count = i;
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_amd17_init(ctx_t *c)
{
    ullong data;
    state_t s;
    // uint aux;
    int cpu;
    int i;

    debug("initializing AMD17 P_STATE control");
    if (state_fail(s = driver->init())) {
        return static_dispose(tp.cpu_count, s, state_msg);
    }
    // Opening MSRs (switch to user mode if permission denied)
    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
			  debug("Trying to open MSR for CPU%d", tp.cpus[cpu].id);
        if (state_fail(s = msr_open(tp.cpus[cpu].id, MSR_WR))) {
            return static_dispose(cpu, s, state_msg);
        }
    }
    // Reading min and max P_STATEs (by MSR, not by CPUFREQ, but it is supposed to be the same).
    if (state_fail(s = msr_read(tp.cpus[0].id, &data, sizeof(ullong), REG_LIMITS))) {
        return static_dispose(tp.cpu_count, s, state_msg);
    }
    #if EXCESIVE_ROBUSTNESS
    // Find maximum and minimum P_STATE
    ullong pstate_max = getbits64(data, 2, 0); // P_STATE max ex: P0
    ullong pstate_min = getbits64(data, 6, 4); // P_STATE min ex: P7
    debug("PSTATE_MAX: %llu", pstate_max);
    debug("PSTATE_MIN: %llu", pstate_min);
    if (pstate_max > 1LLU || pstate_min < 1LLU) {
        return static_dispose(tp.cpu_count, EAR_ERROR, "Incorrect P_STATE limits");
    }
    #endif
    // Save the 8 registers
    for (data = 0LLU, i = MAX_REGISTERS - 1; i >= 0; --i) {
        if (state_fail(s = msr_read(0, &regs[i], sizeof(ullong), REG_P0 + ((ullong) i)))) {
            return static_dispose(tp.cpu_count, s, state_msg);
        }
        debug("Read P%d register: %llu", i, regs[i]);
        if (getbits64(regs[i], 63, 63) == 1LLU) {
            data = regs[i];
        }
    }
    // Keeper will keep any messed configuration
    if (!keeper_load_uint64("Amd17PstateReg0", &regs[0])) {
        keeper_save_uint64("Amd17PstateReg0", regs[0]);
    } else {
        debug("Read P0 from keeper: %llu", regs[0]);
    }
    if (!keeper_load_uint64("Amd17PstateReg1", &regs[1])) {
        keeper_save_uint64("Amd17PstateReg1", regs[1]);
    } else {
        debug("Read P1 from keeper: %llu", regs[1]);
    }
    #if SHOW_DEBUGS
    print_registers();
    #endif
    // Boost enabled checked by MSR.
    if (state_fail(s = msr_read(0, &data, sizeof(ullong), REG_HWCONF))) {
        return static_dispose(tp.cpu_count, s, state_msg);
    }
    boost_enabled = (uint) !getbits64(data, 25, 25);
    pss_nominal   = boost_enabled;
    // Building PSS object list. Is built through the P0 MSR register. But
    // it can be done using the frequency returned by the driver.
    if (state_fail(s = build_pstate_list())) {
        return static_dispose(tp.cpu_count, s, state_msg);
    }
    //
    print_pstate_list();
    debug("num P_STATEs: %d", pss_count);
    debug("nominal P_STATE: P%d", pss_nominal);
    debug("boost enabled: %d", boost_enabled);
    init = 1;
    return EAR_SUCCESS;
}

void mgt_cpufreq_amd17_get_info(apinfo_t *info)
{
    info->api        = API_AMD17;
    info->devs_count = tp.cpu_count;
}

void mgt_cpufreq_amd17_get_freq_details(freq_details_t *details)
{
    // Driver is always present since is required for the load. But frequency
    // details are not guaranteed.
    if (driver->get_freq_details != NULL) {
        driver->get_freq_details(details);
    }
}

state_t mgt_cpufreq_amd17_count_available(ctx_t *c, uint *pstate_count)
{
    *pstate_count = pss_count;
    return EAR_SUCCESS;
}

static state_t search_pstate_list(ullong freq_khz, uint *pstate_index, uint closest)
{
    ullong cof_khz;
    ullong aux_khz;
    int p;

    // Boost test
    if (boost_enabled && freq_khz >= p0_khz(psss[0].cof)) {
        *pstate_index = 0;
        return EAR_SUCCESS;
    }
    // Closest test
    if (closest && freq_khz > p1_khz(psss[pss_nominal].cof)) {
        *pstate_index = pss_nominal;
        return EAR_SUCCESS;
    }
    //
    for (p = pss_nominal; p < pss_count; ++p) {
        cof_khz = p1_khz(psss[p].cof);
        if (freq_khz == cof_khz) {
            *pstate_index = p;
            return EAR_SUCCESS;
        }
        if (closest && freq_khz > cof_khz) {
            if (p > pss_nominal) {
                aux_khz = p1_khz(psss[p - 1].cof) - freq_khz;
                cof_khz = freq_khz - cof_khz;
                p       = p - (aux_khz < cof_khz);
            }
            *pstate_index = p;
            return EAR_SUCCESS;
        }
    }
    if (closest) {
        *pstate_index = pss_count - 1;
        return EAR_SUCCESS;
    }
    return_msg(EAR_ERROR, "P_STATE not found");
}

state_t mgt_cpufreq_amd17_get_available_list(ctx_t *c, pstate_t *pstate_list)
{
    int i;

    if (pss_nominal) {
        pstate_list[0].idx = 0LLU;
        pstate_list[0].khz = p0_khz(psss[0].cof);
    }
    for (i = pss_nominal; i < pss_count; ++i) {
        pstate_list[i].idx = (ullong) i;
        pstate_list[i].khz = p1_khz(psss[i].cof);
    }
    return EAR_SUCCESS;
}

static state_t get_current_hsmp(ctx_t *c, pstate_t *pstate_list)
{
    uint args[2] = {0, -1};
    uint reps[2] = {0, -1};
    uint cpu;
    uint idx;

    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
        // Preparing HSMP arguments and response
        args[0] = tp.cpus[cpu].apicid;
        reps[0] = 0;
        // Calling HSMP
        hsmp_send(tp.cpus[cpu].socket_id, HSMP_GET_BOOST_LIMIT, args, reps); // response is freq (MHz)
        debug("CPU%d HSMP read %u", tp.cpus[cpu].id, reps[0]);

        pstate_list[cpu].idx = pss_nominal;
        pstate_list[cpu].khz = p1_khz(psss[pss_nominal].cof);

        if (state_ok(search_pstate_list(((ullong) reps[0]) * 1000LLU, &idx, 1))) {
            pstate_list[cpu].idx = idx;
            if (idx == 0) {
                pstate_list[cpu].khz = p0_khz(psss[idx].cof);
            } else {
                pstate_list[cpu].khz = p1_khz(psss[idx].cof);
            }
        }
    }
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_amd17_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
    return get_current_hsmp(c, pstate_list);
}

state_t mgt_cpufreq_amd17_get_nominal(ctx_t *c, uint *pstate_index)
{
    *pstate_index = boost_enabled;
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_amd17_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest)
{
    return search_pstate_list(freq_khz, pstate_index, closest);
}

/** Setters */
static state_t set_pstate_hsmp(uint cpu, uint ps)
{
    uint args[2] = { 0, -1};
    uint reps[1] = { -1 };

    args[0] = setbits32(0, tp.cpus[cpu].apicid, 31, 16) | psss[ps].cof; // arguments are apicid and max_freq
    debug("setting PS%d: %u MHz in cpu %d (apic %d) [%u]", ps, getbits32(args[0], 15, 0), cpu,
          getbits32(args[0], 31, 16), args[0]);
    return hsmp_send(tp.cpus[cpu].socket_id, HSMP_SET_BOOST_LIMIT, args, reps);
}

static state_t set_pstate(uint cpu, uint ps)
{
    return set_pstate_hsmp(cpu, ps);
}

static state_t set_pstate0_hsmp(uint cpu)
{
    uint args[2] = {0, -1};
    uint reps[1] = { -1 };
    // Setting 16 bit MAX (it is converted by the system later)
    args[0] = setbits32(0, tp.cpus[cpu].apicid, 31, 16) | 65535;
    debug("setting PS0: %u MHz in cpu %d (apic %d) [%u]",
        getbits32(args[0], 15, 0), cpu, getbits32(args[0], 31, 16), args[0]);
    return hsmp_send(tp.cpus[cpu].socket_id, HSMP_SET_BOOST_LIMIT, args, reps);
}

static state_t set_pstate0(uint cpu)
{
    return set_pstate0_hsmp(cpu);
}

state_t mgt_cpufreq_amd17_set_current_list(ctx_t *c, uint *pstate_index)
{
    state_t s2 = EAR_SUCCESS;
    state_t s1 = EAR_SUCCESS;
    uint cpu;

    #if EXCESIVE_ROBUSTNESS
    // Step 1 (excesss of robustness, better rely on external functions to change governor).
    if (state_fail(s1 = driver->set_governor(Governor.userspace))) {
        return s1;
    }
    #endif
    // Step 2
    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
        if (pstate_index[cpu] == ps_nothing) {
            continue;
        }
        // If P_STATE boost
        if (pstate_index[cpu] == 0 && pss_nominal) {
            if (state_fail(s1 = set_pstate0(cpu))) {
                s2 = s1;
            }
            // Else
        } else if (state_fail(s1 = set_pstate(cpu, pstate_index[cpu]))) {
            s2 = s1;
        }
    }
    return s2;
}

state_t mgt_cpufreq_amd17_set_current(ctx_t *_c, uint pstate_index, int cpu)
{
    state_t s1 = EAR_SUCCESS;
    state_t s2 = EAR_SUCCESS;
    uint c;

    if (cpu >= tp.cpu_count) {
        return_msg(EAR_ERROR, Generr.cpu_invalid);
    }
    if (pstate_index >= pss_count) {
        return_msg(EAR_ERROR, Generr.arg_outbounds);
    }
    #if EXCESIVE_ROBUSTNESS
    // Step 1 (excesss of robustness, better rely on external functions to change governor).
    if (state_fail(s1 = driver->set_governor(Governor.userspace))) {
        return s1;
    }
    #endif
    debug("setting P_STATE %d (cof: %llu) in cpu %d", pstate_index, psss[pstate_index].cof, cpu);
    // Step 2 (single CPU)
    if (cpu != all_cpus) {
        // If P_STATE boost
        if (pstate_index == 0 && pss_nominal) {
            if (state_fail(s1 = set_pstate0(cpu))) {
                return s1;
            }
            return set_pstate0((uint) tp.cpus[cpu].sibling_id);
            // Else
        } else {
            if (state_fail(s1 = set_pstate((uint) cpu, pstate_index))) {
                return s1;
            }
            return set_pstate((uint) tp.cpus[cpu].sibling_id, pstate_index);
        }
    }
    // Step 2 (all CPUs to PX)
    for (c = 0; c < tp.cpu_count; ++c) {
        if (pstate_index == 0 && pss_nominal) {
            if (state_fail(s1 = set_pstate0(c))) {
                s2 = s1;
            }
        } else {
            if (state_fail(s1 = set_pstate(c, pstate_index))) {
                s2 = s1;
            }
        }
    }
    return s2;
}

state_t mgt_cpufreq_amd17_reset(ctx_t *c)
{
    return EAR_SUCCESS;
}

// Governors
state_t mgt_cpufreq_amd17_governor_get(ctx_t *c, uint *governor)
{
    return driver->get_governor(governor);
}

state_t mgt_cpufreq_amd17_governor_get_list(ctx_t *c, uint *governors)
{
    return driver->get_governor_list(governors);
}

state_t mgt_cpufreq_amd17_governor_set(ctx_t *c, uint governor)
{
    state_t s = EAR_SUCCESS;
    if (state_fail(s = driver->set_governor(governor))) {
        return s;
    }
    if (governor == Governor.userspace) {
        s = driver->set_current(0, all_cpus);
    }
    return s;
}

state_t mgt_cpufreq_amd17_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask)
{
    state_t s = EAR_SUCCESS;
    int cpu;

    if (state_fail(s = driver->set_governor_mask(governor, mask))) {
        return s;
    }
    for (cpu = 0; governor == Governor.userspace && cpu < tp.cpu_count; ++cpu) {
        if (CPU_ISSET(cpu, &mask)) {
            s = driver->set_current(0, cpu);
        }
    }
    return s;
}

state_t mgt_cpufreq_amd17_governor_set_list(ctx_t *c, uint *governors)
{
    state_t s = EAR_SUCCESS;
    int cpu;

    if (state_fail(s = driver->set_governor_list(governors))) {
        return s;
    }
    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
        if (governors[cpu] == Governor.userspace) {
            s = driver->set_current(0, cpu);
        }
    }
    return s;
}
