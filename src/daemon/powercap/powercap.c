/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <common/colors.h>
#include <common/config.h>
#include <pthread.h>
// #define SHOW_DEBUGS 1
#include <common/messaging/msg_conf.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/execute.h>
#include <common/system/lock.h>
#include <common/system/monitor.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/log_eard.h>

#include <daemon/power_monitor.h>
#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/shared_configuration.h>

#ifndef POWERCAP_MON
#define POWERCAP_MON 0 // Powercap monitor disabled by default
#endif
#define MAX_PERC_POWER 90

uint32_t current_mode = PC_MODE_AUTO;

node_powercap_opt_t my_pc_opt;
static int my_ip;
extern int *ips;
extern int self_id;
extern volatile int init_ips_ready;
int last_status;
int fd_powercap_values = 0;
static pwr_mgt_t *pcmgr;
static uint pc_pid           = 0;
static uint pc_status_config = AUTO_CONFIG;
static uint current_power    = 0;
static timestamp_t last_powercap_reallocation_time;

pthread_t powercapmon_th;
unsigned long powercapmon_freq = 1;
extern int eard_must_exit;
static uint pc_cpu_strategy;
static uint8_t last_cluster_perc = 0;
/* EARGM soft powercap (EAR_RC_SET_POWER): 0 none, POWER_CAP_UNLIMITED, or a limit.
 * It overrides the cap the node would apply, and never changes def or the allocation. */
static uint32_t eargm_soft_limit = 0;

extern powermon_app_t *current_ear_app[MAX_NESTED_LEVELS];
extern int max_context_created;
extern int num_contexts;

static timestamp_t last_powercap_reallocation_time;

// Per-device powercap storage
static uint32_t *stored_cpu_powercaps  = NULL;
static uint32_t *stored_dram_powercaps = NULL;
#if USE_GPUS
static uint32_t *stored_gpu_powercaps = NULL;
#endif
static uint stored_cpu_count  = 0;
static uint stored_dram_count = 0;
#if USE_GPUS
static uint stored_gpu_count = 0;
#endif

#if POWERCAP_MON
static suscription_t *sus_powercap_monitor;
static bool powercap_monitor_running = false;
#endif

void get_date_str(char *msg, int size)
{
    struct tm *current_t;
    time_t rawtime;
    time(&rawtime);
    current_t = localtime(&rawtime);
    strftime(msg, size, "%c", current_t);
}

static void powercap_init_device_storage()
{
    uint i;
    // Initialize CPU device storage
    stored_cpu_count = pmgt_get_cpu_devices();
    if (stored_cpu_count > 0) {
        stored_cpu_powercaps = calloc(stored_cpu_count, sizeof(uint32_t));
        if (stored_cpu_powercaps == NULL) {
            error("Failed to allocate memory for CPU powercap storage");
            stored_cpu_count = 0;
        } else {
            for (i = 0; i < stored_cpu_count; i++) {
                stored_cpu_powercaps[i] = POWER_CAP_UNLIMITED;
            }
            debug("Initialized CPU powercap storage for %u devices to unlimited", stored_cpu_count);
        }
    }

    // Initialize DRAM device storage
    stored_dram_count = pmgt_get_dram_devices();
    if (stored_dram_count > 0) {
        stored_dram_powercaps = calloc(stored_dram_count, sizeof(uint32_t));
        if (stored_dram_powercaps == NULL) {
            error("Failed to allocate memory for DRAM powercap storage");
            stored_dram_count = 0;
        } else {
            for (i = 0; i < stored_dram_count; i++) {
                stored_dram_powercaps[i] = POWER_CAP_UNLIMITED;
            }
            debug("Initialized DRAM powercap storage for %u devices to unlimited", stored_dram_count);
        }
    }

#if USE_GPUS
    // Initialize GPU device storage
    stored_gpu_count = pmgt_get_gpu_devices();
    if (stored_gpu_count > 0) {
        stored_gpu_powercaps = calloc(stored_gpu_count, sizeof(uint32_t));
        if (stored_gpu_powercaps == NULL) {
            error("Failed to allocate memory for GPU powercap storage");
            stored_gpu_count = 0;
        } else {
            for (i = 0; i < stored_gpu_count; i++) {
                stored_gpu_powercaps[i] = POWER_CAP_UNLIMITED;
            }
            debug("Initialized GPU powercap storage for %u devices to unlimited", stored_gpu_count);
        }
    }
#endif
}

static void powercap_cleanup_device_storage()
{
    if (stored_cpu_powercaps != NULL) {
        free(stored_cpu_powercaps);
        stored_cpu_powercaps = NULL;
        stored_cpu_count     = 0;
    }

    if (stored_dram_powercaps != NULL) {
        free(stored_dram_powercaps);
        stored_dram_powercaps = NULL;
        stored_dram_count     = 0;
    }

#if USE_GPUS
    if (stored_gpu_powercaps != NULL) {
        free(stored_gpu_powercaps);
        stored_gpu_powercaps = NULL;
        stored_gpu_count     = 0;
    }
#endif
}

state_t powercap_set_stored_device_value(uint domain, uint device_id, uint32_t powercap_value)
{
    if (domain >= NUM_DOMAINS) {
        debug("invalid domain %u", domain);
        return EAR_ERROR;
    }

    switch (domain) {
        case DOMAIN_CPU:
            if (device_id >= stored_cpu_count || stored_cpu_powercaps == NULL) {
                debug("invalid CPU device %u (count: %u)", device_id, stored_cpu_count);
                return EAR_ERROR;
            }
            stored_cpu_powercaps[device_id] = powercap_value;
            break;

        case DOMAIN_DRAM:
            if (device_id >= stored_dram_count || stored_dram_powercaps == NULL) {
                debug("invalid DRAM device %u (count: %u)", device_id, stored_dram_count);
                return EAR_ERROR;
            }
            stored_dram_powercaps[device_id] = powercap_value;
            break;

        case DOMAIN_GPU:
#if USE_GPUS
            if (device_id >= stored_gpu_count || stored_gpu_powercaps == NULL) {
                debug("invalid GPU device %u (count: %u)", device_id, stored_gpu_count);
                return EAR_ERROR;
            }
            stored_gpu_powercaps[device_id] = powercap_value;
#else
            debug("GPU domain not supported");
            return EAR_ERROR;
#endif
            break;

        default:
            debug("unsupported domain %u", domain);
            return EAR_ERROR;
    }

    debug("domain=%u device=%u value=%u", domain, device_id, powercap_value);
    return EAR_SUCCESS;
}

void powercap_update_all_device_storage(uint domain, uint32_t powercap_value)
{
    uint device_count = 0;

    switch (domain) {
        case DOMAIN_CPU:
            device_count = stored_cpu_count;
            break;
        case DOMAIN_DRAM:
            device_count = stored_dram_count;
            break;
        case DOMAIN_GPU:
#if USE_GPUS
            device_count = stored_gpu_count;
#endif
            break;
        default:
            return;
    }

    // Set the same powercap value for all devices in this domain
    for (uint i = 0; i < device_count; i++) {
        powercap_set_stored_device_value(domain, i, powercap_value);
    }
}

/***** These two functions monitors node power for status update *****/
state_t pc_monitor_thread_init(void *p)
{
    return EAR_SUCCESS;
}

state_t pc_monitor_thread_main(void *p)
{
    powercap_verify_all_devices();
    return EAR_SUCCESS;
}

void powercap_monitor_init()
{
#if POWERCAP_MON
    sus_powercap_monitor             = suscription();
    sus_powercap_monitor->call_main  = pc_monitor_thread_main;
    sus_powercap_monitor->call_init  = pc_monitor_thread_init;
    sus_powercap_monitor->time_relax = 10000;
    sus_powercap_monitor->time_burst = 10000;
    sus_powercap_monitor->suscribe(sus_powercap_monitor);
    powercap_monitor_running = true;
#endif
}

void powercap_monitor_start()
{
#if POWERCAP_MON
    sus_powercap_monitor->suscribe(sus_powercap_monitor);
    powercap_monitor_running = true;
#endif
}

void powercap_monitor_stop()
{
#if POWERCAP_MON
    monitor_unregister(sus_powercap_monitor);
    powercap_monitor_running = false;
#endif
}

void powercap_monitor_set_time(int time_relax, int time_burst)
{
#if POWERCAP_MON
    if (time_relax == sus_powercap_monitor->time_relax && time_burst == sus_powercap_monitor->time_burst) {
        return;
    }
    if (time_relax >= 5000 && time_burst >= 5000) { // 5 seconds minimum
        sus_powercap_monitor->time_relax = time_relax;
        sus_powercap_monitor->time_burst = time_burst;
        if (powercap_monitor_running) {
            powercap_monitor_stop();
            powercap_monitor_start();
        }
    }
#endif
}

/* my_pc_opt.lock is held while caps reach the devices (RAPL/NVML writes take ms, the
 * INM/grace_hopper commands longer). Every caller waits this long for it, then drops
 * its request. */
#define PC_LOCK_TIMEOUT_MS 2000

static state_t pc_lock(const char *caller)
{
    state_t ret = ear_timedlock(&my_pc_opt.lock, PC_LOCK_TIMEOUT_MS);
    if (state_fail(ret))
        error("%s: powercap lock not acquired (%s), request dropped", caller, state_msg);
    return ret;
}

static void pc_unlock()
{
    int ret = pthread_mutex_unlock(&my_pc_opt.lock);
    if (ret)
        error("powercap lock not released: %s", strerror(ret));
}

/* A rescheduling makes EARL drop its signature and policy decision: it is forced only when
 * the node cap moves by this much (%) from the cap of the last one, or switches between
 * limited and unlimited. Smaller changes are copied and read at the next evaluation. */
#define RESCHED_CAP_CHANGE_PCT 5

static bool cap_change_is_significant(uint32_t cap)
{
    static uint32_t resched_cap = 0;
    static bool resched_done    = false;
    bool limited                = (cap > POWER_CAP_UNLIMITED);
    bool significant;
    if (!resched_done || limited != (resched_cap > POWER_CAP_UNLIMITED)) {
        significant = true;
    } else if (!limited) {
        significant = false;
    } else {
        uint32_t diff = (cap > resched_cap) ? cap - resched_cap : resched_cap - cap;
        significant   = ((uint64_t) diff * 100 >= (uint64_t) RESCHED_CAP_CHANGE_PCT * resched_cap);
    }
    if (significant) {
        resched_cap  = cap;
        resched_done = true;
    }
    return significant;
}

void update_node_powercap_opt_shared_info()
{
    int cc;
    powermon_app_t *pmapp;
    bool resched = cap_change_is_significant(my_pc_opt.current_pc);
    for (cc = 0; cc < MAX_NESTED_LEVELS; cc++) {
        pmapp = NULL;
        if (state_ok(powermon_context_trylock(cc, &pmapp))) {
            /* pper_domain is per job, kept up to date by pmgt: not overwritten with the node's */
            uint32_t shares[NUM_DOMAINS];
            memcpy(shares, pmapp->settings->pc_opt.pper_domain, sizeof(shares));
            memcpy(&pmapp->settings->pc_opt, &my_pc_opt, sizeof(node_powercap_opt_t));
            memcpy(pmapp->settings->pc_opt.pper_domain, shares, sizeof(shares));
            if (resched)
                pmapp->resched->force_rescheduling = 1;
            powermon_context_unlock(cc);
        }
    }
}

void print_node_powercap_opt(node_powercap_opt_t *my_powercap_opt)
{
    fprintf(stderr,
            "cuurent %u pc_def %u pc_idle %u th_inc %u th_red %u th_release %u cluster_perc_power %u requested %u "
            "released %u\n",
            my_powercap_opt->current_pc, my_powercap_opt->base_powercap, my_powercap_opt->powercap_idle,
            my_powercap_opt->th_inc, my_powercap_opt->th_red, my_powercap_opt->th_release,
            my_powercap_opt->cluster_perc_power, my_powercap_opt->requested, my_powercap_opt->released);
}

uint powercap_get_value()
{
    return my_pc_opt.current_pc;
}

ulong powercap_elapsed_last_powercap()
{
    return timestamp_diffnow(&last_powercap_reallocation_time, TIME_SECS);
}

/* Floor (% of the ear.conf base, not the current one, so reductions don't move it) used
 * when neither MIN_WORKING_POWER nor the devices' minimum caps are known: MIN_WORKING_POWER
 * is 0 when the tag does not set it (tags are zeroed at parse). */
#define MIN_WORKING_POWER_DEFAULT_PCT 50

/* Lowest cap of a node running a job. MIN_WORKING_POWER, the power a job needs on this
 * hardware at the lowest P-state, when defined; otherwise the sum of the lowest caps the
 * devices accept (not a power draw: the rest of the node is not in it); otherwise a share
 * of the base. Releases, grants, running caps and the base itself stop here. */
static uint32_t powercap_floor()
{
    uint32_t floor = powermon_get_min_working_power();
    if (floor == 0 && pcmgr != NULL) {
        uint32_t hw_max = 0;
        pmgt_get_node_hw_limits(&floor, &hw_max);
    }
    if (floor == 0)
        floor = (uint32_t) ((uint64_t) powermon_get_base_powercap() * MIN_WORKING_POWER_DEFAULT_PCT / 100);
    /* the fallback is 0 or 1 when the base is POWER_CAP_DISABLED/UNLIMITED or below 20 W:
     * keep the floor above those sentinels so a cap clamped to it is still a real limit */
    return ear_max(floor, POWER_CAP_UNLIMITED + 1);
}

static uint32_t powercap_ceiling()
{
    uint32_t max_powercap = powermon_get_max_powercap_def();
    return (max_powercap > 1) ? max_powercap : UINT32_MAX;
}

/* Bounds an allocation to [floor, max_powercap]. */
static uint32_t clamp_allocation(int64_t alloc)
{
    if (alloc < (int64_t) powercap_floor())
        return powercap_floor();
    if (alloc > (int64_t) powercap_ceiling())
        return powercap_ceiling();
    return (uint32_t) alloc;
}

/* The node cap follows from the node's state, in this order:
 *   1. EARGM soft limit, when EARGM runs in soft mode (POWER_CAP_UNLIMITED included);
 *   2. otherwise the allocation the node holds, or its idle cap while idle;
 *   3. a real cap of a node running a job is then raised to the floor and bounded by
 *      max_powercap. The idle cap (a share of the base) is not raised: no job needs
 *      MIN_WORKING_POWER, and the minimum caps of the devices say nothing of idle power. */
static uint32_t node_cap()
{
    uint32_t cap;
    if (eargm_soft_limit)
        cap = eargm_soft_limit;
    else if (my_pc_opt.powercap_status == PC_STATUS_IDLE)
        cap = ear_min(my_pc_opt.powercap_idle, my_pc_opt.last_t1_allocated);
    else
        cap = my_pc_opt.last_t1_allocated;

    if (cap <= POWER_CAP_UNLIMITED)
        return cap;
    if (my_pc_opt.powercap_status == PC_STATUS_IDLE)
        return ear_min(cap, powercap_ceiling());
    return clamp_allocation(cap);
}

/* Power an idle node can give back: what its allocation holds above the idle cap. Computed
 * from the allocation now, not when the node went idle: EARGM may have reclaimed the extra or
 * reset the allocation since, and that power must not be released (and counted) twice. */
static uint32_t idle_releasable()
{
    if (my_pc_opt.powercap_status != PC_STATUS_IDLE)
        return 0;
    if (my_pc_opt.last_t1_allocated <= my_pc_opt.powercap_idle)
        return 0;
    return my_pc_opt.last_t1_allocated - my_pc_opt.powercap_idle;
}

/* Applies node_cap() when it changed: the only place the node cap reaches the domains.
 * Callers change the state (allocation, status, soft limit) and call this. */
static int apply_node_cap()
{
    char c_date[128];
    powermon_app_t *pmapp;
    uint32_t cap = node_cap();

    if (cap == my_pc_opt.current_pc)
        return EAR_SUCCESS;
    verbose(VCONF, "%snode cap %u -> %u W (allocated %u, soft limit %u)%s", COL_BLU, my_pc_opt.current_pc, cap,
            my_pc_opt.last_t1_allocated, eargm_soft_limit, COL_CLR);
    get_date_str(c_date, sizeof(c_date));
    if (fd_powercap_values >= 0)
        dprintf(fd_powercap_values, "%s domain %u limit %u \n", c_date, DOMAIN_NODE, cap);

    my_pc_opt.current_pc = cap;
    update_node_powercap_opt_shared_info();
    pmgt_set_app_req_freq(pcmgr);
    /* PENDING POWER allocation for jobs sharing the node */
    for (int i = 1; i < MAX_NESTED_LEVELS; i++) {
        pmapp = NULL;
        if (state_ok(powermon_context_trylock(i, &pmapp))) {
            pmapp->settings->pc_opt.current_pc = cap;
            powermon_context_unlock(i);
        }
    }
    timestamp_get(&last_powercap_reallocation_time);
    if (current_mode != PC_MODE_AUTO)
        return EAR_SUCCESS;
    return pmgt_set_powercap_value(pcmgr, pc_pid, DOMAIN_NODE, cap);
}

void set_default_node_powercap_opt(node_powercap_opt_t *my_powercap_opt)
{
    uint32_t def = powermon_get_base_powercap();
    uint32_t max = powermon_get_max_powercap_def();
    /* A default above the max powercap can never be applied: the node would ask for it forever */
    if (def > 1 && max > 1 && def > max) {
        warning("default powercap %u W above max powercap %u W, using %u W", def, max, max);
        def = max;
    }
    my_powercap_opt->base_powercap      = def;
    my_powercap_opt->powercap_idle      = ear_max(def * EARD_POWERCAP_IDLE_PERC, 1);
    my_powercap_opt->current_pc         = 0;
    my_powercap_opt->last_t1_allocated  = def;
    my_powercap_opt->max_node_power     = powermon_get_max_powercap_def();
    my_powercap_opt->released           = (my_powercap_opt->last_t1_allocated > my_powercap_opt->powercap_idle)
                                              ? my_powercap_opt->last_t1_allocated - my_powercap_opt->powercap_idle
                                              : 0;
    my_powercap_opt->th_inc             = 10;
    my_powercap_opt->th_red             = 50;
    my_powercap_opt->th_release         = 25;
    my_powercap_opt->powercap_status    = PC_STATUS_ERROR;
    my_powercap_opt->cluster_perc_power = 0;
    my_powercap_opt->requested          = 0;
    debug("default powercap: %u", my_powercap_opt->base_powercap);
    pthread_mutex_init(&my_powercap_opt->lock, NULL);
}

void powercap_end()
{
    if (pmgt_disable(pcmgr) != EAR_SUCCESS) {
        error("pmgt_disable");
    }
    powercap_cleanup_device_storage();
}

/* Back to automatic mode: the domain caps left by manual mode are replaced by the node split */
static void leave_manual_mode()
{
    uint32_t prev = current_mode;
    current_mode  = PC_MODE_AUTO;
    if (prev == PC_MODE_MANUAL && is_powercap_on(&my_pc_opt))
        pmgt_set_powercap_value(pcmgr, pc_pid, DOMAIN_NODE, my_pc_opt.current_pc);
}

void powercap_process_message(char *action, char *mode, char *level, int32_t num_values, int32_t values[num_values])
{
    if (mode != NULL) {
        debug("mode specified: %s", mode);
        if (!strcasecmp(mode, "manual")) {
            current_mode = PC_MODE_MANUAL;
            if (action != NULL && !strcasecmp(action, "deactivate")) {
                debug("deactivate action received: deactivating manual mode");
                leave_manual_mode();
                return; // deactivate should ignore all other arguments
            }
        } else if (!strcasecmp(mode, "monitor")) {
            debug("mode monitor specified");
            if (action != NULL) {
                if (!strcasecmp(action, "enable")) {
                    debug("monitor enable");
                    powercap_monitor_start();
                    return;
                }
                if (!strcasecmp(action, "disable")) {
                    debug("monitor disable");
                    powercap_monitor_stop();
                    return;
                }
                if (!strcasecmp(action, "set")) {
                    if (num_values >= 2) {
                        debug("monitor set time %d %d", values[0], values[1]);
                        powercap_monitor_set_time(values[0], values[1]);
                    } else {
                        error("monitor set requires 2 values");
                    }
                    return;
                }
            }
        }
    }
    if (level != NULL) {
        debug("level specified: %s", level);
        if (!strcasecmp(level, "node")) {
            leave_manual_mode();
            debug("setting mode to auto");
        } else if (!strcasecmp(level, "cpu") || !strcasecmp(level, "domain") || !strcasecmp(level, "gpu") ||
                   !strcasecmp(level, "device")) {
            current_mode = PC_MODE_MANUAL;
            debug("setting mode to manual");
        }
    }
    if (action != NULL && !strcasecmp(action, "set")) {
        pmgt_process_message(level, num_values, values);
    }
}

int powercap_init()
{
    debug("powercap init");
    set_default_node_powercap_opt(&my_pc_opt);
    print_node_powercap_opt(&my_pc_opt);
    /* powercap set to 0 means unlimited */
    if (powermon_get_base_powercap() == 0) {
        debug("POWERCAP limit disabled");
        update_node_powercap_opt_shared_info();
        return EAR_SUCCESS;
    }
    while (init_ips_ready == 0) {
        sleep(1);
    }
    if (init_ips_ready > 0)
        my_ip = ips[self_id];
    else
        my_ip = 0;

    /* Low level power cap managemen initialization */
    if (pmgt_init() != EAR_SUCCESS) {
        error("Low level power capping management error");
        return EAR_ERROR;
    }
    if (pmgt_handler_alloc(&pcmgr) != EAR_SUCCESS) {
        error("Allocating memory for powercap handler");
        return EAR_ERROR;
    }
    if (pmgt_enable(pcmgr) != EAR_SUCCESS) {
        error("Initializing powercap manager");
        return EAR_ERROR;
    }

    /* End Low level power cap managemen initialization */
    /* the hardware range is known now: a default under the node working minimum is raised */
    if (my_pc_opt.base_powercap > POWER_CAP_UNLIMITED && my_pc_opt.base_powercap < powercap_floor()) {
        warning("default powercap %u W below the node working minimum %u W, using %u W", my_pc_opt.base_powercap,
                powercap_floor(), powercap_floor());
        my_pc_opt.base_powercap     = powercap_floor();
        my_pc_opt.powercap_idle     = ear_max(my_pc_opt.base_powercap * EARD_POWERCAP_IDLE_PERC, 1);
        my_pc_opt.last_t1_allocated = my_pc_opt.base_powercap;
    }
    my_pc_opt.powercap_status = PC_STATUS_IDLE;
    last_status               = PC_STATUS_IDLE;
    pc_cpu_strategy           = pmgt_get_powercap_cpu_strategy(pcmgr);
    pmgt_set_pc_mode(pcmgr, PC_MODE_TARGET);
    /* the plugins record every device cap they apply: the storage must exist before the first one */
    powercap_init_device_storage();
    apply_node_cap();
    debug("powercap initialization finished");
    powercap_monitor_init();
    update_node_powercap_opt_shared_info();
    return EAR_SUCCESS;
}

int powercap_idle_to_run()
{
    if (!is_powercap_on(&my_pc_opt))
        return EAR_SUCCESS;
    if (is_powercap_unlimited())
        return EAR_SUCCESS;
    debug("powercap_idle_to_run");
    if (state_fail(pc_lock(__func__)))
        return EAR_ERROR;
    pmgt_set_status(pcmgr, PC_STATUS_RUN);
    last_status = PC_STATUS_IDLE;
    if (my_pc_opt.powercap_status != PC_STATUS_IDLE) {
        error("We go to run and we were not in idle ");
    } else if (my_pc_opt.last_t1_allocated >= my_pc_opt.base_powercap) {
        /* the node keeps what it holds: power above the base (left by the previous job,
         * or a base lowered while idle) is reported as extra, EARGM reclaims it if needed */
        debug("%sidle to run: %u W allocated (base %u W)%s", COL_GRE, my_pc_opt.last_t1_allocated,
              my_pc_opt.base_powercap, COL_CLR);
        my_pc_opt.powercap_status = PC_STATUS_OK;
        my_pc_opt.released        = 0;
        apply_node_cap();
    } else {
        /* below the base (lent while idle): run with what we hold and ask for the rest */
        debug("%sidle to run: %u W of %u W, asking for the base%s", COL_GRE, my_pc_opt.last_t1_allocated,
              my_pc_opt.base_powercap, COL_CLR);
        my_pc_opt.powercap_status = PC_STATUS_ASK_DEF;
        my_pc_opt.requested       = my_pc_opt.base_powercap - my_pc_opt.last_t1_allocated;
        apply_node_cap();
    }
    pmgt_idle_to_run(pcmgr);
    pc_unlock();
    return EAR_SUCCESS;
}

int powercap_run_to_idle()
{
    if (!is_powercap_on(&my_pc_opt))
        return EAR_SUCCESS;
    if (is_powercap_unlimited())
        return EAR_SUCCESS;
    debug("powercap_run_to_idle");
    if (state_fail(pc_lock(__func__)))
        return EAR_ERROR;
    switch (my_pc_opt.powercap_status) {
        case PC_STATUS_IDLE:
            error("going from run to idle and we were in idle");
            break;
        case PC_STATUS_OK:
        case PC_STATUS_GREEDY:
        case PC_STATUS_ASK_DEF:
        case PC_STATUS_RELEASE:
            debug("%sGoing from run to idle%s", COL_GRE, COL_CLR);
            /* The allocation is kept until EARGM collects the released power (release_idle) */
            my_pc_opt.powercap_status = PC_STATUS_IDLE;
            my_pc_opt.requested       = 0;
            my_pc_opt.released        = idle_releasable();
            apply_node_cap();
            break;
    }
    pmgt_set_status(pcmgr, PC_STATUS_IDLE);
    pmgt_run_to_idle(pcmgr);
    pc_unlock();
    if (last_cluster_perc > MAX_PERC_POWER) {
        debug("resetting powercap due to power usage (%u)", last_cluster_perc);
        powercap_reset_default_power();
    }
    return EAR_SUCCESS;
}

/***************************************************************************************/
/********** Executed each time a new periodic metric is ready **************************/
/***************************************************************************************/

/* To consider, should we differentiate the monitoring frequency vs the DB monitoring */
int powercap_set_power_per_domain(dom_power_t *cp, uint use_earl, ulong avg_f)
{
    current_power = (uint) cp->platform;
    if (!is_powercap_on(&my_pc_opt))
        return EAR_SUCCESS;
    debug("periodic_metric_info");
    if (state_fail(pc_lock(__func__)))
        return EAR_ERROR;

    if (current_mode == PC_MODE_AUTO) {
        if (my_pc_opt.powercap_status == PC_STATUS_IDLE && !is_powercap_unlimited())
            pmgt_set_power_per_domain(pcmgr, cp, PC_STATUS_IDLE);
        else
            pmgt_set_power_per_domain(pcmgr, cp, PC_STATUS_RUN);
    }

    powercap_set_app_req_freq();
    if (current_power > my_pc_opt.current_pc) {
        debug("%s", COL_RED);
    } else {
        debug("%s", COL_GRE);
    }
    debug("PM event, current power %u powercap %u allocated %u status %u released %u requested %u", current_power,
          my_pc_opt.current_pc, my_pc_opt.last_t1_allocated, my_pc_opt.powercap_status, my_pc_opt.released,
          my_pc_opt.requested);
    debug("%s", COL_CLR);
    pc_unlock();
    return EAR_SUCCESS;
}

void print_power_status(powercap_status_t *my_status)
{
    int i;
    debug("Power_status:Ilde %u released %u requested %u total greedy %u  current power %u total power cap %u "
          "total_idle_power %u",
          my_status->idle_nodes, my_status->released, my_status->requested, my_status->num_greedy,
          my_status->current_power, my_status->total_powercap, my_status->total_idle_power);
    for (i = 0; i < my_status->num_greedy; i++) {
        if (my_status->num_greedy)
            debug("greedy=(ip=%u,req=%u,extra=%u) ", my_status->greedy_nodes[i], my_status->greedy_data[i].requested,
                  my_status->greedy_data[i].extra_power);
    }
}

/***************************************************************************************/
/**********  This function is executed under EARGM request    **************************/
/***************************************************************************************/

/* EARGM in hard powercap mode (options, idle releases, a status that may release power)
 * after soft mode, e.g. restarted with the other mode: the soft limit no longer applies and
 * the cap goes back to what the node holds. Called with my_pc_opt.lock held. */
static void follow_hard_powercap()
{
    if (eargm_soft_limit == 0) // no soft limit set: nothing to drop
        return;
    verbose(VEARD_PC, "EARGM is in hard powercap mode: soft limit %u W dropped", eargm_soft_limit);
    eargm_soft_limit = 0;
    apply_node_cap();
}

void powercap_get_status(powercap_status_t *my_status, pmgt_status_t *status, int release_power)
{
    verbose(VEARD_PC, "%spowercap_get_status: get_powercap_status_last_t1 %u def_pc %u release_power %d %s", COL_GRE,
            my_pc_opt.last_t1_allocated, my_pc_opt.base_powercap, release_power, COL_CLR);
    if (state_fail(pc_lock(__func__)))
        return;
    if (release_power) // only hard-mode EARGM asks nodes to release power
        follow_hard_powercap();
    my_status->total_nodes++;
    if (my_pc_opt.powercap_status == PC_STATUS_IDLE) {
        status->status     = PC_STATUS_IDLE;
        my_pc_opt.released = idle_releasable();
        my_status->released += my_pc_opt.released;
        my_status->total_idle_power += my_pc_opt.last_t1_allocated;
        my_status->idle_nodes++;
    } else {
        pmgt_get_status(status);
        if (my_pc_opt.last_t1_allocated < my_pc_opt.base_powercap)
            status->status = PC_STATUS_ASK_DEF;
        my_pc_opt.powercap_status = status->status;
        switch (status->status) {
            case PC_STATUS_GREEDY:
                // status->requested = my_pc_opt.requested;
                if (my_pc_opt.last_t1_allocated > my_pc_opt.base_powercap)
                    status->extra = my_pc_opt.last_t1_allocated - my_pc_opt.base_powercap;
                else
                    status->extra = 0;
                verbose(VEARD_PC, "powercap_get_status: PC_STATUS greedy, requesting %u", status->requested);
                break;
            case PC_STATUS_RELEASE:
                if (my_pc_opt.last_t1_allocated > my_pc_opt.base_powercap)
                    status->extra = my_pc_opt.last_t1_allocated - my_pc_opt.base_powercap;
                else
                    status->extra = 0;
                /* a release lends power for one period: the node asks its base back (ASK_DEF) */
                if (release_power) { // only release power if it comes from EARGM
                    my_pc_opt.last_t1_allocated = clamp_allocation((int64_t) my_pc_opt.last_t1_allocated - status->tbr);
                    apply_node_cap();
                    verbose(VEARD_PC, "powercap_get_status: %sReleased%s %u W, allocated %u W", COL_BLU, COL_CLR,
                            status->tbr, my_pc_opt.last_t1_allocated);
                }
                my_pc_opt.powercap_status = PC_STATUS_OK;
                break;
            case PC_STATUS_ASK_DEF:
                /* Data management */
                verbose(VEARD_PC, "powercap_get_status: %sAsking for default power%s %uW allocated %uW", COL_BLU,
                        COL_CLR, my_pc_opt.requested, my_pc_opt.last_t1_allocated);
                my_status->requested += my_pc_opt.base_powercap - my_pc_opt.last_t1_allocated;
                break;
            case PC_STATUS_OK:
                debug("PC_STATUS OK");
                status->requested = 0;
                if (my_pc_opt.last_t1_allocated > my_pc_opt.base_powercap) {
                    status->extra = my_pc_opt.last_t1_allocated - my_pc_opt.base_powercap;
                } else {
                    status->extra = 0;
                }
                break;
            case PC_STATUS_ERROR:
            default:
                break;
        }
    }
    ulong limit = my_pc_opt.current_pc;
    my_status->current_power += powermon_current_power();
    my_status->total_powercap += get_powercap_allocated(&my_pc_opt);
    pc_unlock();
    powermon_report_event(POWERCAP_VALUE, limit);
}

void copy_node_powercap_opt(node_powercap_opt_t *dst)
{
    memcpy(dst, &my_pc_opt, sizeof(node_powercap_opt_t));
}

void print_powercap_opt(powercap_opt_t *opt)
{
    int i;
    debug("num_greedy %u", opt->num_greedy);
    for (i = 0; i < opt->num_greedy; i++) {
        debug("greedy_node %d extra power %d", opt->greedy_nodes[i], opt->extra_power[i]);
    }
    debug("Current cluster power percentage %u", opt->cluster_perc_power);
}

/***************************************************************************************/
/**********  This function is executed when EARGM sends a message **********************/
/***************************************************************************************/

/* Applies an EARGM grant (negative: reclaim) to the allocation, bounded to
 * [floor, max_powercap]. Reports the amount actually applied; false when nothing changed. */
static bool apply_grant(int32_t extra, uint *event, ulong *value)
{
    uint32_t prev = my_pc_opt.last_t1_allocated;
    uint32_t next = clamp_allocation((int64_t) prev + extra);
    /* A reclaim only takes power above the base, so it stops at the base. A node
     * already below its base (it lent power) is not lowered further: the stop is then
     * its current allocation. Only reduce_def lowers the base itself. */
    uint32_t reclaim_stop = ear_min(prev, my_pc_opt.base_powercap);
    if (extra < 0 && next < reclaim_stop)
        next = reclaim_stop;
    if (next == prev)
        return false;
    *event                      = (next > prev) ? INC_POWERCAP : RED_POWERCAP;
    *value                      = (next > prev) ? next - prev : prev - next;
    my_pc_opt.last_t1_allocated = next;
    apply_node_cap();
    return true;
}

/* ASK_DEF: the node gets its base back and leaves ASK_DEF */
static void grant_base(void)
{
    my_pc_opt.powercap_status   = PC_STATUS_OK;
    my_pc_opt.last_t1_allocated = my_pc_opt.base_powercap;
    my_pc_opt.requested         = 0;
    apply_node_cap();
}

void powercap_set_opt(powercap_opt_t *opt, int id)
{
    /* events reported once the lock is released: at most the default and a grant */
    uint event[2];
    ulong value[2];
    uint n_ev = 0;
    print_powercap_opt(opt);
    if (!is_powercap_on(&my_pc_opt))
        return;
    if (is_powercap_unlimited())
        return;
    debug("powercap_set_opt command received");
    if (state_fail(pc_lock(__func__)))
        return;
    follow_hard_powercap();
    last_cluster_perc            = opt->cluster_perc_power;
    my_pc_opt.cluster_perc_power = opt->cluster_perc_power;
    /* Here we must check, based on our status, if actions must be taken */
    if (id >= 0) {
        switch (my_pc_opt.powercap_status) {
            case PC_STATUS_IDLE:
            case PC_STATUS_OK:
                if (my_pc_opt.last_t1_allocated > my_pc_opt.base_powercap) {
                    debug("%spowercap_set_opt new_pc_opt:status OK but in the greedy node list %d ip=%d with ip=%di "
                          "extra %d%s",
                          COL_RED, id, opt->greedy_nodes[id], my_ip, opt->extra_power[id], COL_CLR);
                    if (apply_grant(opt->extra_power[id], &event[n_ev], &value[n_ev]))
                        n_ev++;
                }
                break;
            case PC_STATUS_RELEASE:
                debug("My powercap status is RELEASE, doing nothing ");
                break;
            case PC_STATUS_ASK_DEF:
                /* listed in the options while asking for the default: the default is granted
                 * first, a positive extra on top of it */
                grant_base();
                event[n_ev] = SET_ASK_DEF;
                value[n_ev] = 0;
                n_ev++;
                if (opt->extra_power[id] > 0 && apply_grant(opt->extra_power[id], &event[n_ev], &value[n_ev]))
                    n_ev++;
                break;
            case PC_STATUS_GREEDY:
                debug("%spowercap_set_opt  new_pc_opt: greedy node %d ip=%d with ip=%d extra %d%s", COL_GRE, id,
                      opt->greedy_nodes[id], my_ip, opt->extra_power[id], COL_CLR);
                if (apply_grant(opt->extra_power[id], &event[n_ev], &value[n_ev]))
                    n_ev++;
                my_pc_opt.powercap_status = PC_STATUS_OK;
                my_pc_opt.requested       = 0;
                break;
        }
    } else if (my_pc_opt.powercap_status == PC_STATUS_ASK_DEF) {
        /* not listed: any options message means the default is granted */
        debug("My status is ASK_DEF and new settings received with new_pc: %u", my_pc_opt.base_powercap);
        grant_base();
        event[n_ev] = SET_ASK_DEF;
        value[n_ev] = 0;
        n_ev++;
    }
    pc_unlock();
    for (uint i = 0; i < n_ev; i++)
        powermon_report_event(event[i], value[i]);
}

void set_powercapstatus_mode(uint mode)
{
    pc_status_config = mode;
}

uint powercap_get_cpu_strategy()
{
    return pmgt_get_powercap_cpu_strategy(pcmgr);
}

uint powercap_get_gpu_strategy()
{
    return pmgt_get_powercap_gpu_strategy(pcmgr);
}

void powercap_set_app_req_freq()
{
    pmgt_set_app_req_freq(pcmgr);
}

void powercap_release_idle_power(pc_release_data_t *release)
{
    uint value = 0;
    if (state_fail(pc_lock(__func__)))
        return;
    follow_hard_powercap();
    switch (my_pc_opt.powercap_status) {
        case PC_STATUS_IDLE:
            value = idle_releasable();
            debug("%sReleasing %u allocated IDLE power %s", COL_BLU, value, COL_CLR);
            release->released += value;
            my_pc_opt.last_t1_allocated -= value;
            my_pc_opt.released = 0;
            break;
        default:
            break;
    }
    pc_unlock();
    powermon_report_event(RELEASE_POWER, value);
}

void powercap_reset_default_power()
{
    if (state_fail(pc_lock(__func__)))
        return;
    /* a soft limit, if any, stays: EARGM sends this reset on a reload in either mode */
    my_pc_opt.last_t1_allocated = my_pc_opt.base_powercap;
    apply_node_cap();
    pc_unlock();
}

void powercap_reduce_def_power(uint power)
{
    if (state_fail(pc_lock(__func__)))
        return;
    /* Saturates at the node working minimum: the base never goes under the floor
     * (nor reaches 0, which would turn the powercap off on the next reset) */
    uint32_t floor = powercap_floor();
    if (my_pc_opt.base_powercap <= floor) {
        pc_unlock();
        return;
    }
    my_pc_opt.base_powercap = (uint32_t) ear_max((int64_t) my_pc_opt.base_powercap - power, (int64_t) floor);
    my_pc_opt.powercap_idle = ear_min(my_pc_opt.powercap_idle, my_pc_opt.base_powercap);
    pc_unlock();
}

void powercap_increase_def_power(uint power)
{
    if (state_fail(pc_lock(__func__)))
        return;
    /* Saturates at max_powercap instead of wrapping */
    my_pc_opt.base_powercap =
        (uint32_t) ear_min((uint64_t) my_pc_opt.base_powercap + power, (uint64_t) powercap_ceiling());
    pc_unlock();
}

void powercap_set_powercap(uint32_t power)
{
    if (!is_powercap_on(&my_pc_opt))
        return;
    if (state_fail(pc_lock(__func__)))
        return;
    eargm_soft_limit = power;
    apply_node_cap();
    pc_unlock();
}

void powercap_new_job()
{
    pmgt_new_job(pcmgr);
}

void powercap_end_job()
{
    pmgt_end_job(pcmgr);
}

uint32_t powercap_get_stored_device_value(uint domain, uint device_id)
{
    if (domain >= NUM_DOMAINS) {
        debug("invalid domain %u", domain);
        return 0;
    }

    switch (domain) {
        case DOMAIN_CPU:
            if (device_id >= stored_cpu_count || stored_cpu_powercaps == NULL) {
                debug("invalid CPU device %u (count: %u)", device_id, stored_cpu_count);
                return 0;
            }
            return stored_cpu_powercaps[device_id];

        case DOMAIN_DRAM:
            if (device_id >= stored_dram_count || stored_dram_powercaps == NULL) {
                debug("invalid DRAM device %u (count: %u)", device_id, stored_dram_count);
                return 0;
            }
            return stored_dram_powercaps[device_id];

        case DOMAIN_GPU:
#if USE_GPUS
            if (device_id >= stored_gpu_count || stored_gpu_powercaps == NULL) {
                debug("invalid GPU device %u (count: %u)", device_id, stored_gpu_count);
                return 0;
            }
            return stored_gpu_powercaps[device_id];
#else
            debug("GPU domain not supported");
            return 0;
#endif

        default:
            debug("unsupported domain %u", domain);
            return 0;
    }
}

state_t powercap_verify_device_values(uint domain)
{
    uint device_count = 0;
    uint32_t stored_value;
    state_t overall_result  = EAR_SUCCESS;
    int mismatches          = 0;
    uint32_t *actual_values = NULL;
    state_t ret;

    if (!is_powercap_on(&my_pc_opt)) {
        debug("powercap is disabled");
        return EAR_SUCCESS;
    }

    if (domain >= NUM_DOMAINS) {
        debug("invalid domain %u", domain);
        return EAR_ERROR;
    }

    // Determine device count based on domain
    switch (domain) {
        case DOMAIN_CPU:
            device_count = stored_cpu_count;
            break;
        case DOMAIN_DRAM:
            device_count = stored_dram_count;
            break;
        case DOMAIN_GPU:
#if USE_GPUS
            device_count = stored_gpu_count;
#else
            device_count = 0; // No GPUs if not compiled with GPU support
#endif
            break;
        default:
            debug("unsupported domain %u", domain);
            return EAR_ERROR;
    }

    debug("checking domain %u with %u devices", domain, device_count);

    if (device_count == 0 || !pmgt_domain_readable(domain)) {
        debug("no devices to verify for domain %u", domain);
        return EAR_SUCCESS;
    }

    actual_values = calloc(device_count, sizeof(uint32_t));
    if (actual_values == NULL) {
        error("powercap_verify_device: failed to allocate memory for actual_values");
        return EAR_ERROR;
    }

    ret = pmgt_get_powercap_value_per_device(domain, actual_values);
    if (ret != EAR_SUCCESS) {
        error("powercap_verify_device: failed to get actual values for domain %u", domain);
        free(actual_values);
        return EAR_ERROR;
    }

    for (uint i = 0; i < device_count; i++) {
        stored_value          = powercap_get_stored_device_value(domain, i);
        uint32_t actual_value = actual_values[i];

        if (actual_value == 0) {
            /* a device that can't be read back is not verified */
            error("powercap_verify_device: failed to get actual value for domain %u device %u", domain, i);
            mismatches++;
            overall_result = EAR_WARNING;
            continue;
        }

        /* caps at or above TDP read back as unlimited: compare both sides the same way */
        if (stored_value != POWER_CAP_UNLIMITED && stored_value >= pmgt_get_device_tdp(domain, i))
            stored_value = POWER_CAP_UNLIMITED;
        debug("domain=%u device=%u stored=%u actual=%u", domain, i, stored_value, actual_value);

        if (stored_value != actual_value) {
            verbose(VEARD_PC, "%sWARNING: device powercap mismatch - domain %u device %u: stored=%u, actual=%u%s",
                    COL_YLW, domain, i, stored_value, actual_value, COL_CLR);
            // Try to set the power cap again?
            mismatches++;
            overall_result = EAR_WARNING;
        }
    }

    free(actual_values);

    if (mismatches == 0) {
        debug("all device values match for domain %u", domain);
    }

    return overall_result;
}

state_t powercap_verify_all_devices()
{
    state_t cpu_result, dram_result, gpu_result;
    state_t overall_result = EAR_SUCCESS;

    if (!is_powercap_on(&my_pc_opt)) {
        debug("powercap is disabled");
        return EAR_SUCCESS;
    }

    // This checks the value of the node by default.
    // if (is_powercap_unlimited()) {
    //     debug("powercap_verify_all_devices: powercap is unlimited");
    //     return EAR_SUCCESS;
    // }

    debug("starting verification of all device domains");

    // Verify CPU devices
    cpu_result = powercap_verify_device_values(DOMAIN_CPU);
    if (cpu_result != EAR_SUCCESS)
        overall_result = cpu_result;

    // Verify DRAM devices
    dram_result = powercap_verify_device_values(DOMAIN_DRAM);
    if (dram_result != EAR_SUCCESS)
        overall_result = dram_result;

    // Verify GPU devices
    gpu_result = powercap_verify_device_values(DOMAIN_GPU);
    if (gpu_result != EAR_SUCCESS)
        overall_result = gpu_result;

    if (overall_result == EAR_SUCCESS) {
        verbose(VEARD_PC, "powercap_verify_all_devices: all device powercaps verified successfully");
    } else {
        verbose(VEARD_PC, "powercap_verify_all_devices: found powercap mismatches in one or more domains");
    }

    return overall_result;
}
