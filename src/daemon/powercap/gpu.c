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
#include <common/config.h>
#include <pthread.h>
#include <signal.h>
// #define SHOW_DEBUGS 1
#include <common/colors.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/execute.h>
#include <common/system/monitor.h>
#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <management/gpu/gpu.h>
#include <metrics/gpu/gpu.h>

static uint32_t current_gpu_pc = POWER_CAP_UNLIMITED;
static uint32_t default_gpu_pc = POWER_CAP_UNLIMITED;
static uint gpu_pc_enabled     = 0;
static uint c_status           = PC_STATUS_IDLE;
static uint c_mode             = PC_MODE_LIMIT;

static gpu_t *values_gpu_init, *values_gpu_end, *values_gpu_diff;
static uint gpu_pc_num_gpus;
static uint32_t *gpu_pc_min_power;
static uint32_t *gpu_pc_max_power;
static uint32_t *gpu_pc_curr_power;
static uint32_t *gpu_pc_util;
static int32_t *gpu_pc_excess; /* cap - consumption: negative when over the cap */
static const ulong **gpu_freq_list = NULL;
static const uint *gpu_num_freqs   = NULL;
static ulong *t_freq;
static float *pdist;
static gpu_t *gpu_pc_power_data;

static uint32_t gpu_pc_total_util = 0;
/* busy (util > 0) state of each GPU at the last split: a GPU going idle or busy re-splits */
static uint8_t gpu_split_busy[MAX_GPUS_SUPPORTED];
static double *gpu_cons_power;
static power_estimator_t *gpu_est; /* power vs clock, per GPU */
static ulong *gpu_freq;
static suscription_t *sus_gpu;

static domain_settings_t settings = {.node_ratio = 0.02, .security_range = 0.01};

#if SHOW_DEBUGS
static uint debug_count = 0;
#define DEBUG_FREQ 5
#endif

static state_t int_set_powercap_value(uint32_t limit, uint32_t *gpu_util);
static state_t int_set_per_device_powercap_value(uint32_t *limit, uint32_t *gpu_util);

/************************ This function is called by the monitor before the iterative part ************************/
state_t gpu_pc_thread_init(void *p)
{

    mgt_gpu_freq_list(no_ctx, &gpu_freq_list, &gpu_num_freqs);
    gpu_est = calloc(gpu_pc_num_gpus, sizeof(power_estimator_t));
    for (uint i = 0; i < gpu_pc_num_gpus && gpu_est != NULL; i++)
        power_estimator_init(&gpu_est[i], gpu_pc_min_power[i], gpu_pc_max_power[i],
                             gpu_freq_list[i][gpu_num_freqs[i] - 1], gpu_freq_list[i][0]);
    if (gpu_read(values_gpu_init) != EAR_SUCCESS) {
        debug("Error in gpu_read in gpu_dvfs_pc");
        return EAR_ERROR;
    }
    return EAR_SUCCESS;
}

/* Watts GPU i needs over its cap to run at its target clock, from the power it draws now */
static uint gpu_needed_power(int i)
{
    if (gpu_est == NULL)
        return 0;
    double at_target = power_estimator_at(&gpu_est[i], gpu_cons_power[i], gpu_freq[i], ear_max(t_freq[i], gpu_freq[i]));
    return (at_target > gpu_pc_curr_power[i]) ? (uint) (at_target - gpu_pc_curr_power[i]) : 0;
}

/************************ This function is called by the monitor in iterative part ************************/
state_t gpu_pc_thread_main(void *p)
{
    uint i, j;
    uint gpu_local_util = 0;

    if (current_gpu_pc <= 1)
        return EAR_SUCCESS;

    if (!gpu_pc_enabled)
        return EAR_SUCCESS;
    if (gpu_read(values_gpu_end) != EAR_SUCCESS) {
        debug("Error in gpu_read gpu_dvfs_pc");
        return EAR_ERROR;
    }
    /* Update target frequency from EARL */
    pmgt_get_app_req_freq(DOMAIN_GPU, t_freq, gpu_pc_num_gpus);

    /* Metrics computation */
    gpu_data_diff(values_gpu_end, values_gpu_init, values_gpu_diff);
    /* Copy end values to new init */
    gpu_data_copy(values_gpu_init, values_gpu_end);
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        gpu_local_util += values_gpu_diff[i].util_gpu;
        gpu_pc_util[i]    = values_gpu_diff[i].util_gpu;
        gpu_cons_power[i] = values_gpu_diff[i].power_w;
        gpu_freq[i]       = values_gpu_diff[i].freq_gpu;
        if (gpu_est != NULL && gpu_pc_util[i] > 0)
            power_estimator_sample(&gpu_est[i], gpu_cons_power[i], gpu_freq[i]);
        gpu_pc_excess[i] = (int32_t) gpu_pc_curr_power[i] - (int32_t) gpu_cons_power[i];
        debug("gpu %d has excess %d (current power %.2lf and current pc %u)", i, gpu_pc_excess[i], gpu_cons_power[i],
              gpu_pc_curr_power[i]);
    }
    // redistribute power between gpus
    uint moved = 0;
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        if (t_freq[i] == 0)
            t_freq[i] = gpu_freq_list[i][0]; // set the nominal if no GPU freq was specified
        if (gpu_pc_util[i] == 0)
            continue; // idle nodes need no reallocation
        if (gpu_pc_excess[i] > 0)
            continue; // nodes with extra power need no reallocation
        for (j = 0; j < gpu_pc_num_gpus; j++) {
            if (gpu_pc_util[j] == 0)
                continue; // we don't want to remove power from idle nodes
            if (gpu_pc_excess[j] > 5) {
                /* we leave a bit of excess power to prevent fluctuation errors; the giver keeps its
                 * device minimum and the receiver can't go over its maximum */
                int32_t give = gpu_pc_excess[j] - 5;
                give         = ear_min(give, (int32_t) gpu_pc_curr_power[j] - (int32_t) gpu_pc_min_power[j]);
                give         = ear_min(give, (int32_t) gpu_pc_max_power[i] - (int32_t) gpu_pc_curr_power[i]);
                if (give <= 0)
                    continue;
                debug("moving %d watts from %d to %d (current pc from %u to %u)", give, j, i, gpu_pc_curr_power[j],
                      gpu_pc_curr_power[i]);
                gpu_pc_curr_power[i] += give;
                gpu_pc_curr_power[j] -= give; // remove the power from the original gpu
                gpu_pc_excess[i] += give;
                gpu_pc_excess[j] -= give;
                moved = 1;

                if (gpu_pc_excess[i] >= 0)
                    break; // if we have enough power in the gpu we can stop redistributing towards it
            }
        }
    }
    /* apply the moves on the devices */
    if (moved) {
        if (mgt_gpu_power_cap_set(no_ctx, gpu_pc_curr_power) != EAR_SUCCESS)
            debug("Error setting the redistributed GPU powercap");
        for (i = 0; i < gpu_pc_num_gpus; i++)
            powercap_set_stored_device_value(DOMAIN_GPU, i, gpu_pc_curr_power[i]);
    }
#if SHOW_DEBUGS
    debug_count++;
    if (debug_count % DEBUG_FREQ == 0)
        debug("Current gpu_local_util %u", gpu_local_util);
#endif
    uint busy_changed = 0;
    for (i = 0; i < gpu_pc_num_gpus && i < MAX_GPUS_SUPPORTED; i++)
        busy_changed |= ((gpu_pc_util[i] > 0) != gpu_split_busy[i]);
    if (busy_changed || util_changed(gpu_local_util, gpu_pc_total_util)) {
        debug("GPU:Internal power redistribution because util (%u -> %u)", gpu_local_util, gpu_pc_total_util);
        int_set_powercap_value(current_gpu_pc, gpu_pc_util);
        gpu_pc_total_util = gpu_local_util;
    }

    return EAR_SUCCESS;
}

state_t disable()
{
    gpu_pc_enabled = 0;
    return EAR_SUCCESS;
}

state_t enable(suscription_t *sus)
{
    state_t ret = EAR_SUCCESS;
    debug("GPU: power cap  enable");
    mgt_gpu_load(NO_EARD);
    if ((ret = mgt_gpu_init(no_ctx)) != EAR_SUCCESS) {
        debug("Error in mgt_gpu_init");
    } else {
        gpu_pc_enabled = 1;
        mgt_gpu_count_devices(no_ctx, &gpu_pc_num_gpus);
        debug("%d GPUS detectd in gpu_powercap", gpu_pc_num_gpus);
        gpu_pc_min_power = calloc(gpu_pc_num_gpus, sizeof(uint32_t));
        if (gpu_pc_min_power == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }

        mgt_gpu_power_data_alloc(&gpu_pc_max_power);
        mgt_gpu_power_data_alloc(&gpu_pc_curr_power);
        mgt_gpu_freq_data_alloc(&gpu_freq);
        mgt_gpu_freq_data_alloc(&t_freq);

        if (gpu_pc_max_power == NULL || gpu_pc_curr_power == NULL || gpu_freq == NULL || t_freq == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }

        gpu_cons_power = calloc(gpu_pc_num_gpus, sizeof(double));
        gpu_pc_excess  = calloc(gpu_pc_num_gpus, sizeof(int32_t));
        gpu_pc_util    = calloc(gpu_pc_num_gpus, sizeof(uint32_t));
        pdist          = calloc(gpu_pc_num_gpus, sizeof(float));

        if (gpu_cons_power == NULL || gpu_pc_excess == NULL || gpu_pc_util == NULL || pdist == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }

        mgt_gpu_power_cap_get_rank(no_ctx, gpu_pc_min_power, gpu_pc_max_power);
    }
    gpu_load(NO_EARD);
    gpu_data_alloc(&gpu_pc_power_data);
    gpu_data_alloc(&values_gpu_init);
    gpu_data_alloc(&values_gpu_end);
    gpu_data_alloc(&values_gpu_diff);

    /* Suscription is for gpu power monitoring */
    if (sus == NULL) {
        debug("NULL subscription in GPU-DVFS powercap");
        return EAR_ERROR;
    }
    sus->call_main  = gpu_pc_thread_main;
    sus->call_init  = gpu_pc_thread_init;
    sus->time_relax = 60000;
    sus->time_burst = 1000;
    sus->suscribe(sus);
    sus_gpu = sus;

    return ret;
}

state_t plugin_set_burst()
{
    return monitor_burst(sus_gpu, 0);
}

state_t plugin_set_relax()
{
    return monitor_relax(sus_gpu);
}

void plugin_get_settings(domain_settings_t *s)
{
    memcpy(s, &settings, sizeof(domain_settings_t));
}

state_t set_powercap_value(uint pid, uint32_t domain, uint32_t *limit, uint32_t *gpu_util)
{
    state_t ret = EAR_ERROR;
    debug("GPU: powermgr gpu power allocated %u", *limit);
    if (domain == LEVEL_NODE || domain == LEVEL_DOMAIN) {
        ret = int_set_powercap_value(*limit, gpu_util);

        if (ret == EAR_SUCCESS) {
            default_gpu_pc = *limit;
            c_status       = PC_STATUS_OK;
        }
    } else
        ret = int_set_per_device_powercap_value(limit, gpu_util);
    return ret;
}

state_t reset_powercap_value()
{
    debug("GPU: Resetting powercap value to %u", default_gpu_pc);
    c_status = PC_STATUS_OK;
    return int_set_powercap_value(default_gpu_pc, gpu_pc_util);
}

state_t release_powercap_allocation(uint decrease)
{
    debug("GPU: releasing %u from powercap allocation", decrease);
    current_gpu_pc -= decrease;
    c_status = PC_STATUS_RELEASE;
    return int_set_powercap_value(current_gpu_pc, gpu_pc_util);
}

state_t increase_powercap_allocation(uint increase)
{
    debug("GPU: increasing %u from powercap allocation", increase);
    current_gpu_pc += increase;
    if (current_gpu_pc >= default_gpu_pc)
        c_status = PC_STATUS_OK;
    return int_set_powercap_value(current_gpu_pc, gpu_pc_util);
}

state_t restore_powercap()
{
    int i;
    powercap_update_all_device_storage(DOMAIN_GPU, POWER_CAP_UNLIMITED);
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        gpu_pc_curr_power[i] = gpu_pc_max_power[i];
    }
    return mgt_gpu_power_cap_set(no_ctx, gpu_pc_curr_power);
}

static state_t int_set_per_device_powercap_value(uint32_t *limit, uint32_t *gpu_util)
{
    state_t ret;
    int i;

    /* Set data */
    debug("%s", COL_BLU);
    debug("GPU: set_manual_powercap_value (first GPU: %u)", *limit);
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        /* unlimited is the device maximum; anything else is clamped to what NVML accepts */
        uint32_t w = (limit[i] == POWER_CAP_UNLIMITED) ? gpu_pc_max_power[i] : limit[i];
        if (gpu_pc_max_power[i] && w > gpu_pc_max_power[i])
            w = gpu_pc_max_power[i];
        if (w < gpu_pc_min_power[i])
            w = gpu_pc_min_power[i];
        if (w != limit[i] && limit[i] != POWER_CAP_UNLIMITED)
            warning("GPU %d cap %u W outside its %u..%u W range, using %u W", i, limit[i], gpu_pc_min_power[i],
                    gpu_pc_max_power[i], w);
        gpu_pc_curr_power[i] = w;
        powercap_set_stored_device_value(DOMAIN_GPU, i, gpu_pc_curr_power[i]);
        debug("GPU: util_gpu[%d]=%u power_alloc=%u", i, gpu_util[i], w);
    }
    ret = mgt_gpu_power_cap_set(no_ctx, gpu_pc_curr_power);
    if (ret == EAR_ERROR) {
        debug("Error setting powercap");
    }
    memcpy(gpu_pc_util, gpu_util, sizeof(uint32_t) * gpu_pc_num_gpus);
    return ret;
}

static state_t int_set_powercap_value(uint32_t limit, uint32_t *gpu_util)
{
    state_t ret;
    int i, pass;
    int64_t remaining;
    uint64_t total_util;
    uint8_t fixed[MAX_GPUS_SUPPORTED];
    current_gpu_pc = limit;

    if (current_gpu_pc == POWER_CAP_UNLIMITED) {
        return restore_powercap();
    }
    debug("GPU: set_powercap_value %u", limit);
    /* Phase 1: idle GPUs sit at their device minimum and reserve exactly that */
    remaining = limit;
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        gpu_split_busy[i] = (gpu_util[i] > 0);
        fixed[i]          = (gpu_util[i] == 0);
        if (fixed[i]) {
            gpu_pc_curr_power[i] = gpu_pc_min_power[i];
            remaining -= gpu_pc_min_power[i];
        }
    }
    /* Phase 2: busy GPUs share the rest by utilization (water-filling): a GPU clamped to
     * its [min, max] range is fixed there and what it can't take goes to the others */
    for (pass = 0; pass <= gpu_pc_num_gpus; pass++) {
        int clamped = 0;
        total_util  = 0;
        for (i = 0; i < gpu_pc_num_gpus; i++)
            if (!fixed[i])
                total_util += gpu_util[i];
        if (total_util == 0)
            break;
        for (i = 0; i < gpu_pc_num_gpus; i++) {
            if (fixed[i])
                continue;
            int64_t alloc = (remaining > 0) ? (int64_t) (remaining * gpu_util[i] / total_util) : 0;
            if (alloc >= gpu_pc_max_power[i] || alloc <= gpu_pc_min_power[i]) {
                gpu_pc_curr_power[i] = (alloc >= gpu_pc_max_power[i]) ? gpu_pc_max_power[i] : gpu_pc_min_power[i];
                remaining -= gpu_pc_curr_power[i];
                fixed[i] = 1;
                clamped  = 1;
                break; /* re-split the rest without it */
            }
        }
        if (clamped)
            continue;
        for (i = 0; i < gpu_pc_num_gpus; i++)
            if (!fixed[i])
                gpu_pc_curr_power[i] = (uint32_t) (remaining * gpu_util[i] / total_util);
        break;
    }
    ret = mgt_gpu_power_cap_set(no_ctx, gpu_pc_curr_power);
    if (ret == EAR_ERROR)
        debug("Error setting powercap");
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        powercap_set_stored_device_value(DOMAIN_GPU, i, gpu_pc_curr_power[i]);
        debug("GPU: util_gpu[%d]=%u power_alloc=%u", i, gpu_util[i], gpu_pc_curr_power[i]);
    }
    memcpy(gpu_pc_util, gpu_util, sizeof(uint32_t) * gpu_pc_num_gpus);
    return ret;
}

state_t get_powercap_value(uint pid, uint32_t *powercap)
{
    /* copy data */
    debug("GPU::get_powercap_value");
    // Get real powercap using APIs
    return mgt_gpu_power_cap_get_current(no_ctx, powercap);
}

uint is_powercap_policy_enabled(uint pid)
{
    return gpu_pc_enabled;
}

void print_powercap_value(int fd)
{
    dprintf(fd, "gpu_powercap_value %u\n", current_gpu_pc);
}

void powercap_to_str(char *b)
{
    sprintf(b, "%u", current_gpu_pc);
}

void set_status(uint status)
{
    debug("GPU. set_status %u", status);
    c_status = status;
}

uint get_powercap_strategy()
{
    debug("GPU. get_powercap_strategy");
    return PC_POWER;
}

void set_pc_mode(uint mode)
{
    debug("GPU. set_pc_mode");
    c_mode = mode;
}

void set_verb_channel(int fd)
{
    WARN_SET_FD(fd);
    VERB_SET_FD(fd);
    DEBUG_SET_FD(fd);
}

void set_new_utilization(uint32_t *util)
{
    int i;
    if (!gpu_pc_enabled || gpu_pc_util == NULL || gpu_pc_curr_power == NULL)
        return;
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        debug("GPU: util_gpu[%d]=%u", i, util[i]);
    }
    int_set_powercap_value(current_gpu_pc, util);
    memcpy(gpu_pc_util, util, sizeof(uint32_t) * gpu_pc_num_gpus);
}

void set_app_req_freq(ulong *f)
{
    int i;
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        // debug("GPU_DVFS:GPU %d Requested application freq set to %lu",i,f[i]);
        t_freq[i] = f[i];
    }
}

#define MIN_GPU_POWER_MARGIN 10

/* Returns 0 when 1) No info, 2) No limit 3) We cannot share power. */
/* Returns 1 when power can be shared with other modules */
/* Status can be: PC_STATUS_OK, PC_STATUS_GREEDY, PC_STATUS_RELEASE, PC_STATUS_ASK_DEF */
/* tbr is > 0 when PC_STATUS_RELEASE , 0 otherwise */
/* X_status is the plugin status */
/* X_ask_def means we must ask for our power */

uint get_powercap_status(domain_status_t *status)
{
    int i;
    uint used          = 0;
    uint g_tbr         = 0;
    uint tmp_stress    = 0;
    uint requested     = 0;
    uint greedy        = 0;
    uint ask_def       = 0;
    status->ok         = PC_STATUS_OK;
    status->exceed     = 0;
    status->requested  = 0;
    status->stress     = 0;
    status->current_pc = current_gpu_pc;

    if (current_gpu_pc == POWER_CAP_UNLIMITED) {
        return 0;
    }
    /* If we are not using th GPU we can release all the power */
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        used += (gpu_pc_util[i] > 0);
        if (t_freq[i] == 0)
            return 0;
    }
    if (!used) {
        status->ok = PC_STATUS_RELEASE;
        /* idle GPUs are held at their device minimum: keep that, release the rest */
        uint32_t keep = 0;
        for (i = 0; i < gpu_pc_num_gpus; i++)
            keep += gpu_pc_min_power[i];
        status->exceed = (current_gpu_pc > keep) ? current_gpu_pc - keep : 0;
        if (status->exceed == 0)
            status->ok = PC_STATUS_OK;
        // debug("%sReleasing %u Watts from the GPU%s",COL_GRE,status->exceed,COL_CLR);
        return 1;
    }

    for (i = 0; i < gpu_pc_num_gpus; i++) {
        if (t_freq[i] > 0 && gpu_pc_util[i]) {
            tmp_stress += powercap_get_stress(gpu_freq_list[i][0], gpu_freq_list[i][gpu_num_freqs[i] - 1], t_freq[i],
                                              gpu_freq[i]);
            debug("gpu %d tmp_stress: %u (current freq %lu target_freq %lu)", i, tmp_stress, gpu_freq[i], t_freq[i]);
        }
    }
    tmp_stress /= used;
    status->stress = tmp_stress;

    /* If we know, we must check */
    status->ok     = PC_STATUS_RELEASE;
    status->exceed = 0;
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        /* gpu_pc_util is an average during a period , is more confident than an instantaneous measure*/
        if (tmp_stress && (t_freq[i] > gpu_freq[i]) && (gpu_pc_util[i] > 0)) {
            // debug("We cannot release power from GPU %d",i);
            if (c_status == PC_STATUS_RELEASE) {
                debug("GPU status ASK_DEF, current_pc %u and def_pc %u", current_gpu_pc, default_gpu_pc);
                ask_def = 1;
            } else {
                greedy = 1;
                requested += powercap_device_request(gpu_pc_max_power[i], gpu_pc_curr_power[i], gpu_needed_power(i));
            }
        } else if (gpu_cons_power[i] > gpu_pc_curr_power[i]) {
            /* at req_f but using more power than we should */
            greedy = 1;
            requested += powercap_device_request(gpu_pc_max_power[i], gpu_pc_curr_power[i], gpu_needed_power(i));
            debug("GPU %d is greedy because more power is needed to mantain current speed", i);
        } else {
            /* However we use instanteneous power to compute potential power releases */
            g_tbr          = (uint) ((gpu_pc_curr_power[i] - gpu_cons_power[i]) * 0.5);
            status->exceed = status->exceed + g_tbr;
        }
    }
    /* Throttled GPUs that can't use more power (at their hardware max, or not held by the cap): not greedy */
    if (greedy && !requested)
        greedy = 0;
    if (ask_def || greedy) {
        status->ok        = ask_def ? PC_STATUS_ASK_DEF : PC_STATUS_GREEDY;
        status->requested = ask_def ? 0 : ear_min(requested, UINT16_MAX);
        status->exceed    = 0;
        return 0;
    }
    if (status->exceed < MIN_GPU_POWER_MARGIN) {
        status->exceed = 0;
        status->ok     = PC_STATUS_OK;
        return 0;
    }
    return 1;
}
