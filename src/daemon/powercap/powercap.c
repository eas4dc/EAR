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

void update_node_powercap_opt_shared_info()
{
    int cc;
    for (cc = 0; cc <= max_context_created; cc++) {
        if (current_ear_app[cc] != NULL) {
            /* Do we have to copy, "as is" ? PENDING */
            memcpy(&current_ear_app[cc]->settings->pc_opt, &my_pc_opt, sizeof(node_powercap_opt_t));
            current_ear_app[cc]->resched->force_rescheduling = 1;
        }
    }
}

void print_node_powercap_opt(node_powercap_opt_t *my_powercap_opt)
{
    fprintf(stderr,
            "cuurent %u pc_def %u pc_idle %u th_inc %u th_red %u th_release %u cluster_perc_power %u requested %u "
            "released %u\n",
            my_powercap_opt->current_pc, my_powercap_opt->def_powercap, my_powercap_opt->powercap_idle,
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

// this function changes the default powercap, last t1 and current powercap
static int set_powercap_value(uint domain, uint32_t limit)
{
    char c_date[128];
    int i;
    uint32_t max_powercap;
    verbose(VCONF, "%spowercap_set_powercap_value domain %u limit %u (current pc %u)%s", COL_BLU, domain, limit,
            my_pc_opt.current_pc, COL_CLR);
    max_powercap = powermon_get_max_powercap_def();
    // filter through the max powercap first
    if (max_powercap > 1)
        limit = ear_min(limit, max_powercap);

    // set the default and last t1 to the limit
    my_pc_opt.def_powercap      = limit;
    my_pc_opt.last_t1_allocated = limit;
    // if the filtered is already the current, no need to apply any change
    if (limit == my_pc_opt.current_pc)
        return EAR_SUCCESS;

    get_date_str(c_date, sizeof(c_date));
    if (fd_powercap_values >= 0) {
        dprintf(fd_powercap_values, "%s domain %u limit %u \n", c_date, domain, limit);
    }

    // update the current limit
    my_pc_opt.current_pc = limit;
    update_node_powercap_opt_shared_info();
    pmgt_set_app_req_freq(pcmgr);
    /* PENDING POWER allocation for jobs sharing the node */
    for (i = 1; i <= max_context_created; i++) {
        if (current_ear_app[i] != NULL) {
            current_ear_app[i]->settings->pc_opt.current_pc = powercap_get_value();
        }
    }
    timestamp_get(&last_powercap_reallocation_time);
    // apply the current limit
    if (current_mode == PC_MODE_AUTO)
        return pmgt_set_powercap_value(pcmgr, pc_pid, domain, (ulong) limit);
    else
        return EAR_SUCCESS;
}

void set_default_node_powercap_opt(node_powercap_opt_t *my_powercap_opt)
{
    my_powercap_opt->def_powercap       = powermon_get_powercap_def();
    my_powercap_opt->powercap_idle      = ear_max(powermon_get_powercap_def() * EARD_POWERCAP_IDLE_PERC, 1);
    my_powercap_opt->current_pc         = 0;
    my_powercap_opt->last_t1_allocated  = powermon_get_powercap_def();
    my_powercap_opt->max_node_power     = powermon_get_max_powercap_def();
    my_powercap_opt->released           = my_powercap_opt->last_t1_allocated - my_powercap_opt->powercap_idle;
    my_powercap_opt->th_inc             = 10;
    my_powercap_opt->th_red             = 50;
    my_powercap_opt->th_release         = 25;
    my_powercap_opt->powercap_status    = PC_STATUS_ERROR;
    my_powercap_opt->cluster_perc_power = 0;
    my_powercap_opt->requested          = 0;
    debug("default powercap: %u", my_powercap_opt->def_powercap);
    pthread_mutex_init(&my_powercap_opt->lock, NULL);
}

void powercap_end()
{
    if (pmgt_disable(pcmgr) != EAR_SUCCESS) {
        error("pmgt_disable");
    }
    powercap_cleanup_device_storage();
}

void powercap_process_message(char *action, char *mode, char *level, int32_t num_values, int32_t values[num_values])
{
    if (mode != NULL) {
        debug("mode specified: %s", mode);
        if (!strcasecmp(mode, "manual")) {
            current_mode = PC_MODE_MANUAL;
            if (action != NULL && !strcasecmp(action, "deactivate")) {
                debug("deactivate action received: deactivating manual mode");
                current_mode = PC_MODE_AUTO;
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
            current_mode = PC_MODE_AUTO;
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
    if (powermon_get_powercap_def() == 0) {
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
    my_pc_opt.powercap_status = PC_STATUS_IDLE;
    last_status               = PC_STATUS_IDLE;
    pc_cpu_strategy           = pmgt_get_powercap_cpu_strategy(pcmgr);
    pmgt_set_pc_mode(pcmgr, PC_MODE_TARGET);
    set_powercap_value(DOMAIN_NODE, my_pc_opt.powercap_idle);
    debug("powercap initialization finished");
    powercap_monitor_init();
    update_node_powercap_opt_shared_info();
    powercap_init_device_storage();
    return EAR_SUCCESS;
}

int powercap_idle_to_run()
{
    uint extra;
    if (!is_powercap_on(&my_pc_opt))
        return EAR_SUCCESS;
    if (is_powercap_unlimited())
        return EAR_SUCCESS;
    debug("powercap_idle_to_run");
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ; /* can we create some deadlock because of status ? */
    pmgt_set_status(pcmgr, PC_STATUS_RUN);
    // debug("pc status modified");
    last_status = PC_STATUS_IDLE;
    extra       = 0;
    switch (my_pc_opt.powercap_status) {
        case PC_STATUS_IDLE:
            debug("%sGoin from idle to run:allocated %u %s ", COL_GRE, my_pc_opt.last_t1_allocated, COL_CLR);
            /* There is enough power for me */
            if ((my_pc_opt.last_t1_allocated + extra) >= my_pc_opt.def_powercap) {
                /* if we already had de power, we just set the status as OK */
                if (my_pc_opt.last_t1_allocated >= my_pc_opt.def_powercap) {
                    my_pc_opt.powercap_status = PC_STATUS_OK;
                    my_pc_opt.released        = 0;
                    set_powercap_value(DOMAIN_NODE, my_pc_opt.last_t1_allocated);
                } else {
                    /* We must use extra power */
                    my_pc_opt.last_t1_allocated = ear_min(my_pc_opt.def_powercap, my_pc_opt.last_t1_allocated + extra);
                    my_pc_opt.powercap_status   = PC_STATUS_OK;
                    my_pc_opt.released          = 0;
                    set_powercap_value(DOMAIN_NODE, my_pc_opt.last_t1_allocated);
                }
            } else { /* we must ask more power */
                uint pending;
                my_pc_opt.last_t1_allocated += extra;
                my_pc_opt.powercap_status = PC_STATUS_ASK_DEF;
                pending                   = my_pc_opt.def_powercap - my_pc_opt.last_t1_allocated;
                my_pc_opt.requested       = pending;
                set_powercap_value(DOMAIN_NODE, my_pc_opt.last_t1_allocated);
            }
            break;
        case PC_STATUS_OK:
        case PC_STATUS_GREEDY:
        case PC_STATUS_RELEASE:
        case PC_STATUS_ASK_DEF:
            error("We go to run and we were not in idle ");
            break;
    }
    pmgt_idle_to_run(pcmgr);
    pthread_mutex_unlock(&my_pc_opt.lock);
    return EAR_SUCCESS;
}

int powercap_run_to_idle()
{
    if (!is_powercap_on(&my_pc_opt))
        return EAR_SUCCESS;
    if (is_powercap_unlimited())
        return EAR_SUCCESS;
    debug("powercap_run_to_idle");
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ;
    switch (my_pc_opt.powercap_status) {
        case PC_STATUS_IDLE:
            error("going from run to idle and we were in idle");
            break;
        case PC_STATUS_OK:
        case PC_STATUS_GREEDY:
        case PC_STATUS_ASK_DEF:
        case PC_STATUS_RELEASE:
            debug("%sGoing from run to idle%s", COL_GRE, COL_CLR);
            my_pc_opt.released        = my_pc_opt.last_t1_allocated - my_pc_opt.powercap_idle;
            my_pc_opt.requested       = 0;
            my_pc_opt.powercap_status = PC_STATUS_IDLE;
            set_powercap_value(DOMAIN_NODE, my_pc_opt.powercap_idle);
            break;
    }
    pmgt_set_status(pcmgr, PC_STATUS_IDLE);
    pmgt_run_to_idle(pcmgr);
    pthread_mutex_unlock(&my_pc_opt.lock);
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
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ;

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
    pthread_mutex_unlock(&my_pc_opt.lock);
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

void powercap_release_power()
{
    my_pc_opt.powercap_status   = PC_STATUS_OK;
    my_pc_opt.released          = 0;
    my_pc_opt.last_t1_allocated = my_pc_opt.current_pc;
    if (current_mode == PC_MODE_AUTO) {
        pmgt_set_powercap_value(pcmgr, pc_pid, DOMAIN_NODE, (ulong) my_pc_opt.current_pc);
    }
}

void powercap_get_status(powercap_status_t *my_status, pmgt_status_t *status, int release_power)
{
    verbose(VEARD_PC, "%spowercap_get_status: get_powercap_status_last_t1 %u def_pc %u release_power %d %s", COL_GRE,
            my_pc_opt.last_t1_allocated, my_pc_opt.def_powercap, release_power, COL_CLR);
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ;
    my_status->total_nodes++;
    if (my_pc_opt.powercap_status == PC_STATUS_IDLE) {
        status->status = PC_STATUS_IDLE;
        my_status->released += my_pc_opt.released;
        my_status->total_idle_power += my_pc_opt.last_t1_allocated;
        my_status->idle_nodes++;
    } else {
        pmgt_get_status(status);
        if (my_pc_opt.last_t1_allocated < my_pc_opt.def_powercap)
            status->status = PC_STATUS_ASK_DEF;
        my_pc_opt.powercap_status = status->status;
        switch (status->status) {
            case PC_STATUS_GREEDY:
                // status->requested = my_pc_opt.requested;
                if (my_pc_opt.last_t1_allocated > my_pc_opt.def_powercap)
                    status->extra = my_pc_opt.last_t1_allocated - my_pc_opt.def_powercap;
                else
                    status->extra = 0;
                verbose(VEARD_PC, "powercap_get_status: PC_STATUS greedy, requesting %u", status->requested);
                break;
            case PC_STATUS_RELEASE:
                if (my_pc_opt.last_t1_allocated > my_pc_opt.def_powercap)
                    status->extra = my_pc_opt.last_t1_allocated - my_pc_opt.def_powercap;
                else
                    status->extra = 0;
                if (release_power) { // only release power if it comes from EARGM
                    my_pc_opt.released = status->tbr;
                    my_pc_opt.current_pc -= status->tbr;
                }
                verbose(VEARD_PC, "powercap_get_status: %sReleasing%s %u W allocated %u W", COL_BLU, COL_CLR,
                        my_pc_opt.released, my_pc_opt.current_pc);
                powercap_release_power();
                break;
            case PC_STATUS_ASK_DEF:
                /* Data management */
                verbose(VEARD_PC, "powercap_get_status: %sAsking for default power%s %uW allocated %uW", COL_BLU,
                        COL_CLR, my_pc_opt.requested, my_pc_opt.last_t1_allocated);
                my_status->requested += my_pc_opt.def_powercap - my_pc_opt.last_t1_allocated;
                break;
            case PC_STATUS_OK:
                debug("PC_STATUS OK");
                status->requested = 0;
                if (my_pc_opt.last_t1_allocated > my_pc_opt.def_powercap) {
                    status->extra = my_pc_opt.last_t1_allocated - my_pc_opt.def_powercap;
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
    pthread_mutex_unlock(&my_pc_opt.lock);
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

void powercap_set_opt(powercap_opt_t *opt, int id)
{
    uint report_ev = 0;
    print_powercap_opt(opt);
    if (!is_powercap_on(&my_pc_opt))
        return;
    if (is_powercap_unlimited())
        return;
    uint event;
    ulong value;
    debug("powercap_set_opt command received");
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ;
    last_cluster_perc            = opt->cluster_perc_power;
    my_pc_opt.cluster_perc_power = opt->cluster_perc_power;
    /* Here we must check, based on our status, if actions must be taken */
    if (id >= 0) {
        switch (my_pc_opt.powercap_status) {
            case PC_STATUS_IDLE:
            case PC_STATUS_OK:
                if (my_pc_opt.last_t1_allocated > my_pc_opt.def_powercap) {
                    debug("%spowercap_set_opt new_pc_opt:status OK but in the greedy node list %d ip=%d with ip=%di "
                          "extra %d%s",
                          COL_RED, id, opt->greedy_nodes[id], my_ip, opt->extra_power[id], COL_CLR);
                    my_pc_opt.last_t1_allocated = my_pc_opt.last_t1_allocated + opt->extra_power[id];
                    if (opt->extra_power[id] > 0) {
                        event = INC_POWERCAP;
                        value = opt->extra_power[id];
                    } else {
                        event = RED_POWERCAP;
                        value = -opt->extra_power[id];
                    }

                    report_ev = 1;
                    set_powercap_value(DOMAIN_NODE, my_pc_opt.last_t1_allocated);
                }
                break;
            case PC_STATUS_RELEASE:
                debug("My powercap status is RELEASE, doing nothing ");
                break;
            case PC_STATUS_GREEDY:
                debug("%spowercap_set_opt  new_pc_opt: greedy node %d ip=%d with ip=%d extra %d%s", COL_GRE, id,
                      opt->greedy_nodes[id], my_ip, opt->extra_power[id], COL_CLR);
                if (opt->extra_power[id] > 0) {
                    event = INC_POWERCAP;
                    value = opt->extra_power[id];
                } else {
                    event = RED_POWERCAP;
                    value = -opt->extra_power[id];
                }
                report_ev                   = 1;
                my_pc_opt.last_t1_allocated = my_pc_opt.last_t1_allocated + opt->extra_power[id];
                set_powercap_value(DOMAIN_NODE, my_pc_opt.last_t1_allocated);
                my_pc_opt.powercap_status = PC_STATUS_OK;
                my_pc_opt.requested       = 0;
                break;
        }
    } else {
        if (my_pc_opt.powercap_status == PC_STATUS_ASK_DEF) {
            /* In that case, we assume we cat use the default power cap */
            debug("My status is ASK_DEF and new settings received with new_pc: %u", my_pc_opt.def_powercap);
            my_pc_opt.powercap_status = PC_STATUS_OK;
            /* We assume we will receive the requested power */
            my_pc_opt.last_t1_allocated = my_pc_opt.def_powercap;
            my_pc_opt.requested         = 0;
            set_powercap_value(DOMAIN_NODE, my_pc_opt.def_powercap);
            event     = SET_ASK_DEF;
            value     = 0;
            report_ev = 1;
        }
    }
    pthread_mutex_unlock(&my_pc_opt.lock);
    if (report_ev)
        powermon_report_event(event, value);
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
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ;
    switch (my_pc_opt.powercap_status) {
        case PC_STATUS_IDLE:
            debug("%sReleasing %u allocated IDLE power %s", COL_BLU, my_pc_opt.released, COL_CLR);
            release->released += my_pc_opt.released;
            value                       = my_pc_opt.released;
            my_pc_opt.released          = 0;
            my_pc_opt.last_t1_allocated = my_pc_opt.current_pc;
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&my_pc_opt.lock);
    powermon_report_event(RELEASE_POWER, value);
}

void powercap_reset_default_power()
{
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ;
    if (my_pc_opt.last_t1_allocated != my_pc_opt.def_powercap) {
        my_pc_opt.last_t1_allocated = my_pc_opt.def_powercap;
        set_powercap_value(DOMAIN_NODE, my_pc_opt.last_t1_allocated);
    }
    pthread_mutex_unlock(&my_pc_opt.lock);
}

void powercap_reduce_def_power(uint power)
{
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ;
    if (power > my_pc_opt.def_powercap) {
        pthread_mutex_unlock(&my_pc_opt.lock);
        return;
    }
    my_pc_opt.def_powercap -= power;
    pthread_mutex_unlock(&my_pc_opt.lock);
}

void powercap_increase_def_power(uint power)
{
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ;
    my_pc_opt.def_powercap += power;
    pthread_mutex_unlock(&my_pc_opt.lock);
}

void powercap_set_powercap(uint32_t power)
{
    while (pthread_mutex_trylock(&my_pc_opt.lock))
        ;
    set_powercap_value(DOMAIN_NODE, power);
    pthread_mutex_unlock(&my_pc_opt.lock);
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

    if (device_count == 0) {
        debug("no devices found for domain %u", domain);
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
            error("powercap_verify_device: failed to get actual value for domain %u device %u", domain, i);
            continue;
        }

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
