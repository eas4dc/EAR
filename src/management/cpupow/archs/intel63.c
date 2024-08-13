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
#define DEBUG_POWER_CONSUMING 0

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <common/output/debug.h>
#include <common/utils/keeper.h>
#include <common/utils/strtable.h>
#include <common/system/monitor.h>
#include <common/hardware/bithack.h>
#include <metrics/common/msr.h>
#include <metrics/common/apis.h>
#include <management/cpupow/cpupow.h>
#include <management/cpupow/archs/intel63.h>

#define MSR_NO_REG              0x000
#define MSR_RAPL_POWER_UNIT     0x606
#define MSR_PKG_POWER_LIMIT     0x610
#define MSR_PKG_ENERGY_STATUS   0x611
#define MSR_PKG_POLICY          MSR_NO_REG
#define MSR_PKG_PERF_STATUS     0x613
#define MSR_PKG_POWER_INFO      0x614
#define MSR_DRAM_POWER_LIMIT    0x618
#define MSR_DRAM_ENERGY_STATUS  0x619
#define MSR_DRAM_POLICY         MSR_NO_REG
#define MSR_DRAM_PERF_STATUS    0x61b
#define MSR_DRAM_POWER_INFO     0x61c
#define MSR_PP0_POWER_LIMIT     0x638
#define MSR_PP0_ENERGY_STATUS   0x639
#define MSR_PP0_POLICY          0x63a
#define MSR_PP0_PERF_STATUS     MSR_NO_REG
#define MSR_PP0_POWER_INFO      MSR_NO_REG
#define MSR_PP1_POWER_LIMIT     0x640
#define MSR_PP1_ENERGY_STATUS   0x641
#define MSR_PP1_POLICY          0x642
#define MSR_PP1_PERF_STATUS     MSR_NO_REG
#define MSR_PP1_POWER_INFO      MSR_NO_REG

#if SHOW_DEBUGS
#define tprintf3(...) tprintf2(__VA_ARGS__)
#else
#define tprintf3(...)
#endif

typedef struct msr_power_unit_s {
    ullong mult_reg;
    double mult_power;
    double mult_energy;
    double mult_time;
} msr_power_unit_t;

typedef struct msr_power_limit_s {
    ullong reg;
    uint pl1_power; // Power limit #1 (long term)
    uint pl1_enable;
    uint pl1_clamp;
    uint pl1_y; // time Y
    uint pl1_z; // time Z
    double pl1_window; // time window
    uint pl2_power; // Power limit #2 (short term)
    uint pl2_enable;
    uint pl2_clamp;
    uint pl2_y; // time Y
    uint pl2_z; // time Z
    double pl2_window; // time window
    uint window_10s;
    uint window_1s;
    uint window_100ms;
    uint window_10ms;
    uint lock;
} msr_power_limit_t;

typedef struct msr_power_info_s {
    ullong reg;
    uint max_power; //W
    uint min_power; //W
    double max_timew; //time window (uS)
    uint tdp; //W
} msr_power_info_t;

struct domain_s {
    topology_t        *tp;
    const char        *name;
    int                capable;
    msr_power_limit_t *pl;
    off_t              pl_address; // MSR_*_POWER_LIMIT
    int                pl_bit_lock;
    double             pl_divisor; // Time Window divisor
    const char        *pl_keeper_name;
    char               pl_error[256];
    off_t              es_address; // MSR_*_ENERGY_STATUS
    off_t              po_address; // MSR_*_POLICY
    off_t              ps_address; // MSR_*_PERF_STATUS
    msr_power_info_t  *pi;
    off_t              pi_address; // MSR_*_POWER_INFO
    char               pi_error[256];
    timestamp_t        pm_time; // Power Monitor
    ullong            *pm_energy;
};

static msr_power_unit_t *pu;
static topology_t        tp_cores;
static topology_t        tp_sockets;
static strtable_t        t;

#define DOMAIN_INIT(_tp, dom, pl_bl, pl_dv, pl_kn) { \
	.tp = _tp, \
    .name = #dom, \
    .capable = 1, \
	.pl_address = MSR_ ## dom ## _POWER_LIMIT, \
    .pl_bit_lock = pl_bl, \
    .pl_divisor = pl_dv, \
    .pl_keeper_name = pl_kn, \
    .es_address = MSR_ ## dom ## _ENERGY_STATUS, \
	.po_address = MSR_ ## dom ## _POLICY, \
	.ps_address = MSR_ ## dom ## _PERF_STATUS, \
	.pi_address = MSR_ ## dom ## _POWER_INFO \
}

static struct domain_s domains[] = {
    DOMAIN_INIT(&tp_sockets, PKG,  63,  4.0, "MsrPkgPowerLimit"),
    DOMAIN_INIT(&tp_sockets, DRAM, 31, 10.0, "MsrDramPowerLimit"),
    DOMAIN_INIT(&tp_cores  , PP0,  31, 10.0, "MsrPp0PowerLimit"),
    DOMAIN_INIT(&tp_sockets, PP1,  31, 10.0, "MsrPp1PowerLimit"),
    { NULL, NULL, 0 },
};

static uint domain_window_power_limit(cchar *name, cchar *keep, uint cpu, double time_s, double div, double mult)
{
    double time_nearest = 0;
    double time_current = 0;
    uint y_nearest = 0;
    uint z_nearest = 0;
    uint y, z;
    // Time Window DRAM/PP0/PP1 -> (2^Y)*(1+(Z/10) * TU -> Y(21:17) & Z(23:22) (Z is F in documents)
    // Time Window PKG          -> (2^Y)*(1+(Z/4)  * TU -> Y(21:17) & Z(23:22)
    for (y = 0; y < 32; ++y) { // Y(21:17) is 5 bits, 31 max integer
        for (z = 0; z < 4; ++z) { // Z(23:22) 2 bits, 3 max integer
            time_current = (pow(2.0, ((double) y)) * (1.0 + (((double) z)/div))) * mult;
            // Selecting the nearest time window under time
            if (time_current <= time_s && time_current > time_nearest) {
                time_nearest = time_current;
                y_nearest = y;
                z_nearest = z;
            }
        }
    }
    tprintf3(&t, "%s[%d] %s||window_%0.3lf||%lf||S||2^%u*(1+(%u/%u))", name, cpu, keep, time_s, time_nearest, y_nearest, z_nearest, (uint) div);
    return (uint) (setbits64(0LLU, z_nearest, 6, 5) | setbits64(0LLU, y_nearest, 4, 0));
}

static void domain_bit_power_limit(cchar *name, cchar *keep, cpu_t *cpu, msr_power_limit_t *pl, int bl, double div)
{
    double mult_power = pu[cpu->socket_id].mult_power;
    double mult_time  = pu[cpu->socket_id].mult_time;

    pl->lock       = (uint) getbits64(pl->reg, bl, bl);
    pl->pl1_enable = (uint) getbits64(pl->reg, 15, 15);
    pl->pl2_enable = (uint) getbits64(pl->reg, 47, 47);
    pl->pl1_clamp  = (uint) getbits64(pl->reg, 16, 16);
    pl->pl2_clamp  = (uint) getbits64(pl->reg, 48, 48);
    pl->pl1_y      = (uint) getbits64(pl->reg, 21, 17);
    pl->pl1_z      = (uint) getbits64(pl->reg, 23, 22);
    pl->pl2_y      = (uint) getbits64(pl->reg, 53, 49);
    pl->pl2_z      = (uint) getbits64(pl->reg, 55, 54);
    pl->pl1_power  = (uint) ((double) (getbits64(pl->reg, 14,  0) * mult_power));
    pl->pl2_power  = (uint) ((double) (getbits64(pl->reg, 46, 32) * mult_power));
    pl->pl1_window = (double) (pow(2.0, pl->pl1_y) * (1.0 + ((double) pl->pl1_z)/div)) * mult_time;
    pl->pl2_window = (double) (pow(2.0, pl->pl2_y) * (1.0 + ((double) pl->pl2_z)/div)) * mult_time;
    tprintf3(&t, "%s[%d] %s||pl1_power  ||%u    ||W||%04u * %0.3lf", name, cpu->id, keep, pl->pl1_power, (uint) getbits64(pl->reg, 14,  0), mult_power);
    tprintf3(&t, "%s[%d] %s||pl2_power  ||%u    ||W||%04u * %0.3lf", name, cpu->id, keep, pl->pl2_power, (uint) getbits64(pl->reg, 46, 32), mult_power);
    tprintf3(&t, "%s[%d] %s||pl1_window ||%0.3lf||S||2^%02u * (1+(%u/4)) * %lf", name, cpu->id, keep, pl->pl1_window, pl->pl1_y, pl->pl1_z, mult_time);
    tprintf3(&t, "%s[%d] %s||pl2_window ||%0.3lf||S||2^%02u * (1+(%u/4)) * %lf", name, cpu->id, keep, pl->pl2_window, pl->pl2_y, pl->pl2_z, mult_time);
    tprintf3(&t, "%s[%d] %s||pl1_enabled||%d    ||-||-", name, cpu->id, keep, pl->pl1_enable);
    tprintf3(&t, "%s[%d] %s||pl2_enabled||%d    ||-||-", name, cpu->id, keep, pl->pl2_enable);
    tprintf3(&t, "%s[%d] %s||lock       ||%d    ||-||-", name, cpu->id, keep, pl->lock);
    // Calculating time window for future use
    pl->window_10ms  = domain_window_power_limit(name, keep, cpu->id,  0.012, div, mult_time); // 10 ms (well ~)
    pl->window_100ms = domain_window_power_limit(name, keep, cpu->id,  0.110, div, mult_time); // 100 ms
    pl->window_1s    = domain_window_power_limit(name, keep, cpu->id,  1.000, div, mult_time); // 1 s
    pl->window_10s   = domain_window_power_limit(name, keep, cpu->id, 10.000, div, mult_time); // 10 s
}

static int domain_read_power_limit(const char *name, off_t address, cpu_t *cpu, msr_power_limit_t *pl, int bl, double div)
{
    if (state_fail(msr_read(cpu->id, &pl->reg, sizeof(ullong), address))) {
        tprintf3(&t, "%s[%d]||error||-||-||MSR_%s_POWER_LIMIT (%s)", name, cpu->id, name, state_msg);
        return 0;
    }
    // TODO: don't know if this is correct, can be 0 a domain which is capable?
    if (pl->reg == 0LLU) {
        tprintf3(&t, "%s[%d]||error||-||-||MSR_%s_POWER_LIMIT (the register is empty)", name, cpu->id, name);
        return 0;
    }
    domain_bit_power_limit(name, "", cpu, pl, bl, div);
    return 1;
}

static int domain_read_power_info(const char *name, off_t address, cpu_t *cpu, msr_power_info_t *pi)
{
    double mult_power = pu[cpu->socket_id].mult_power;
    double mult_time  = pu[cpu->socket_id].mult_time;

    if ((msr_read(cpu->id, &pi->reg, sizeof(ullong), address))) {
        tprintf3(&t, "%s[%d]||error||-||-||MSR_%s_POWER_INFO (%s)", name, cpu->id, name, state_msg);
        return 0;
    }
    // TODO: don't know if this is correct, can be 0 a domain which is capable?
    if (pi->reg == 0LLU) {
        tprintf3(&t, "%s[%d]||error||-||-||MSR_%s_POWER_INFO (the register is empty)", name, cpu->id, name);
        return 0;
    }
    pi->tdp       = (uint) (((double) getbits64(pi->reg, 14,  0)) * mult_power);
    pi->min_power = (uint) (((double) getbits64(pi->reg, 30, 16)) * mult_power);
    pi->max_power = (uint) (((double) getbits64(pi->reg, 46, 32)) * mult_power);
    pi->max_timew =        (((double) getbits64(pi->reg, 53, 48)) * mult_time );
    tprintf3(&t, "%s[%d]||tdp       ||%u||W||%04llu * %lf", name, cpu->id, pi->tdp      , getbits64(pi->reg, 14,  0), mult_power);
    tprintf3(&t, "%s[%d]||min_power ||%u||W||%04llu * %lf", name, cpu->id, pi->min_power, getbits64(pi->reg, 30, 16), mult_power);
    tprintf3(&t, "%s[%d]||max_power ||%u||W||%04llu * %lf", name, cpu->id, pi->max_power, getbits64(pi->reg, 46, 32), mult_power);

    return 1;
}

static int domain_init(int domain)
{
    struct domain_s *p = &domains[domain];
    char buffer[64];
    int i;

    // Allocation
    p->pi = calloc(p->tp->cpu_count, sizeof(msr_power_info_t));
    p->pl = calloc(p->tp->cpu_count, sizeof(msr_power_limit_t));
    // 
    for (i = 0; i < p->tp->cpu_count; ++i) {
        // MSR_*_POWER_INFO
        if (!domain_read_power_info(p->name, p->pi_address, &p->tp->cpus[i], &p->pi[i])) {
            //sprintf(p->pi_error, "Failed while reading MSR_%s_POWER_INFO: %s", p->name, state_msg);
            return 0;
        }
        // MSR_*_POWER_LIMIT
        if (!domain_read_power_limit(p->name, p->pl_address, &p->tp->cpus[i], &p->pl[i], p->pl_bit_lock, p->pl_divisor)) {
            //sprintf(p->pl_error, "Failed while reading MSR_%s_POWER_LIMIT: %s", p->name, state_msg);
            return 0;
        }
        // Recovering MSR_*_POWER_LIMIT registers
        sprintf(buffer, "%s%d", p->pl_keeper_name, i);
        if (!keeper_load_uint64(buffer, &p->pl[i].reg)) {
            keeper_save_uint64(buffer, p->pl[i].reg);
        } else {
            domain_bit_power_limit(p->name, "(k)", &p->tp->cpus[i], &p->pl[i], p->pl_bit_lock, p->pl_divisor);
        }
    }
    return 1;
}

static int rapl_init()
{
    int i;

    // Allocation
    pu = calloc(tp_sockets.cpu_count, sizeof(msr_power_unit_t));
    // Domain | Variable | Value | Formula
    tprintf_init2(&t, verb_channel, STR_MODE_DEF, "13 16 14 6 14");
    tprintf3(&t, "Domain||Variable||Value||Unit||Formula");
    tprintf3(&t, "------||--------||-----||----||-------");
    //
    for (i = 0; i < tp_sockets.cpu_count; ++i) {
        if (state_fail(msr_read(tp_sockets.cpus[i].id, &pu[i].mult_reg, sizeof(ullong), MSR_RAPL_POWER_UNIT))) {
            return 0;
        }
        // Get the multipliers
        pu[i].mult_power  = pow(0.5, (double) getbits64(pu[i].mult_reg,  3,  0));
        pu[i].mult_energy = pow(0.5, (double) getbits64(pu[i].mult_reg, 12,  8));
        pu[i].mult_time   = pow(0.5, (double) getbits64(pu[i].mult_reg, 19, 16));
        tprintf3(&t, "RAPL[%d]||mult_power ||%lf||W||1/(2^%u)", tp_sockets.cpus[i].id, pu[i].mult_power  , (uint) getbits64(pu[i].mult_reg,  3,  0));
        tprintf3(&t, "RAPL[%d]||mult_energy||%lf||W||1/(2^%u)", tp_sockets.cpus[i].id, pu[i].mult_energy , (uint) getbits64(pu[i].mult_reg, 12,  8));
        tprintf3(&t, "RAPL[%d]||mult_time  ||%lf||W||1/(2^%u)", tp_sockets.cpus[i].id, pu[i].mult_time   , (uint) getbits64(pu[i].mult_reg, 19, 16));
    }
    return 1;
}

#if DEBUG_POWER_CONSUMING
static state_t debug_monitor(void *x)
{
    struct domain_s *p = &domains[CPUPOW_SOCKET];
    static ullong energy1[2] = { 0, 0 };
    static ullong energy2[2] = { 0, 0 };
    double energy;
    double power;
    ullong reg;
    int i;

    for (i = 0; i < p->tp->cpu_count; ++i) {
        if (state_fail(msr_read(p->tp->cpus[i].id, &reg, sizeof(ullong), p->es_address))) {
            debug("msr_read returned %s", state_msg);
        }
        energy2[i] = getbits64(reg, 31, 0);
        debug("SOCKET[%u] read: %llu", i, energy2[i]);
        if (energy1[i] > 0) {
            energy = ((double) (energy2[i] - energy1[i])) * pu[i].mult_energy;
            power  = energy / 2.0; // We know its 2 seconds
            debug("SOCKET[%u] diff: %llu = %llu - %llu", i, energy2[i] - energy1[i], energy2[i], energy1[i]);
            debug("SOCKET[%u] energy: %lf J, power: %lf W (UNITS %lf)", i, energy, power, pu[i].mult_energy);
        }
        energy1[i] = energy2[i];
    }
    return EAR_SUCCESS;
}
#endif

CPUPOW_F_LOAD(mgt_cpupow_intel63_load)
{
    int i;

    if (tpo->vendor == VENDOR_INTEL && tpo->model >= MODEL_HASWELL_X) {
    } else {
        // API incompatible
        debug("api incompatible");
        return;
    }
    topology_select(tpo, &tp_cores  , TPSelect.core  , TPGroup.merge, 0);
    topology_select(tpo, &tp_sockets, TPSelect.socket, TPGroup.merge, 0);
    // Tesing and opening MSRs
    if (state_fail(msr_test(tpo, MSR_WR))) {
        debug("failed msr test: %s", state_msg);
        return;
    }
    for (i = 0; i < tp_cores.cpu_count; ++i) {
        if (state_fail(msr_open(tp_cores.cpus[i].id, MSR_WR))) {
            debug("Failed while opening CPU%d MSR: %s", tp_cores.cpus[i].id, state_msg);
            return;
        }
    }
    // MSR_RAPL_POWER_UNIT
    if (!rapl_init()) {
        return;
    }
    // PKG and DRAM initializations
    for(i = 0; domains[i].capable; ++i) {
        if (!domain_init(i)) {
            domains[i].capable = 0;
        }
    }
    if (!domains[CPUPOW_SOCKET].capable) {
        return;
    }
    #if SHOW_DEBUGS
    verbose(0, "-------------------------------------------------------------");
    tprintf_block(&t, 1);
    #endif
    // Extreme debugging, power monitor
    #if DEBUG_POWER_CONSUMING
    suscription_t *sus = suscription();
    sus->call_main     = debug_monitor;
    sus->time_relax    = 2000;
    sus->time_burst    = 2000;
    sus->suscribe(sus);
    monitor_init();
    #endif
    //
    apis_put(ops->get_info           , mgt_cpupow_intel63_get_info);
    apis_put(ops->count_devices      , mgt_cpupow_intel63_count_devices);
    apis_put(ops->powercap_is_capable, mgt_cpupow_intel63_powercap_is_capable);
    apis_put(ops->powercap_is_enabled, mgt_cpupow_intel63_powercap_is_enabled);
    apis_put(ops->powercap_get       , mgt_cpupow_intel63_powercap_get);
    apis_put(ops->powercap_set       , mgt_cpupow_intel63_powercap_set);
    apis_put(ops->powercap_reset     , mgt_cpupow_intel63_powercap_reset);
    apis_put(ops->tdp_get            , mgt_cpupow_intel63_tdp_get);
}

CPUPOW_F_GET_INFO(mgt_cpupow_intel63_get_info)
{
    info->api         = API_INTEL63;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = 1;
}

CPUPOW_F_COUNT_DEVICES(mgt_cpupow_intel63_count_devices)
{
    return domains[domain].tp->cpu_count;
}

CPUPOW_F_POWERCAP_IS_CAPABLE(mgt_cpupow_intel63_powercap_is_capable)
{
    return domains[domain].capable;
}

CPUPOW_F_POWERCAP_IS_ENABLED(mgt_cpupow_intel63_powercap_is_enabled)
{
        struct domain_s *p = &domains[domain];
        uint watts[16];
        state_t s;
        int i;

        memset(watts, 0, sizeof(int)*16);
        memset(enabled, 0, sizeof(int)*(p->tp->cpu_count));
        // Just the socket domain is enabled, so 16 of watts space is enough
        if (state_fail(s = mgt_cpupow_intel63_powercap_get(domain, watts))) {
            return s;
        }
        for (i = 0; i < p->tp->cpu_count; ++i) {
            enabled[i] = (watts[i] != p->pi[i].tdp);
            debug("enabled[%d]: %d %u %u", p->tp->cpus[i].id, enabled[i], watts[i], p->pi[i].tdp);
        }
        return s;
}

CPUPOW_F_POWERCAP_GET(mgt_cpupow_intel63_powercap_get)
{
    struct domain_s *p = &domains[domain];
    msr_power_limit_t pl;
    int i;

    // Cleaning
    if (watts != NULL) {
        memset(watts, 0, sizeof(uint)*p->tp->cpu_count);
    }
    //
    for (i = 0; i < p->tp->cpu_count; ++i) {
        if (!domain_read_power_limit(p->name, p->pl_address, &p->tp->cpus[i], &pl, p->pl_bit_lock, p->pl_divisor)) {
            return EAR_ERROR;
        }
        if (watts != NULL && pl.pl1_enable) {
            watts[i] = pl.pl1_power;
            debug("%s[%02d]: %u W of current powercap", p->name, p->tp->cpus[i].id, watts[i]);
        }
    }
    return EAR_SUCCESS;
}

CPUPOW_F_POWERCAP_SET(mgt_cpupow_intel63_powercap_set)
{
    struct domain_s *p = &domains[domain];
    double power;
    ullong reg;
    int i;

    if (!p->capable) {
        return EAR_ERROR;
    }
    for (i = 0; i < p->tp->cpu_count; ++i) {
        // Getting base register
        reg = p->pl[i].reg;
        // If is disabling
        if (watts[i] == POWERCAP_DISABLE) {
            // By now we are just using PL#1 (TODO: use TDP in power field)
            reg = setbits64(reg, 0, 15, 15); // enable
            debug("%s%d: disabling PL#1 (0x%llx)", p->name, p->tp->cpus[i].id, reg);
        } else {
            // Getting power value understood by RAPL
            power = ((double) watts[i]) / pu[p->tp->cpus[i].socket_id].mult_power;
            reg   = setbits64(reg,   (ullong) power, 14,  0); // power limit
            reg   = setbits64(reg,                1, 15, 15); // enable
            reg   = setbits64(reg, p->pl->window_1s, 23, 17); // time window
            debug("%s%d: writting watts %u -> %lf, regs %llu %llu",
                  p->name, p->tp->cpus[i].id, watts[i], power, p->pl->reg, reg);
        }
        if (state_fail(msr_write(p->tp->cpus[i].id, &reg, sizeof(ullong), p->pl_address))) {
            debug("Failed while writing MSR_%s_POWER_LIMIT[%d]: %s", p->name, p->tp->cpus[i].id, state_msg);
        }
    }
    #if SHOW_DEBUGS
    mgt_cpupow_intel63_powercap_get(domain, NULL); 
    #endif
    return EAR_SUCCESS;
}

CPUPOW_F_POWERCAP_RESET(mgt_cpupow_intel63_powercap_reset)
{
    struct domain_s *p = &domains[domain];
    int i;

    if (!p->capable) {
        return EAR_ERROR;
    }
    for (i = 0; i < p->tp->cpu_count; ++i) {
        msr_write(p->tp->cpus[i].id, &p->pl->reg, sizeof(ullong), p->pl_address);
    }
    return EAR_SUCCESS;
}

CPUPOW_F_TDP_GET(mgt_cpupow_intel63_tdp_get)
{
    struct domain_s *p = &domains[domain];
    int i;

    // Cleaning
    memset(watts, 0, sizeof(uint)*p->tp->cpu_count);
    // Iterating over sockets
    for (i = 0; i < p->tp->cpu_count; ++i) {
        watts[i] = p->pi[i].tdp;
    }
    return EAR_SUCCESS;
}
