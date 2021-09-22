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

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
//#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/colors.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/system/monitor.h>

#include <daemon/powercap/powercap_status_conf.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_mgt.h>

#include <metrics/gpu/gpu.h>
#include <management/gpu/gpu.h>


#define DEBUG_PERIOD 15
#define MIN_GPU_IDLE_POWER 30
#define UTIL_INC    10
#define UTIL_WAIT   50

static uint gpu_idle_power = MIN_GPU_IDLE_POWER;

static uint gpu_dvfs_pc_enabled=0;
//static ulong c_req_f;

/* This  subscription will take care automatically of the power monitoring */
static ulong current_gpu_pc=0;
static ulong default_gpu_pc=0;
static uint gpu_pc_enabled=0;
static uint c_status=PC_STATUS_IDLE;
static uint c_mode=PC_MODE_LIMIT;

static gpu_t *values_gpu_init,*values_gpu_end,*values_gpu_diff,*values_gpu_idle;

static ctx_t gpu_pc_ctx,gpu_metric_ctx;
static uint gpu_pc_num_gpus;
static ulong *gpu_pc_min_power;
static ulong *gpu_pc_max_power;
/* These two variables controls the power ditribution between GPUS */
static ulong *gpu_pc_curr_power;
static ulong *gpu_pc_util;
/* util wait to control when a GPU has been idle and a previous freq to restore it when it starts again */
static ulong *gpu_pc_util_wait;
static ulong *gpu_pc_prev_freq;

static long  *gpu_pc_excess;
static float *pdist;
static ulong *c_freq,*t_freq,*n_freq;
static const ulong **gpu_freq_list=NULL;
static const uint *gpu_num_freqs=NULL;
static gpu_ops_t     *ops_met;
static uint gpu_dvfs_status = PC_STATUS_OK;
static uint gpu_dvfs_ask_def = 0;
static uint gpu_dvfs_monitor_initialized = 0;
static uint gpu_dvfs_util = 0;

static state_t int_set_powercap_value(ulong limit,ulong *gpu_util);

static char gpu_dvfs_greedy = 0;

static int gpu_freq_to_pstate(int gpuid,ulong f)
{
    int i = 0;
    while(i < gpu_num_freqs[gpuid]-1){
        if (gpu_freq_list[gpuid][i] == f) return i;
        i++;
    }	
    return gpu_num_freqs[gpuid]-1;
}
ulong select_lower_gpu_freq(uint i, ulong f, uint pstate_steps)
{
    int j=0,found=0;
    /* If we are the minimum*/
    // debug("Looking for lower freq for %lu",f);
    if (f == gpu_freq_list[i][gpu_num_freqs[i]-1]) return f;
    /* Otherwise, look for f*/
    do{
        if (gpu_freq_list[i][j] == f) found = 1;
        else j++;
    }while((found==0) && (j!=gpu_num_freqs[i]));
    if (!found){
        // debug("Frequency %lu not found in GPU %u",f,i);
        return gpu_freq_list[i][gpu_num_freqs[i]-1];
    }else{
        // debug("Frequency %lu found in position %d",f,j);
    }
    if (j < (gpu_num_freqs[i]-1)){
        /* When reducing GPU frequency we do it in steps of N instead of 1
         * since the frequencies list for GPUs is very fine-grained and one
         * step does not reduce the total power consumption by much*/
        return gpu_freq_list[i][ear_min(j+pstate_steps, gpu_num_freqs[i]-1)];
    }
    return f;
}

ulong select_higher_gpu_freq(uint i, ulong f, int extra_steps)
{
    int j=0,found=0;
    // debug("Looking for higher freq for %lu",f);
    /* If we are the maximum*/
    if (f == gpu_freq_list[i][0]) return f;
    /* Otherwise, look for f*/
    do{
        if (gpu_freq_list[i][j] == f) found = 1;
        else j++;
    }while((found==0) && (j!=gpu_num_freqs[i]));
    if (!found){
        debug("Frequency %lu not found in GPU %u",f,i);
        return gpu_freq_list[i][0];
    }else{
        debug("Frequency %lu found in position %d returning %d",f,j, ear_max(j-extra_steps, 0));
    }
    return gpu_freq_list[i][ear_max(j-extra_steps, 0)];
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
    if (gpu_load(&ops_met,0,NULL)!=EAR_SUCCESS){
        debug("Error at gpu load");
        gpu_dvfs_pc_enabled = 0;
        return EAR_ERROR;
    }
    if (gpu_init(&gpu_metric_ctx) != EAR_SUCCESS){
        debug("Error at gpu initialization");
        gpu_dvfs_pc_enabled = 0;
        return EAR_ERROR;
    }
    gpu_data_alloc(&values_gpu_init);
    gpu_data_alloc(&values_gpu_end);
    gpu_data_alloc(&values_gpu_diff);
    mgt_gpu_freq_list(&gpu_pc_ctx, &gpu_freq_list, &gpu_num_freqs);
    // printf_gpu_freq_list(gpu_freq_list,gpu_num_freqs);
    gpu_dvfs_pc_enabled=1;
    debug("Power measurement initialized in gpu_dvfs_pc thread initialization");
    if (gpu_read(&gpu_metric_ctx,values_gpu_init)!= EAR_SUCCESS){
        debug("Error in gpu_read in gpu_dvfs_pc");
        gpu_dvfs_pc_enabled=0;
    }
    gpu_dvfs_monitor_initialized = 1;
    return EAR_SUCCESS;
}

/************************ This function is called by the monitor in iterative part ************************/
state_t gpu_dvfs_pc_thread_main(void *p)
{
    int extra;
    uint i, j;
    uint gpu_dvfs_local_util = 0;

    if (!gpu_dvfs_pc_enabled) return EAR_SUCCESS;
    if (gpu_read(&gpu_metric_ctx,values_gpu_end)!=EAR_SUCCESS){
        debug("Error in gpu_read gpu_dvfs_pc");
        return EAR_ERROR;
    }

    pmgt_get_app_req_freq(DOMAIN_GPU, t_freq, gpu_pc_num_gpus);
    /* Calcular power */
    gpu_data_diff(values_gpu_end,values_gpu_init,values_gpu_diff);
    if (values_gpu_diff[0].power_w == 0 ) return EAR_SUCCESS;
    mgt_gpu_freq_limit_get_current(&gpu_pc_ctx,c_freq);
    /* Copiar init=end */
    gpu_data_copy(values_gpu_init,values_gpu_end);
    /* If utilisation has changed we internally redistribute powercap */
    for (i=0;i<gpu_pc_num_gpus;i++){
        gpu_dvfs_local_util += values_gpu_diff[i].util_gpu;
        gpu_pc_util[i] = values_gpu_diff[i].util_gpu;
        gpu_pc_excess[i] = (long)gpu_pc_curr_power[i] - (long)values_gpu_diff[i].power_w;
    }
    if (util_changed(gpu_dvfs_local_util,gpu_dvfs_util)){
        int_set_powercap_value(current_gpu_pc,gpu_pc_util);
    }

    gpu_dvfs_greedy = 0;
    if ((current_gpu_pc <= 0) || (c_status != PC_STATUS_RUN)){
        return EAR_SUCCESS;
    }

    //redistribute power between gpus
    for (i = 0; i < gpu_pc_num_gpus; i++)
    {
        //if (gpu_pc_util[i] == 0) continue; //idle nodes need no reallocation
        if (gpu_pc_excess[i] > 0) continue; //nodes with extra power need no reallocation
        for (j = 0; j < gpu_pc_num_gpus; j++) {
            if (gpu_pc_util[j] == 0) continue; //we don't want to remove power from idle nodes
            /* We leave a minimum of 5 Watts of excess power in case of power fluctuations. */
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

    for (i=0;i<gpu_pc_num_gpus;i++){
        if (t_freq[i] == 0) t_freq[i] = gpu_freq_list[i][0]; //set the nominal if no GPU freq was specified
        if (gpu_pc_util[i] == 0) 
        {
            gpu_pc_util_wait[i] += UTIL_INC; 
            /* if the GPU has been unused  for UTIL_WAIT we set its freq to the minimum */
            if (gpu_pc_util_wait[i] % UTIL_WAIT == 0)
            {
                gpu_pc_prev_freq[i] = ear_max(c_freq[i], gpu_pc_prev_freq[i]);
                n_freq[i] = gpu_freq_list[i][gpu_num_freqs[i]-1];
                debug("util at 0, setting gpu-freq to minimum");
            }
            debug("util at 0, increasing util_wait (current %lu, needed %u)", gpu_pc_util_wait[i], UTIL_WAIT);
            continue;
        }
        /* if the frequency had been set to the minimum (due to util), reset it to the previous one */
        if (gpu_pc_prev_freq[i] > 0) {
            c_freq[i] = gpu_pc_prev_freq[i];
            gpu_pc_prev_freq[i] = 0;
        }
        if (t_freq[i] > 0) c_freq[i] = ear_min(c_freq[i], t_freq[i]);
        n_freq[i] = c_freq[i];
        /* Aplicar limites */
        //debug("GPU[%d] power %lf limit %lu",i,values_gpu_diff[i].power_w,gpu_pc_curr_power[i]);
        if (values_gpu_diff[i].power_w > gpu_pc_curr_power[i]){ /* We are above the PC */
            if (gpu_dvfs_status == PC_STATUS_RELEASE){
                debug("gpu_dvfs setting ask_default=1");
                gpu_dvfs_ask_def = 1;
            }
            /* Number of steps is estimated by watching how far are we from the cap.
             * The further from the cap, the more pstates we reduce. Minimum 1 pstate */
            uint pstate_steps = ear_max((uint)(values_gpu_diff[i].power_w - gpu_pc_curr_power[i]) - 2, 1);
            /* Use the list of frequencies */
            n_freq[i] = select_lower_gpu_freq(i, c_freq[i], pstate_steps);
            gpu_dvfs_greedy |= 1;
            debug("%sReducing the GPU-freq[%d] from %lu to %lu (cpower %lf limit %lu) %s",
                    COL_RED,i,c_freq[i],n_freq[i],values_gpu_diff[i].power_w,gpu_pc_curr_power[i],COL_CLR);
        }else{ /* We are below the PC */
            if ((c_freq[i] < t_freq[i]) && (t_freq[i] > 0)){

                //extra = compute_extra_gpu_power(c_freq[i], values_gpu_diff[i].power_w, t_freq[i]);

                extra = (gpu_pc_curr_power[i] - values_gpu_diff[i].power_w);
                extra = extra > 5 ? extra - 2 : 1;
                n_freq[i] = ear_min(select_higher_gpu_freq(i, c_freq[i], extra), t_freq[i]);

                //n_freq[i] = ear_min(c_freq[ear_max(i-extra, 0)], t_freq[i]);
                debug("%sIncreasing the GPU-freq[%d] from %lu to %lu, estimated %lf=(%lf+%d), limit %lu %s", 
                        COL_GRE,i,c_freq[i],n_freq[i],values_gpu_diff[i].power_w+(ulong)extra,values_gpu_diff[i].power_w,extra,gpu_pc_curr_power[i],COL_CLR);

                //we can't reach target frequency, check if ask_default
                if (gpu_dvfs_status == PC_STATUS_RELEASE && (t_freq[i] > 0) && (n_freq[i] < t_freq[i])){
                    gpu_dvfs_ask_def = 1;
                }
            }
        }
    } /* end for loop */

    mgt_gpu_freq_limit_set(&gpu_metric_ctx,n_freq);

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
    state_t ret=EAR_SUCCESS;
    debug("GPU-DVFS");
    /* Suscription is for gpu power monitoring */
    if (sus == NULL){
        debug("NULL subscription in GPU-DVFS powercap");
        return EAR_ERROR;
    }
    sus->call_main = gpu_dvfs_pc_thread_main;
    sus->call_init = gpu_dvfs_pc_thread_init;
    sus->time_relax = 1000;
    sus->time_burst = 1000;
    /* Init data */
    debug("GPU_DVFS: power cap  enable");
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
        gpu_pc_excess = calloc(gpu_pc_num_gpus,sizeof(long));
        if (gpu_pc_excess == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_util = calloc(gpu_pc_num_gpus,sizeof(ulong));
        if (gpu_pc_util == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_util_wait = calloc(gpu_pc_num_gpus,sizeof(ulong));
        if (gpu_pc_util_wait == NULL){
            gpu_pc_enabled = 0;
            ret = EAR_ERROR;
            debug("Error allocating memory in GPU enable");
        }
        gpu_pc_prev_freq = calloc(gpu_pc_num_gpus,sizeof(ulong));
        if (gpu_pc_prev_freq == NULL){
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
        c_freq=calloc(gpu_pc_num_gpus,sizeof(ulong));
        t_freq=calloc(gpu_pc_num_gpus,sizeof(ulong));
        n_freq=calloc(gpu_pc_num_gpus,sizeof(ulong));
        mgt_gpu_power_cap_get_rank(&gpu_pc_ctx,gpu_pc_min_power,gpu_pc_max_power);
        gpu_data_alloc(&values_gpu_idle);
    }
    sus->suscribe(sus);
    debug("GPU-DVFS Subscription done");
    return ret;

}

state_t set_powercap_value(uint pid,uint domain,ulong limit,ulong *gpu_util)
{
    gpu_dvfs_status = PC_STATUS_OK;
    gpu_dvfs_ask_def = 0;
    default_gpu_pc = limit;
    return int_set_powercap_value(default_gpu_pc,gpu_util);
}

state_t reset_powercap_value()
{
    debug("Reset powercap value to %lu", default_gpu_pc);
    gpu_dvfs_status = PC_STATUS_OK;
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
    if (current_gpu_pc >= default_gpu_pc) gpu_dvfs_status = PC_STATUS_OK;
    return int_set_powercap_value(current_gpu_pc, gpu_pc_util);
}

static state_t int_set_powercap_value(ulong limit,ulong *gpu_util)
{
    int i;
    float alloc;
    ulong ualloc;
    uint gpu_run=0,total_util=0;
    /* Set data */
    current_gpu_pc = limit;
    debug("%sGPU-DVFS:set_powercap_value %lu%s",COL_BLU,limit,COL_CLR);

    if (gpu_read_raw(&gpu_metric_ctx,values_gpu_idle)!=EAR_SUCCESS){
        debug("Error in gpu_read_raw gpu_dvfs_pc (int_set_powercap)");
        return EAR_ERROR;
    }
    //debug("%s",COL_BLU);
    //debug("GPU: set_powercap_value %lu",limit);
    //debug("GPU: Phase 1, allocating power to idle GPUS");
    for (i=0;i<gpu_pc_num_gpus;i++){    
        pdist[i]=0.0;
        total_util+=gpu_util[i];
        if (gpu_util[i]==0){ 
            gpu_pc_curr_power[i] = values_gpu_idle[i].power_w + 5;
            gpu_idle_power = values_gpu_idle[i].power_w + 5;
            limit = limit - gpu_pc_curr_power[i];
        }else{
            gpu_run++;
        }
    }
    gpu_dvfs_util = total_util;
    //debug("GPU: Phase 2: Allocating power to %u running GPUS",gpu_run);
    for (i=0;i<gpu_pc_num_gpus;i++){
        if (gpu_util[i]>0){
            pdist[i]=(float)gpu_util[i]/(float)total_util;
            alloc=(float)limit*pdist[i];
            ualloc = (ulong)alloc;
            if (ualloc > gpu_pc_max_power[i]) ualloc = gpu_pc_max_power[i];
            gpu_pc_curr_power[i] = ualloc;
        }
    }
    for (i=0;i<gpu_pc_num_gpus;i++) {
        //debug("GPU: util_gpu[%d]=%lu power_alloc=%lu",i,gpu_util[i],gpu_pc_curr_power[i]);
    }
    memcpy(gpu_pc_util,gpu_util,sizeof(ulong)*gpu_pc_num_gpus);
    //debug("%s",COL_CLR);
    return EAR_SUCCESS;
}

state_t get_powercap_value(uint pid,ulong *powercap)
{
    /* copy data */
    //debug("GPU::get_powercap_value");
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
    char cmd[128];
    int i=0;
    cmd[0]='\0';
    for (i=0;i<gpu_pc_num_gpus;i++){
        sprintf(cmd,"GPU_powercap[%d]=%lu ",i,current_gpu_pc);
        strcat(b,cmd);
    }
}

void set_status(uint status)
{
    debug("GPU. set_status %u",status);
    c_status=status;
}
uint get_powercap_strategy()
{
    debug("GPU. get_powercap_strategy");
    return PC_DVFS;
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

    // debug("GPU_DVFS: get_powercap_status");
    if (!gpu_dvfs_monitor_initialized) return 0;
    if (current_gpu_pc == PC_UNLIMITED){
        return 0;
    }
    /* We need the released power */
    if (gpu_dvfs_ask_def){
        debug("GPU status ASK_DEF, current_pc %lu and def_pc %lu", current_gpu_pc, default_gpu_pc);
        status->ok = PC_STATUS_ASK_DEF;
        return 0;
    }

    /*for (i=0;i<gpu_pc_num_gpus;i++){
      debug("total util is 0, GPU %d util %lu", i, gpu_pc_util[i]);
      }*/
    /* If we are not using th GPU we can release all the power */
    for (i=0;i<gpu_pc_num_gpus;i++){
        used += (gpu_pc_util[i]>0);
        if (t_freq[i] == 0) return 0;
    }
    if (!used){
        status->ok = PC_STATUS_RELEASE;
        if (gpu_pc_num_gpus == 0) status->exceed = current_gpu_pc;
        else status->exceed=(current_gpu_pc - (gpu_idle_power*gpu_pc_num_gpus));
        if (status->exceed >= current_gpu_pc) status->exceed = 0;
        debug("%sCould release %u Watts from the GPU%s",COL_GRE,status->exceed,COL_CLR);
        return 1;
    }
    /* If we know, we must check */
    status->ok = PC_STATUS_RELEASE;

    /* Compute the current stress before checking the status */
    for (i=0;i<gpu_pc_num_gpus;i++){
        if (gpu_pc_util[i] > 0)
            tmp_stress += powercap_get_stress(gpu_freq_list[i][0], gpu_freq_list[i][gpu_num_freqs[i]-1], t_freq[i], c_freq[i]);
    }
    tmp_stress /= used;
    status->stress = tmp_stress;

    for (i=0;i<gpu_pc_num_gpus;i++){
        if ((c_freq[i] < t_freq[i]) && (gpu_pc_util[i]>0) && gpu_dvfs_greedy){ 
            debug("We cannot release power from GPU %d",i);
            status->ok = PC_STATUS_GREEDY;
            status->exceed = 0;
            /* For NVIDIA GPUS we have estimated one extra Watt per gpu p_state */
            status->requested += gpu_freq_to_pstate(i,t_freq[i]) - gpu_freq_to_pstate(i,c_freq[i]);
            debug("GPU_DVFS status GREEDY");
            return 0;
        }else if ((c_freq[i] >= t_freq[i]) && (gpu_pc_util[i]>0)){
            if (values_gpu_diff[i].power_w > gpu_pc_curr_power[i]) // if we are at req_f but we are using more power than we should
            {
                status->ok = PC_STATUS_GREEDY;
                /* FOr NVIDIA GPUS we have estimated one extra Watt per gpu p_state */
                status->requested += gpu_freq_to_pstate(i,t_freq[i]) - gpu_freq_to_pstate(i,c_freq[i]);
                status->exceed = 0;
                debug("GPU_DVFS status greedy because we need more power to maintain current speed");
                return 0;
            }
            g_tbr = (uint)(((double)gpu_pc_curr_power[i] - values_gpu_diff[i].power_w) * 0.5);
            status->exceed = status->exceed + g_tbr;
            debug("gpu_pc_curr_power %lu and current_power %lf, exceed: %u", gpu_pc_curr_power[i], values_gpu_diff[i].power_w, status->exceed);
            debug("%sWe can release %u W from GPU %d since target = %lu current %lu%s",COL_GRE,g_tbr,i,t_freq[i] ,c_freq[i],COL_CLR);
        }
    }
    if (status->exceed < MIN_GPU_POWER_MARGIN){ 
        status->exceed = 0;
        status->ok = PC_STATUS_OK;
        debug("GPU_DVFS status OK");
        return 0;
    }

    return 1;
}
