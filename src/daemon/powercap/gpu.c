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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <pthread.h>
#include <common/config.h>
#include <signal.h>
//#define SHOW_DEBUGS 1
#include <common/colors.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/system/execute.h>
#include <metrics/gpu/gpu.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_mgt.h>
#include <management/gpu/gpu.h>
#include <common/system/monitor.h>




static ulong current_gpu_pc=0;
static ulong default_gpu_pc=0;
static uint gpu_pc_enabled=0;
static uint c_status=PC_STATUS_IDLE;
static uint c_mode=PC_MODE_LIMIT;

static ctx_t gpu_pc_ctx;
static ctx_t gpu_pc_ctx_data;
static gpu_t *values_gpu_init,*values_gpu_end,*values_gpu_diff;
static uint gpu_pc_num_gpus;
static ulong *gpu_pc_min_power;
static ulong *gpu_pc_max_power;
static ulong *gpu_pc_curr_power;
static ulong *gpu_pc_util;
static long  *gpu_pc_excess;
static const ulong **gpu_freq_list = NULL;
static const uint *gpu_num_freqs = NULL;
static ulong *t_freq;
static float *pdist;
static gpu_t *gpu_pc_power_data;

static ulong gpu_pc_total_util = 0;
static double *gpu_cons_power;
static ulong *gpu_freq;


#define MIN_GPU_IDLE_POWER 40

#if SHOW_DEBUGS
static uint debug_count = 0;
#define DEBUG_FREQ 5
#endif

static int gpu_freq_to_pstate(int gpuid,ulong f)
{
  int i = 0;
  while(i < gpu_num_freqs[gpuid]-1){
    if (gpu_freq_list[gpuid][i] == f) return i;
    if (gpu_freq_list[gpuid][i] < f) return i-1;
    i++;
  }
	return gpu_num_freqs[gpuid]-1;
}

static state_t int_set_powercap_value(ulong limit,ulong *gpu_util);
/************************ This function is called by the monitor before the iterative part ************************/
state_t gpu_pc_thread_init(void *p)
{

    mgt_gpu_freq_list(&gpu_pc_ctx, &gpu_freq_list, &gpu_num_freqs);    
    if (gpu_read(&gpu_pc_ctx_data,values_gpu_init)!= EAR_SUCCESS){
        debug("Error in gpu_read in gpu_dvfs_pc");
        return EAR_ERROR;
    }
    return EAR_SUCCESS;
}
/************************ This function is called by the monitor in iterative part ************************/
state_t gpu_pc_thread_main(void *p)
{
    uint i, j;
    uint gpu_local_util = 0;

    if (!gpu_pc_enabled) return EAR_SUCCESS;
    if (gpu_read(&gpu_pc_ctx_data,values_gpu_end) != EAR_SUCCESS){
        debug("Error in gpu_read gpu_dvfs_pc");
        return EAR_ERROR;
    }
    /* Update target frequency from EARL */
    pmgt_get_app_req_freq(DOMAIN_GPU, t_freq, gpu_pc_num_gpus);

    /* Metrics computation */
    gpu_data_diff(values_gpu_end, values_gpu_init, values_gpu_diff);
    /* Copy end values to new init */
    gpu_data_copy(values_gpu_init,values_gpu_end);
    for (i = 0; i < gpu_pc_num_gpus; i++) {
        gpu_local_util += values_gpu_diff[i].util_gpu;
        gpu_pc_util[i] = values_gpu_diff[i].util_gpu;
        gpu_cons_power[i] = values_gpu_diff[i].power_w;
        gpu_freq[i] = values_gpu_diff[i].freq_gpu;
        gpu_pc_excess[i] = (long)gpu_pc_curr_power[i] - (long)gpu_cons_power[i];
        debug("gpu %d has excess %ld (current power %.2lf and current pc %lu)", i, gpu_pc_excess[i], gpu_cons_power[i], gpu_pc_curr_power[i]);
    }
    //redistribute power between gpus
    for (i = 0; i < gpu_pc_num_gpus; i++)
    {
        if (t_freq[i] == 0) t_freq[i] = gpu_freq_list[i][0]; //set the nominal if no GPU freq was specified
        if (gpu_pc_util[i] == 0) continue; //idle nodes need no reallocation
        if (gpu_pc_excess[i] > 0) continue; //nodes with extra power need no reallocation
        for (j = 0; j < gpu_pc_num_gpus; j++) {
            if (gpu_pc_util[j] == 0) continue; //we don't want to remove power from idle nodes
            if (gpu_pc_excess[j] > 5) {
                debug("moving %ld watts from %d to %d (current pc from %lu to %lu)", gpu_pc_excess[j]-5, j, i, gpu_pc_curr_power[j], gpu_pc_curr_power[i]);
                gpu_pc_curr_power[i] += gpu_pc_excess[j] - 5; //we leave a bit of excess power to prevent fluctuation errors
                gpu_pc_curr_power[j] -= gpu_pc_excess[j] - 5; //remove the power from the original gpu
                gpu_pc_excess[i] += gpu_pc_excess[j] - 5;
                gpu_pc_excess[j] = 5;

                if (gpu_pc_excess[i] >= 0) break; //if we have enough power in the gpu we can stop redistributing towards it
            }
        }
    }
#if SHOW_DEBUGS
    debug_count++;
    if (debug_count % DEBUG_FREQ == 0)
        debug("Current gpu_local_util %u", gpu_local_util);
#endif
    if (util_changed(gpu_local_util,gpu_pc_total_util)){
        debug("GPU:Internal power redistribution because util (%u -> %lu)",gpu_local_util,gpu_pc_total_util);
        int_set_powercap_value(current_gpu_pc,gpu_pc_util);
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
    state_t ret=EAR_SUCCESS;
    debug("GPU: power cap  enable");
    mgt_gpu_load(NULL);
    if ((ret = mgt_gpu_init(&gpu_pc_ctx)) != EAR_SUCCESS){
        debug("Error in mgt_gpu_init");
    }else{
        gpu_pc_enabled = 1;
        mgt_gpu_count(&gpu_pc_ctx,&gpu_pc_num_gpus);
        debug("%d GPUS detectd in gpu_powercap",gpu_pc_num_gpus);
        gpu_pc_min_power = calloc(gpu_pc_num_gpus,sizeof(ulong));
        if (gpu_pc_min_power == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_max_power = calloc(gpu_pc_num_gpus,sizeof(ulong));
        if (gpu_pc_max_power == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_curr_power = calloc(gpu_pc_num_gpus,sizeof(ulong));
        if (gpu_pc_curr_power == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_cons_power = calloc(gpu_pc_num_gpus,sizeof(ulong));
        if (gpu_cons_power == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }

        gpu_pc_excess = calloc(gpu_pc_num_gpus, sizeof(ulong));
        if (gpu_pc_excess == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }

        gpu_freq = calloc(gpu_pc_num_gpus, sizeof(ulong));
        if (gpu_freq == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }

        gpu_pc_util = calloc(gpu_pc_num_gpus,sizeof(uint));
        if (gpu_pc_util == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        pdist = calloc(gpu_pc_num_gpus,sizeof(float));
        if (pdist == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        t_freq=calloc(gpu_pc_num_gpus,sizeof(ulong));

        mgt_gpu_power_cap_get_rank(&gpu_pc_ctx,gpu_pc_min_power,gpu_pc_max_power);
    }
    if (gpu_load(NULL,0,NULL)!= EAR_SUCCESS){
        error("Initializing GPU(load) in gpu plugin");
    }
    if (gpu_init(&gpu_pc_ctx_data)!=EAR_SUCCESS){
        error("Initializing GPU in GPU powercap plugin");
    }
    if (gpu_data_alloc(&gpu_pc_power_data)!=EAR_SUCCESS){
        error("Initializing GPU data in powercap");
    }
    gpu_data_alloc(&values_gpu_init);
    gpu_data_alloc(&values_gpu_end);
    gpu_data_alloc(&values_gpu_diff);

    /* Suscription is for gpu power monitoring */
    if (sus == NULL){
        debug("NULL subscription in GPU-DVFS powercap");
        return EAR_ERROR;
    }
    sus->call_main = gpu_pc_thread_main;
    sus->call_init = gpu_pc_thread_init;
    sus->time_relax = 1000;
    sus->time_burst = 1000;
    sus->suscribe(sus);

    return ret;
}

state_t set_powercap_value(uint pid,uint domain,ulong limit,ulong *gpu_util)
{
    debug("GPU: powermgr gpu power allocated %lu",limit);
    default_gpu_pc = limit;
    c_status = PC_STATUS_OK;
    return int_set_powercap_value(limit, gpu_util);
}

state_t reset_powercap_value()
{
    debug("GPU: Resetting powercap value to %lu", default_gpu_pc);
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
    if (current_gpu_pc >= default_gpu_pc) c_status = PC_STATUS_OK;
    return int_set_powercap_value(current_gpu_pc, gpu_pc_util);
}

static state_t int_set_powercap_value(ulong limit,ulong *gpu_util)
{
    state_t ret;
    int i;
    float alloc,ualloc;
    ulong gpu_idle=0,gpu_run=0,total_util=0;
    current_gpu_pc=limit;
    /* Set data */
    debug("%s",COL_BLU);
    debug("GPU: set_powercap_value %lu",limit);
    debug("GPU: Phase 1, allocating power to idle GPUS");
    for (i=0;i<gpu_pc_num_gpus;i++){	
        pdist[i] = 0.0;
        total_util += gpu_util[i];
        if (gpu_util[i] == 0){ 
            gpu_idle++;
            gpu_pc_curr_power[i] = gpu_pc_min_power[i];
            limit = limit - MIN_GPU_IDLE_POWER;
        }else{
            gpu_run++;
        }
    }
    debug("GPU: Phase 2: Allocating power to %lu running GPUS",gpu_run);
    for (i=0;i<gpu_pc_num_gpus;i++){
        if (gpu_util[i]>0){
            pdist[i] = (float)gpu_util[i]/(float)total_util;
            alloc = (float)limit*pdist[i];
            ualloc = alloc;
            if (ualloc > gpu_pc_max_power[i]) ualloc = gpu_pc_max_power[i];
            if (ualloc < gpu_pc_min_power[i]) ualloc = gpu_pc_min_power[i];
            gpu_pc_curr_power[i] = ualloc;
        }
    }
    ret = mgt_gpu_power_cap_set(&gpu_pc_ctx,gpu_pc_curr_power);
    if (ret == EAR_ERROR) debug("Error setting powercap");
    for (i=0;i<gpu_pc_num_gpus;i++) {
        debug("GPU: util_gpu[%d]=%lu power_alloc=%lu",i,gpu_util[i],gpu_pc_curr_power[i]);
    }
    memcpy(gpu_pc_util,gpu_util,sizeof(ulong)*gpu_pc_num_gpus);
    debug("%s",COL_CLR);
    return EAR_SUCCESS;
}

state_t get_powercap_value(uint pid,ulong *powercap)
{
    /* copy data */
    debug("GPU::get_powercap_value");
    *powercap=current_gpu_pc;
    return EAR_SUCCESS;
}

uint is_powercap_policy_enabled(uint pid)
{
    return gpu_pc_enabled;
}

void print_powercap_value(int fd)
{
    dprintf(fd,"gpu_powercap_value %lu\n",current_gpu_pc);
}
void powercap_to_str(char *b)
{
    sprintf(b,"%lu",current_gpu_pc);
}

void set_status(uint status)
{
    debug("GPU. set_status %u",status);
    c_status=status;
}
uint get_powercap_strategy()
{
    debug("GPU. get_powercap_strategy");
    return PC_POWER;
}

void set_pc_mode(uint mode)
{
    debug("GPU. set_pc_mode");
    c_mode=mode;
}


void set_verb_channel(int fd)
{
    WARN_SET_FD(fd);
    VERB_SET_FD(fd);
    DEBUG_SET_FD(fd);
}

void set_new_utilization(ulong *util)
{
    int i;
    for (i=0;i<gpu_pc_num_gpus;i++) {
        debug("GPU: util_gpu[%d]=%lu",i,util[i]);
    }
    int_set_powercap_value(current_gpu_pc,util);
    memcpy(gpu_pc_util,util,sizeof(ulong)*gpu_pc_num_gpus);
}

void set_app_req_freq(ulong *f)
{
    int i;
    for (i=0;i<gpu_pc_num_gpus;i++) {
        //debug("GPU_DVFS:GPU %d Requested application freq set to %lu",i,f[i]); 
        t_freq[i]=f[i];
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
    uint used=0;
    uint g_tbr = 0;
    uint tmp_stress = 0;
    status->ok = PC_STATUS_OK;
    status->exceed = 0;
		status->requested = 0;
    status->stress = 0;
    status->current_pc = current_gpu_pc;

    if (current_gpu_pc == PC_UNLIMITED){
        return 0;
    }
    /* If we are not using th GPU we can release all the power */
    for (i=0;i<gpu_pc_num_gpus;i++){
        used += (gpu_pc_util[i]>0);
        if (t_freq[i] == 0) return 0;
    }
    if (!used){
        status->ok = PC_STATUS_RELEASE;
        if (gpu_pc_num_gpus == 0) status->exceed = current_gpu_pc;
        else status->exceed=(current_gpu_pc - (MIN_GPU_IDLE_POWER*gpu_pc_num_gpus));
        //debug("%sReleasing %u Watts from the GPU%s",COL_GRE,status->exceed,COL_CLR);
        return 1;
    }

    for (i = 0; i < gpu_pc_num_gpus; i++) {
        if (t_freq[i] > 0 && gpu_pc_util[i]) {
            tmp_stress += powercap_get_stress(gpu_freq_list[i][0], gpu_freq_list[i][gpu_num_freqs[i]-1], t_freq[i], gpu_freq[i]);
            debug("gpu %d tmp_stress: %u (current freq %lu target_freq %lu)", i, tmp_stress, gpu_freq[i], t_freq[i]);
        }
    }
    tmp_stress /= used;
    status->stress = tmp_stress;

    /* If we know, we must check */
    status->ok = PC_STATUS_RELEASE;
    status->exceed = 0;
    for (i=0;i<gpu_pc_num_gpus;i++){
        /* gpu_pc_util is an average during a period , is more confident than an instantaneous measure*/
        if ((t_freq[i] > gpu_freq[i]) && (gpu_pc_util[i]>0)){ 
            //debug("We cannot release power from GPU %d",i);
            if (c_status == PC_STATUS_RELEASE) {
                debug("GPU status ASK_DEF, current_pc %lu and def_pc %lu", current_gpu_pc, default_gpu_pc);
                status->ok = PC_STATUS_ASK_DEF;
            } else {
                status->requested += ear_max(gpu_freq_to_pstate(i,t_freq[i]) - gpu_freq_to_pstate(i,gpu_freq[i]),0);
                status->ok = PC_STATUS_GREEDY;
            }
            status->exceed = 0;
            return 0;
        }else{
            if (gpu_cons_power[i] > gpu_pc_curr_power[i])  //if we are at req_f but using more power than we should
            {
                status->ok = PC_STATUS_GREEDY;
                status->requested += ear_max(gpu_freq_to_pstate(i,t_freq[i]) - gpu_freq_to_pstate(i,gpu_freq[i]),0);
                status->exceed = 0;
                debug("GPU status is greedy because more power is needed to mantain current speed");
                return 0;
            }
            /* However we use instanteneous power to compute potential power releases */
            g_tbr = (uint)((gpu_pc_curr_power[i] - gpu_cons_power[i]) *0.5);
            status->exceed = status->exceed + g_tbr;
            //debug("%sWe can release %u W from GPU %d since target = %lu current %lu%s",COL_GRE,g_tbr,i,t_freq[i] ,gpu_pc_freqs[i],COL_CLR);
        }
    }
    if (status->exceed < MIN_GPU_POWER_MARGIN){ 
        status->exceed = 0;
        status->ok = PC_STATUS_OK;
        return 0;
    }
    return 1;

}

