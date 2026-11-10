/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// #define SHOW_DEBUGS 1
#include <common/colors.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/monitor.h>

#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_status_conf.h>

#include <management/gpu/gpu.h>
#include <metrics/gpu/gpu.h>

#define DEBUG_PERIOD       15
#define MIN_GPU_IDLE_POWER 30
#define UTIL_INC           10
#define UTIL_WAIT          50

static uint32_t gpu_idle_power = MIN_GPU_IDLE_POWER;

static uint gpu_dvfs_pc_enabled = 0;
// static ulong c_req_f;

/* This  subscription will take care automatically of the power monitoring */
static uint32_t current_gpu_pc = 0;
static uint32_t default_gpu_pc = 0;
static uint gpu_pc_enabled     = 0;
static uint c_status           = PC_STATUS_IDLE;
static uint c_mode             = PC_MODE_LIMIT;

static gpu_t *values_gpu_init, *values_gpu_end, *values_gpu_diff, *values_gpu_idle;

static uint gpu_pc_num_gpus;
static uint32_t *gpu_pc_min_power;
static uint32_t *gpu_pc_max_power;
/* These two variables controls the power ditribution between GPUS */
static uint32_t *gpu_pc_curr_power;
static uint32_t *gpu_pc_util;
/* util wait to control when a GPU has been idle and a previous freq to restore it when it starts again */
static uint32_t *gpu_pc_util_wait;
static ulong *gpu_pc_prev_freq;

static long *gpu_pc_excess;
static float *pdist;
static ulong *c_freq, *t_freq, *n_freq;
/* Damped proportional controller on the GPU's power vs clock estimate */
static power_estimator_t *gpu_est = NULL;
#define CTRL_GAIN       0.5 /* fraction of the power gap closed each period */
#define CTRL_MAX_STEP   4   /* at most 1/CTRL_MAX_STEP of the clock range per period */
#define CTRL_DEADBAND_W 2   /* over the cap by less than this: hold */
#define CTRL_RAISE_W    5   /* raise only with this much headroom */
static const ulong **gpu_freq_list       = NULL;
static const uint *gpu_num_freqs         = NULL;
static uint gpu_dvfs_status              = PC_STATUS_OK;
static uint gpu_dvfs_ask_def             = 0;
static uint gpu_dvfs_monitor_initialized = 0;
static uint32_t gpu_dvfs_util            = 0;
/* busy (util > 0) state of each GPU at the last split: a GPU going idle or busy re-splits */
static uint8_t gpu_split_busy[MAX_GPUS_SUPPORTED];
static suscription_t *sus_gpu;

static state_t int_set_powercap_value(uint32_t limit, uint32_t *gpu_util);

static domain_settings_t settings = {.node_ratio = 0.02, .security_range = 0.01};

static char gpu_dvfs_greedy = 0;

ulong select_lower_gpu_freq(uint i, ulong f, uint pstate_steps);
ulong select_higher_gpu_freq(uint i, ulong f, int extra_steps);

/* Watts GPU i needs over its budget to run at its target clock, from the power it draws now */
static uint gpu_needed_power(uint i)
{
    double at_target =
        power_estimator_at(&gpu_est[i], values_gpu_diff[i].power_w, c_freq[i], ear_max(t_freq[i], c_freq[i]));
    return (at_target > gpu_pc_curr_power[i]) ? (uint) (at_target - gpu_pc_curr_power[i]) : 0;
}

/* Highest supported clock <= khz (the list is sorted from the highest) */
static ulong gpu_freq_at_most(uint i, ulong khz)
{
    for (uint j = 0; j < gpu_num_freqs[i]; j++)
        if (gpu_freq_list[i][j] <= khz)
            return gpu_freq_list[i][j];
    return gpu_freq_list[i][gpu_num_freqs[i] - 1];
}

/* Next clock: a move proportional to the power gap over the GPU's W/kHz estimate,
 * damped by CTRL_GAIN and bounded per period */
static ulong gpu_next_freq(uint i, ulong cur, double power, double cap, ulong target)
{
    ulong fmax = gpu_freq_list[i][0], fmin = gpu_freq_list[i][gpu_num_freqs[i] - 1];
    double range     = (fmax > fmin) ? (double) (fmax - fmin) : 1.0;
    double w_per_khz = (gpu_est[i].w_per_khz > 0) ? gpu_est[i].w_per_khz : 300.0 / range;

    double gap = cap - power; /* > 0: headroom, < 0: over the cap */
    if (gap < 0 && -gap <= CTRL_DEADBAND_W)
        return cur;
    if (gap >= 0 && (gap < CTRL_RAISE_W || cur >= target))
        return cur;
    double dkhz = ear_min(fabs(gap) * CTRL_GAIN / w_per_khz, range / CTRL_MAX_STEP);
    if (gap < 0) {
        ulong next = gpu_freq_at_most(i, (cur > fmin + dkhz) ? cur - (ulong) dkhz : fmin);
        /* over the cap: at least one step down */
        return (next == cur) ? select_lower_gpu_freq(i, cur, 1) : next;
    }
    ulong next = gpu_freq_at_most(i, cur + (ulong) dkhz);
    /* headroom left: at least one step up */
    if (next <= cur)
        next = select_higher_gpu_freq(i, cur, 1);
    return ear_min(next, target);
}

ulong select_lower_gpu_freq(uint i, ulong f, uint pstate_steps)
{
    int j = 0, found = 0;
    /* If we are the minimum*/
    // debug("Looking for lower freq for %lu",f);
    if (f == gpu_freq_list[i][gpu_num_freqs[i] - 1])
        return f;
    /* Otherwise, look for f*/
    do {
        if (gpu_freq_list[i][j] == f)
            found = 1;
        else
            j++;
    } while ((found == 0) && (j != gpu_num_freqs[i]));
    if (!found) {
        // debug("Frequency %lu not found in GPU %u",f,i);
        return gpu_freq_list[i][gpu_num_freqs[i] - 1];
    } else {
        // debug("Frequency %lu found in position %d",f,j);
    }
    if (j < (gpu_num_freqs[i] - 1)) {
        /* When reducing GPU frequency we do it in steps of N instead of 1
         * since the frequencies list for GPUs is very fine-grained and one
         * step does not reduce the total power consumption by much*/
        return gpu_freq_list[i][ear_min(j + pstate_steps, gpu_num_freqs[i] - 1)];
    }
    return f;
}

ulong select_higher_gpu_freq(uint i, ulong f, int extra_steps)
{
    int j = 0, found = 0;
    // debug("Looking for higher freq for %lu",f);
    /* If we are the maximum*/
    if (f == gpu_freq_list[i][0])
        return f;
    /* Otherwise, look for f*/
    do {
        if (gpu_freq_list[i][j] == f)
            found = 1;
        else
            j++;
    } while ((found == 0) && (j != gpu_num_freqs[i]));
    if (!found) {
        debug("Frequency %lu not found in GPU %u", f, i);
        return gpu_freq_list[i][0];
    } else {
        debug("Frequency %lu found in position %d returning %d", f, j, ear_max(j - extra_steps, 0));
    }
    return gpu_freq_list[i][ear_max(j - extra_steps, 0)];
}

/*
   static void printf_gpu_freq_list(const ulong **f, const uint *num_f)
   {
   int i,j;
#if SHOW_DEBUGS
for (i=0;i<gpu_pc_num_gpus;i++){
debug("GPU[%d]------------- limit %u",i,num_f[i]);
for (j=0;j<num_f[i];j++){
debug("Freq[%d]=%lu",j,f[i][j]);
}
}
#endif
}*/

/************************ This function is called by the monitor before the iterative part ************************/
state_t gpu_dvfs_pc_thread_init(void *p)
{
    gpu_load(NO_EARD);
    gpu_data_alloc(&values_gpu_init);
    gpu_data_alloc(&values_gpu_end);
    gpu_data_alloc(&values_gpu_diff);
    mgt_gpu_freq_list(no_ctx, &gpu_freq_list, &gpu_num_freqs);
    for (uint i = 0; i < gpu_pc_num_gpus && gpu_est != NULL; i++)
        power_estimator_init(&gpu_est[i], gpu_pc_min_power[i], gpu_pc_max_power[i],
                             gpu_freq_list[i][gpu_num_freqs[i] - 1], gpu_freq_list[i][0]);
    gpu_dvfs_pc_enabled = 1;
    debug("Power measurement initialized in gpu_dvfs_pc thread initialization");
    if (gpu_read(values_gpu_init) != EAR_SUCCESS) {
        debug("Error in gpu_read in gpu_dvfs_pc");
        gpu_dvfs_pc_enabled = 0;
    }
    gpu_dvfs_monitor_initialized = 1;
    return EAR_SUCCESS;
}

/************************ This function is called by the monitor in iterative part ************************/
state_t gpu_dvfs_pc_thread_main(void *p)
{
    uint i, j;
    uint gpu_dvfs_local_util = 0;

    if (!gpu_dvfs_pc_enabled)
        return EAR_SUCCESS;
    if (gpu_read(values_gpu_end) != EAR_SUCCESS) {
        debug("Error in gpu_read gpu_dvfs_pc");
        return EAR_ERROR;
    }

    pmgt_get_app_req_freq(DOMAIN_GPU, t_freq, gpu_pc_num_gpus);
    /* Calcular power */
    gpu_data_diff(values_gpu_end, values_gpu_init, values_gpu_diff);
    if (values_gpu_diff[0].power_w == 0)
        return EAR_SUCCESS;
    mgt_gpu_freq_limit_get_current(no_ctx, c_freq);
    /* Copiar init=end */
    gpu_data_copy(values_gpu_init, values_gpu_end);
    /* If utilisation has changed we internally redistribute powercap */
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        gpu_dvfs_local_util += values_gpu_diff[i].util_gpu;
        gpu_pc_util[i]   = values_gpu_diff[i].util_gpu;
        gpu_pc_excess[i] = (long) gpu_pc_curr_power[i] - (long) values_gpu_diff[i].power_w;
    }
    uint busy_changed = 0;
    for (i = 0; i < gpu_pc_num_gpus && i < MAX_GPUS_SUPPORTED; i++)
        busy_changed |= ((gpu_pc_util[i] > 0) != gpu_split_busy[i]);
    if (busy_changed || util_changed(gpu_dvfs_local_util, gpu_dvfs_util)) {
        int_set_powercap_value(current_gpu_pc, gpu_pc_util);
    }

    gpu_dvfs_greedy = 0;
    // if ((current_gpu_pc <= 1) || (c_status != PC_STATUS_RUN)){
    if (current_gpu_pc <= 1) {
        return EAR_SUCCESS;
    }

    // redistribute power between gpus
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        // if (gpu_pc_util[i] == 0) continue; //idle nodes need no reallocation
        if (gpu_pc_excess[i] > 0)
            continue; // nodes with extra power need no reallocation
        for (j = 0; j < gpu_pc_num_gpus; j++) {
            if (gpu_pc_util[j] == 0)
                continue; // we don't want to remove power from idle nodes
            /* We leave a minimum of 5 Watts of excess power in case of power fluctuations. */
            if (gpu_pc_excess[j] > 5) {
                debug("moving %ld watts from %d to %d (current pc from %lu to %lu)", gpu_pc_excess[j] - 5, j, i,
                      gpu_pc_curr_power[j], gpu_pc_curr_power[i]);
                gpu_pc_curr_power[i] +=
                    gpu_pc_excess[j] - 5; // we leave a bit of excess power to prevent fluctuation errors
                gpu_pc_curr_power[j] -= gpu_pc_excess[j] - 5; // remove the power from the original gpu
                gpu_pc_excess[i] += gpu_pc_excess[j] - 5;
                gpu_pc_excess[j] = 5;

                if (gpu_pc_excess[i] >= 0)
                    break; // if we have enough power in the gpu we can stop redistributing towards it
            }
        }
    }

    for (i = 0; i < gpu_pc_num_gpus; i++) {
        if (t_freq[i] == 0)
            t_freq[i] = gpu_freq_list[i][0]; // set the nominal if no GPU freq was specified
        if (gpu_pc_util[i] == 0) {
            gpu_pc_util_wait[i] += UTIL_INC;
            /* if the GPU has been unused  for UTIL_WAIT we set its freq to the minimum */
            if (gpu_pc_util_wait[i] % UTIL_WAIT == 0) {
                gpu_pc_prev_freq[i] = ear_max(c_freq[i], gpu_pc_prev_freq[i]);
                n_freq[i]           = gpu_freq_list[i][gpu_num_freqs[i] - 1];
                debug("util at 0, setting gpu-freq to minimum");
            }
            debug("util at 0, increasing util_wait (current %lu, needed %u)", gpu_pc_util_wait[i], UTIL_WAIT);
            continue;
        }
        /* if the frequency had been set to the minimum (due to util), reset it to the previous one */
        if (gpu_pc_prev_freq[i] > 0) {
            c_freq[i]           = gpu_pc_prev_freq[i];
            gpu_pc_prev_freq[i] = 0;
        }
        if (t_freq[i] > 0)
            c_freq[i] = ear_min(c_freq[i], t_freq[i]);
        n_freq[i] = c_freq[i];
        power_estimator_sample(&gpu_est[i], values_gpu_diff[i].power_w, c_freq[i]);
        /* Aplicar limites */
        // debug("GPU[%d] power %lf limit %lu",i,values_gpu_diff[i].power_w,gpu_pc_curr_power[i]);
        if (values_gpu_diff[i].power_w > gpu_pc_curr_power[i]) { /* We are above the PC */
            if (gpu_dvfs_status == PC_STATUS_RELEASE) {
                debug("gpu_dvfs setting ask_default=1");
                gpu_dvfs_ask_def = 1;
            }
            /* clock cut proportional to the overshoot (watts, not a step count) */
            n_freq[i] = gpu_next_freq(i, c_freq[i], values_gpu_diff[i].power_w, gpu_pc_curr_power[i], t_freq[i]);
            gpu_dvfs_greedy |= 1;
            debug("%sReducing the GPU-freq[%d] from %lu to %lu (cpower %lf limit %lu) %s", COL_RED, i, c_freq[i],
                  n_freq[i], values_gpu_diff[i].power_w, gpu_pc_curr_power[i], COL_CLR);
        } else { /* We are below the PC */
            if ((c_freq[i] < t_freq[i]) && (t_freq[i] > 0)) {
                n_freq[i] = gpu_next_freq(i, c_freq[i], values_gpu_diff[i].power_w, gpu_pc_curr_power[i], t_freq[i]);
                debug("%sIncreasing the GPU-freq[%d] from %lu to %lu (power %lf, limit %lu)%s", COL_GRE, i, c_freq[i],
                      n_freq[i], values_gpu_diff[i].power_w, gpu_pc_curr_power[i], COL_CLR);

                // we can't reach target frequency, check if ask_default
                if (gpu_dvfs_status == PC_STATUS_RELEASE && (t_freq[i] > 0) && (n_freq[i] < t_freq[i])) {
                    gpu_dvfs_ask_def = 1;
                }
            }
        }
    } /* end for loop */

    mgt_gpu_freq_limit_set(no_ctx, n_freq);

    return EAR_SUCCESS;
}

state_t disable()
{
    gpu_pc_enabled = 0;

    return EAR_SUCCESS;
    /* Stop thread */
}

state_t enable(suscription_t *sus)
{
    state_t ret = EAR_SUCCESS;
    debug("GPU-DVFS");
    /* Suscription is for gpu power monitoring */
    if (sus == NULL) {
        debug("NULL subscription in GPU-DVFS powercap");
        return EAR_ERROR;
    }
    sus->call_main  = gpu_dvfs_pc_thread_main;
    sus->call_init  = gpu_dvfs_pc_thread_init;
    sus->time_relax = 60000;
    sus->time_burst = 1000;
    /* Init data */
    debug("GPU_DVFS: power cap  enable");
    mgt_gpu_load(NO_EARD);
    if ((ret = mgt_gpu_init(no_ctx)) != EAR_SUCCESS) {
        debug("Error in mgt_gpu_init");
    } else {
        gpu_pc_enabled = 1;
        mgt_gpu_count_devices(no_ctx, &gpu_pc_num_gpus);
        debug("%d GPUS detectd in gpu_powercap", gpu_pc_num_gpus);
        gpu_pc_min_power = calloc(gpu_pc_num_gpus, sizeof(ulong));
        if (gpu_pc_min_power == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_max_power = calloc(gpu_pc_num_gpus, sizeof(ulong));
        if (gpu_pc_max_power == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_curr_power = calloc(gpu_pc_num_gpus, sizeof(ulong));
        if (gpu_pc_curr_power == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_excess = calloc(gpu_pc_num_gpus, sizeof(long));
        if (gpu_pc_excess == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_util = calloc(gpu_pc_num_gpus, sizeof(ulong));
        if (gpu_pc_util == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_util_wait = calloc(gpu_pc_num_gpus, sizeof(ulong));
        if (gpu_pc_util_wait == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_prev_freq = calloc(gpu_pc_num_gpus, sizeof(ulong));
        if (gpu_pc_prev_freq == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        pdist = calloc(gpu_pc_num_gpus, sizeof(float));
        if (pdist == NULL) {
            gpu_pc_enabled = 0;
            ret            = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        c_freq  = calloc(gpu_pc_num_gpus, sizeof(ulong));
        gpu_est = calloc(gpu_pc_num_gpus, sizeof(power_estimator_t));
        t_freq  = calloc(gpu_pc_num_gpus, sizeof(ulong));
        n_freq  = calloc(gpu_pc_num_gpus, sizeof(ulong));
        mgt_gpu_power_cap_get_rank(no_ctx, gpu_pc_min_power, gpu_pc_max_power);
        gpu_data_alloc(&values_gpu_idle);
    }
    sus->suscribe(sus);
    sus_gpu = sus;
    debug("GPU-DVFS Subscription done");
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

void restore_frequency()
{
    int i;
    pmgt_get_app_req_freq(DOMAIN_GPU, t_freq, gpu_pc_num_gpus);
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        if (t_freq[i] == 0)
            t_freq[i] = gpu_freq_list[i][0]; // set the nominal if no GPU freq was specified
        n_freq[i] = t_freq[i];
    }
    mgt_gpu_freq_limit_set(no_ctx, n_freq);
}

state_t set_powercap_value(uint pid, uint domain, uint32_t *limit, uint32_t *gpu_util)
{
    gpu_dvfs_status  = PC_STATUS_OK;
    gpu_dvfs_ask_def = 0;

    if (domain == LEVEL_DEVICE) {
        uint32_t new_limit = 0;
        for (int32_t i = 0; i < gpu_pc_num_gpus; i++) {
            new_limit += limit[i];
        }
        warning("Per device powercap not implemented for GPU DVFS, adding up the values (new limit %u)", new_limit);
        default_gpu_pc = new_limit;
    } else {
        default_gpu_pc = *limit;
    }
    return int_set_powercap_value(default_gpu_pc, gpu_util);
}

state_t reset_powercap_value()
{
    debug("Reset powercap value to %lu", default_gpu_pc);
    gpu_dvfs_status  = PC_STATUS_OK;
    gpu_dvfs_ask_def = 0;
    return int_set_powercap_value(default_gpu_pc, gpu_pc_util);
}

state_t release_powercap_allocation(uint decrease)
{
    debug("GPU_DVFS:Releasing %u from powercap allocationg", decrease);
    current_gpu_pc -= decrease;
    gpu_dvfs_status = PC_STATUS_RELEASE;
    return int_set_powercap_value(current_gpu_pc, gpu_pc_util);
}

state_t increase_powercap_allocation(uint increase)
{
    debug("GPU_DVFS:Increasing %u from powercap allocationg", increase);
    current_gpu_pc += increase;
    if (current_gpu_pc >= default_gpu_pc)
        gpu_dvfs_status = PC_STATUS_OK;
    return int_set_powercap_value(current_gpu_pc, gpu_pc_util);
}

static state_t int_set_powercap_value(uint32_t limit, uint32_t *gpu_util)
{
    int i;
    float alloc;
    uint32_t ualloc;
    uint32_t total_util = 0;
    /* Set data */
    current_gpu_pc = limit;
    debug("%sGPU-DVFS:set_powercap_value %u%s", COL_BLU, limit, COL_CLR);
    if (current_gpu_pc == POWER_CAP_UNLIMITED) {
        restore_frequency();
        return EAR_SUCCESS;
    }

    if (gpu_read_raw(values_gpu_idle) != EAR_SUCCESS) {
        debug("Error in gpu_read_raw gpu_dvfs_pc (int_set_powercap)");
        return EAR_ERROR;
    }
    // debug("%s",COL_BLU);
    // debug("GPU: set_powercap_value %lu",limit);
    // debug("GPU: Phase 1, allocating power to idle GPUS");
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        pdist[i] = 0.0;
        total_util += gpu_util[i];
        if (gpu_util[i] == 0) {
            gpu_pc_curr_power[i] = values_gpu_idle[i].power_w + 5;
            gpu_idle_power       = values_gpu_idle[i].power_w + 5;
            limit                = limit - gpu_pc_curr_power[i];
        }
    }
    gpu_dvfs_util = total_util;
    for (i = 0; i < gpu_pc_num_gpus && i < MAX_GPUS_SUPPORTED; i++)
        gpu_split_busy[i] = (gpu_util[i] > 0);
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        if (gpu_util[i] > 0) {
            pdist[i] = (float) gpu_util[i] / (float) total_util;
            alloc    = (float) limit * pdist[i];
            ualloc   = (uint32_t) alloc;
            if (ualloc > gpu_pc_max_power[i])
                ualloc = gpu_pc_max_power[i];
            gpu_pc_curr_power[i] = ualloc;
        }
    }
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        // debug("GPU: util_gpu[%d]=%lu power_alloc=%lu",i,gpu_util[i],gpu_pc_curr_power[i]);
    }
    memcpy(gpu_pc_util, gpu_util, sizeof(uint32_t) * gpu_pc_num_gpus);
    // debug("%s",COL_CLR);
    return EAR_SUCCESS;
}

state_t get_powercap_value(uint pid, uint32_t *powercap)
{
    /* copy data */
    // debug("GPU::get_powercap_value");
    *powercap = current_gpu_pc;
    return EAR_SUCCESS;
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
    char cmd[128];
    int i  = 0;
    cmd[0] = '\0';
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        sprintf(cmd, "GPU_powercap[%d]=%u ", i, current_gpu_pc);
        strcat(b, cmd);
    }
}

void set_status(uint status)
{
    debug("GPU. set_status %u", status);
    c_status = status;
}

uint get_powercap_strategy()
{
    debug("GPU. get_powercap_strategy");
    return PC_DVFS;
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
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        debug("GPU: util_gpu[%d]=%u", i, util[i]);
    }
    int_set_powercap_value(current_gpu_pc, util);
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
    uint32_t used   = 0;
    uint g_tbr      = 0;
    uint tmp_stress = 0;
    uint requested  = 0;
    uint greedy     = 0;

    status->ok         = PC_STATUS_OK;
    status->exceed     = 0;
    status->requested  = 0;
    status->stress     = 0;
    status->current_pc = current_gpu_pc;

    // debug("GPU_DVFS: get_powercap_status");
    if (!gpu_dvfs_monitor_initialized)
        return 0;
    if (current_gpu_pc == POWER_CAP_UNLIMITED) {
        return 0;
    }
    /* We need the released power */
    if (gpu_dvfs_ask_def) {
        debug("GPU status ASK_DEF, current_pc %u and def_pc %u", current_gpu_pc, default_gpu_pc);
        status->ok = PC_STATUS_ASK_DEF;
        return 0;
    }

    /*for (i=0;i<gpu_pc_num_gpus;i++){
      debug("total util is 0, GPU %d util %lu", i, gpu_pc_util[i]);
      }*/
    /* If we are not using th GPU we can release all the power */
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        used += (gpu_pc_util[i] > 0);
        if (t_freq[i] == 0)
            return 0;
    }
    if (!used) {
        status->ok = PC_STATUS_RELEASE;
        if (gpu_pc_num_gpus == 0)
            status->exceed = current_gpu_pc;
        else
            status->exceed = (current_gpu_pc - (gpu_idle_power * gpu_pc_num_gpus));
        if (status->exceed >= current_gpu_pc)
            status->exceed = 0;
        debug("%sCould release %u Watts from the GPU%s", COL_GRE, status->exceed, COL_CLR);
        return 1;
    }
    /* If we know, we must check */
    status->ok = PC_STATUS_RELEASE;

    /* Compute the current stress before checking the status */
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        if (gpu_pc_util[i] > 0)
            tmp_stress +=
                powercap_get_stress(gpu_freq_list[i][0], gpu_freq_list[i][gpu_num_freqs[i] - 1], t_freq[i], c_freq[i]);
    }
    tmp_stress /= used;
    status->stress = tmp_stress;

    for (i = 0; i < gpu_pc_num_gpus; i++) {
        if ((c_freq[i] < t_freq[i]) && (gpu_pc_util[i] > 0) && gpu_dvfs_greedy) {
            debug("We cannot release power from GPU %d", i);
            greedy = 1;
            requested += powercap_device_request(gpu_pc_max_power[i], gpu_pc_curr_power[i], gpu_needed_power(i));
        } else if ((c_freq[i] >= t_freq[i]) && (gpu_pc_util[i] > 0)) {
            if (values_gpu_diff[i].power_w >
                gpu_pc_curr_power[i]) // if we are at req_f but we are using more power than we should
            {
                greedy = 1;
                requested += powercap_device_request(gpu_pc_max_power[i], gpu_pc_curr_power[i], gpu_needed_power(i));
                debug("GPU_DVFS GPU %d greedy because we need more power to maintain current speed", i);
                continue;
            }
            g_tbr          = (uint) (((double) gpu_pc_curr_power[i] - values_gpu_diff[i].power_w) * 0.5);
            status->exceed = status->exceed + g_tbr;
            debug("gpu_pc_curr_power %u and current_power %lf, exceed: %u", gpu_pc_curr_power[i],
                  values_gpu_diff[i].power_w, status->exceed);
            debug("%sWe can release %u W from GPU %d since target = %lu current %lu%s", COL_GRE, g_tbr, i, t_freq[i],
                  c_freq[i], COL_CLR);
        }
    }
    /* Throttled GPUs already at their max power can't use more: not greedy */
    if (greedy && requested) {
        status->ok        = PC_STATUS_GREEDY;
        status->requested = ear_min(requested, UINT16_MAX);
        status->exceed    = 0;
        debug("GPU_DVFS status GREEDY, requesting %u W", status->requested);
        return 0;
    }
    if (status->exceed < MIN_GPU_POWER_MARGIN) {
        status->exceed = 0;
        status->ok     = PC_STATUS_OK;
        debug("GPU_DVFS status OK");
        return 0;
    }

    return 1;
}
