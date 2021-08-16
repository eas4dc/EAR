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
#include <limits.h>
#define _GNU_SOURCE
//#define SHOW_DEBUGS 1
#include <pthread.h>
#include <common/config.h>
#include <signal.h>
#include <common/colors.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/system/execute.h>
#include <metrics/energy/cpu.h>
#include <management/cpufreq/frequency.h>
#include <common/hardware/hardware_info.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_mgt.h>
#include <common/system/monitor.h>


#define RAPL_VS_NODE_POWER 1
#define RAPL_VS_NODE_POWER_limit 0.85
#define DEBUG_PERIOD 15

#define DVFS_PERIOD 0.5
#define DVFS_BURST_DURATION 1000
#define DVFS_RELAX_DURATION 1000

#define PSTATE_STEP 6
#define PSTATE0_STEP 25

static uint current_dvfs_pc=0;
static uint default_dvfs_pc=0;
static uint dvfs_pc_enabled=0;
static topology_t node_desc;

static uint c_status=PC_STATUS_IDLE;
static uint c_mode=PC_MODE_LIMIT;
static ulong *c_req_f;
static ulong num_pstates;
static suscription_t *sus_dvfs;

/* This  subscription will take care automatically of the power monitoring */
static uint dvfs_pc_secs=0, num_packs;
static  unsigned long long *values_rapl_init,*values_rapl_end,*values_diff;
static int *fd_rapl;
static float power_rapl,my_limit;
static uint *prev_pstate;
static ulong *c_freq;
static uint  *c_pstate,*t_pstate;
static uint node_size;
static uint dvfs_status = PC_STATUS_OK;
static uint dvfs_ask_def = 0;
static uint dvfs_monitor_initialized = 0;

/************************ These functions must be implemented in cpufreq *****/
static void frequency_nfreq_to_npstate(ulong *f,uint *p,uint cpus)
{
    int i;
    for (i=0;i<cpus;i++) p[i] = frequency_freq_to_pstate(f[i]);
}
static void frequency_npstate_to_nfreq(uint *p,ulong *f,uint cpus)
{
    int i;
    for (i=0;i<cpus;i++) f[i] = frequency_pstate_to_freq(p[i]);
}

/* Increases one pstate in f */
static void increase_one_pstate(uint *f)
{
    int i;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++){
        f[i] = ear_min(f[i]+1, num_pstates -1);
    }
}

/* Reduce 1 pstate in f, which a limit */
static void reduce_one_pstate(uint * f,uint *limit)
{
    int i;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++){
        if (f[i] > limit[i]) f[i]--;
    }
}

/* Returns true if some pstate in p1 is higher than p2 */
static int higher_pstate(uint *p1,uint *p2)
{
    int i;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++){
        if (p1[i] > p2[i]) return 1;
    }
    return 0;
}

int pstate_are_equal(uint *p1, uint *p2)
{
    int i;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++){
        if (p1[i] != p2[i]) return 0;
    }
    return 1;
}

/* P2 is the target, p1 can not be less than p2. Lower pstates means higher frequency */
static void limit_pstate(uint *p1,uint *p2)
{
    int i;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++){
        if (p1[i] < p2[i]) p1[i] = p2[i];	
    }
}

void vector_print_pstates(uint *pstates, uint num_cpus) {
    char *tmp_buff = calloc(num_cpus*32, sizeof(char));
    char sml_buff[32];
    int i;
    for (i = 0; i < num_cpus; i++) {
        sprintf(sml_buff, "p%d: %u, ", i, pstates[i]);
        strcat(tmp_buff, sml_buff);
    }
    debug(tmp_buff);
    free(tmp_buff);
}

/* Computes how many cpus can reduce one pstate with the given limit */
void num_cpus_to_reduce_pstates(uint *tmp,uint *target, uint *changes, uint *changes_max)
{
    int i, logical_pstates;
    *changes = 0;
    *changes_max = 0;
    ulong tmp_freq, next_freq;

    /* A logical pstate is a diff of 100000 MHz and is an average increase of 6 Watts
     * (except the first pstate). In some cases there is a jump in available frequencies
     * for a CPU of 2 logical p_states (and only 1 "real") so we use the logical steps
     * to compute the necessary power. */ 
    for (i = 0; i < node_size; i++){
        if (tmp[i] == 0) continue; //cannot reduce further
        tmp_freq = frequency_pstate_to_freq(tmp[i]);
        next_freq = frequency_pstate_to_freq(tmp[i] - 1);
        logical_pstates = (next_freq - tmp_freq) / 100000; 

        if (tmp[i] > target[i]){
            /* *changes is from pstate 3 --> 2 or lower, *changes_max is from 2 --> 1 or 1 --> 0 */
            if (tmp[i] < 2) *changes_max += 1;
            else *changes += logical_pstates;
        }
    }
}

//moved here from powercap_status.c as it is only used in dvfs cases
uint compute_extra_power(uint current_power, uint max_steps, uint *current, uint *target)
{
    uint total = 0, changes,changes_max;
    uint tmp[MAX_CPUS_SUPPORTED];
    //no pstate_change
    if (pstate_are_equal(current,target)) return 0;

    memcpy(tmp, current, sizeof(uint)*MAX_CPUS_SUPPORTED);
    do{
        num_cpus_to_reduce_pstates(tmp, target, &changes, &changes_max);
        if (changes || changes_max){
            total += (PSTATE_STEP * (float)changes/(float)node_size) + (PSTATE0_STEP * (float)changes_max/(float)node_size);
            reduce_one_pstate(tmp, target);
        }
        max_steps --;
        
    } while((changes || changes_max) && (max_steps > 0)); //if no more changes can be done or we pass the max number of steps
    return total;
}

int is_null_f(ulong *f)
{
    int i;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++){
        if (f[i]) return 0;
    }
    return 1;
}


/************************ This function is called by the monitor before the iterative part ************************/
state_t dvfs_pc_thread_init(void *p)
{

    debug("DVFS_monitor_init");


    num_packs=node_desc.socket_count;
    if (num_packs==0){
        error("Num packages cannot be detected in dvfs_pc thread initialization");
        pthread_exit(NULL);
    }
    debug("DVFS:Num pcks detected in dvfs_pc thread %u",num_packs);
    values_rapl_init=(unsigned long long*)calloc(num_packs*RAPL_POWER_EVS,sizeof(unsigned long long));
    values_rapl_end=(unsigned long long*)calloc(num_packs*RAPL_POWER_EVS,sizeof(unsigned long long));
    values_diff=(unsigned long long*)calloc(num_packs*RAPL_POWER_EVS,sizeof(unsigned long long));
    if ((values_rapl_init==NULL) || (values_rapl_end==NULL) || (values_diff==NULL))
    {
        error("values_rapl returns NULL in dvfs_pc thread initialization");
        pthread_exit(NULL);
    }
    fd_rapl=(int*)calloc(sizeof(int),num_packs);
    if (fd_rapl==NULL){
        error("fd_rapl cannot be allocated in dvfs_pc thread initialization");
        pthread_exit(NULL);
    }
    debug("DVFS:Initializing RAPL in dvfs_pc");
    if (init_rapl_msr(fd_rapl)<0){
        error("Error initializing rapl in dvfs_pc thread initialization");
        pthread_exit(NULL);
    }
    dvfs_pc_enabled=1;
    verbose(1,"Power measurement initialized in dvfs_pc thread initialization");
    /* RAPL is initialized */
    read_rapl_msr(fd_rapl,values_rapl_init);
    dvfs_monitor_initialized = 1;
    return EAR_SUCCESS;
}

/************************ This function is called by the monitor in iterative part ************************/
state_t dvfs_pc_thread_main(void *p)
{
    unsigned long long acum_energy;
    uint extra, tmp_pstate[MAX_CPUS_SUPPORTED];

    /* Update target frequency */
    pmgt_get_app_req_freq(DOMAIN_CPU, c_req_f, node_size);

    dvfs_pc_secs=(dvfs_pc_secs+1)%DEBUG_PERIOD;
    read_rapl_msr(fd_rapl,values_rapl_end);
    /* Calculate power */
    diff_rapl_msr_energy(values_diff,values_rapl_end,values_rapl_init);
    acum_energy=acum_rapl_energy(values_diff);
    power_rapl=(float)acum_energy/(DVFS_PERIOD*RAPL_MSR_UNITS);
    my_limit=(float)current_dvfs_pc;

    /* Copy init=end */
    memcpy(values_rapl_init, values_rapl_end, num_packs * RAPL_POWER_EVS * sizeof(unsigned long long));

    if ((current_dvfs_pc <= 0) || (c_status != PC_STATUS_RUN)){
        return EAR_SUCCESS;
    }
    if (power_rapl < 0.5) return EAR_ERROR;

    /* Apply limits */
    frequency_get_cpufreq_list(node_size,c_freq);
    frequency_nfreq_to_npstate(c_freq,c_pstate,node_size);
    frequency_nfreq_to_npstate(c_req_f,t_pstate,node_size);

    if (power_rapl > my_limit){ /* We are above the PC , reduce freq*/
        if (current_dvfs_pc < default_dvfs_pc) {
            dvfs_ask_def = 1;
        }
        /* We save in a temp variable the curr pstate and we increase 1 pstate (lower freq) */
        memcpy(tmp_pstate,c_pstate,sizeof(uint)*MAX_CPUS_SUPPORTED);
        increase_one_pstate(tmp_pstate);
        if ((power_rapl - my_limit) > 30) {            /* If the current power is above by a lot, we reduce 2 pstates instead of 1 */
            increase_one_pstate(tmp_pstate);
        }
        /* We limit anyway with the target */
        limit_pstate(tmp_pstate,t_pstate);
        /* If we are reducing the frequency (higher pstate) we copy in the prev_pstate */
        if (higher_pstate(tmp_pstate,c_pstate)){
            memcpy(prev_pstate,c_pstate,sizeof(uint)*MAX_CPUS_SUPPORTED);
        }
        /* Finally we set */
        memcpy(c_pstate,tmp_pstate,sizeof(uint)*MAX_CPUS_SUPPORTED);
        frequency_npstate_to_nfreq(c_pstate,c_freq,node_size);
        frequency_set_with_list(node_size,c_freq);
        debug("%spower above limit, setting freq0 %lu freqm %lu%s", COL_RED, c_freq[0], c_freq[node_size-1], COL_CLR);

    }else{ /* We are below the PC */
        if (is_null_f(c_req_f)) {
            //debug("current requested frequency is not set");
            return EAR_SUCCESS;
        }

        frequency_nfreq_to_npstate(c_req_f,t_pstate,node_size);
        if (higher_pstate(c_pstate,t_pstate)){
            
            extra = compute_extra_power(power_rapl, 1, c_pstate, t_pstate);
            if (((power_rapl+extra)<my_limit) && (c_mode==PC_MODE_TARGET)){ 
                if (higher_pstate(c_pstate, prev_pstate) && ((extra*2.0+power_rapl) > my_limit)) { //the second condition is to prevent drastic power decreases from being unrecoverable
                    //debug("Can't raise frequency without a powercap increase (would exceed power usage)");
                    return EAR_SUCCESS;
                }
                reduce_one_pstate(c_pstate,t_pstate);
                frequency_npstate_to_nfreq(c_pstate,c_freq,node_size);
                frequency_set_with_list(node_size,c_freq);
                debug("%spower below limit, setting freq0 %lu freqm %lu %s", COL_GRE, c_freq[0], c_freq[node_size-1], COL_CLR);
            }
        }else if (higher_pstate(t_pstate,c_pstate)){
            frequency_npstate_to_nfreq(t_pstate,c_freq,node_size);
            frequency_set_with_list(node_size,c_freq);
            debug("%spower below limit, and freq above req_f (%lu) setting freq %lu%s", COL_GRE, c_req_f[0], c_freq[0], COL_CLR);
        }
    }
    return EAR_SUCCESS;
}


state_t disable()
{
    return EAR_SUCCESS;
}

state_t enable(suscription_t *sus)
{
    state_t s;
    debug("DVFS");

    if (sus == NULL){
        debug("NULL subscription in DVFS powercap");
        return EAR_ERROR;
    }
    sus->call_main = dvfs_pc_thread_main;
    sus->call_init = dvfs_pc_thread_init;
    sus->time_relax = DVFS_RELAX_DURATION*DVFS_PERIOD;
    sus->time_burst = DVFS_BURST_DURATION*DVFS_PERIOD;
    sus_dvfs = sus;
    debug("DVFS Subscription done");
    /* Init data */
    s = topology_init(&node_desc);
    if (s == EAR_ERROR) debug("Error getting node topology");

    node_size = node_desc.cpu_count;
    debug("DVFS:Initializing frequency in dvfs_pc %u cpus",node_size);
    frequency_init(node_size);
    num_pstates = frequency_get_num_pstates();
    /* node_size or MAX_CPUS_SUPPORTED */
    c_freq = calloc(MAX_CPUS_SUPPORTED,sizeof(ulong));
    c_req_f = calloc(MAX_CPUS_SUPPORTED,sizeof(ulong));
    c_pstate = calloc(MAX_CPUS_SUPPORTED,sizeof(uint));
    t_pstate = calloc(MAX_CPUS_SUPPORTED,sizeof(uint));
    prev_pstate = calloc(MAX_CPUS_SUPPORTED,sizeof(uint));
    int i;
    for (i = 0; i < MAX_CPUS_SUPPORTED; i++) prev_pstate[i] = UINT_MAX;

    sus->suscribe(sus);
    return EAR_SUCCESS;
}

state_t set_powercap_value(uint pid,uint domain,uint limit,uint *cpu_util)
{
    int i;
    /* Set data */
    debug("%sDVFS:set_powercap_value %u%s",COL_BLU,limit,COL_CLR);
    default_dvfs_pc = limit;
    current_dvfs_pc = default_dvfs_pc;
    dvfs_status = PC_STATUS_OK;
    dvfs_ask_def = 0;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++) prev_pstate[i] = UINT_MAX; //reset flipflop measures
    return EAR_SUCCESS;
}

state_t reset_powercap_value() 
{
    int i;
    debug("Reset powercap value, changing from %u to %u", current_dvfs_pc, default_dvfs_pc);
    current_dvfs_pc = default_dvfs_pc;
    dvfs_status = PC_STATUS_OK;
    dvfs_ask_def = 0;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++) prev_pstate[i] = UINT_MAX; //reset flipflop measures
    return EAR_SUCCESS;
}

state_t increase_powercap_allocation(uint increase) 
{
    int i;
    debug("Increasing powercap allocation by %u", increase);
    current_dvfs_pc += increase;
    dvfs_ask_def = 0;
    if (current_dvfs_pc >= default_dvfs_pc) {
        dvfs_status = PC_STATUS_OK;
    }
    for (i=0;i<MAX_CPUS_SUPPORTED;i++) prev_pstate[i] = UINT_MAX; //reset flipflop measures
    return EAR_SUCCESS;
}

state_t release_powercap_allocation(uint decrease)
{
    int i;
    debug("Decreasing powercap allocation by %u", decrease);
    current_dvfs_pc -= decrease;
    if (current_dvfs_pc < default_dvfs_pc) dvfs_status = PC_STATUS_RELEASE;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++) prev_pstate[i] = UINT_MAX; //reset flipflop measures
    return EAR_SUCCESS;
}

state_t get_powercap_value(uint pid,ulong *powercap)
{
    /* copy data */
    //debug("DVFS:get_powercap_value");
    *powercap=current_dvfs_pc;
    return EAR_SUCCESS;
}

uint is_powercap_policy_enabled(uint pid)
{
    return dvfs_pc_enabled;
}

void print_powercap_value(int fd)
{
    dprintf(fd,"dvfs_powercap_value %u\n",current_dvfs_pc);
}

void powercap_to_str(char *b)
{
    sprintf(b,"%u",current_dvfs_pc);
}

void set_status(uint status)
{
    debug("DVFS:set_status %u",status);
    c_status=status;
}
uint get_powercap_strategy()
{
    debug("DVFS:get_powercap_strategy");
    return PC_DVFS;
}

void set_pc_mode(uint mode)
{
    debug("DVFS:set_pc_mode");
    c_mode=mode;
}


void set_app_req_freq(ulong *f)
{
    debug("DVFS:Requested application freq[0] set to %lu",f[0]);
    memcpy(c_req_f, f, sizeof(ulong)*MAX_CPUS_SUPPORTED);    
}

void set_verb_channel(int fd)
{
    WARN_SET_FD(fd);
    VERB_SET_FD(fd);
    DEBUG_SET_FD(fd);
}


#define MIN_CPU_POWER_MARGIN 10


/* Returns 0 when 1) No info, 2) No limit 3) We cannot share power. 
 *
 * state_t release_powercap_allocation(
 * Returns 1 when power can be shared with other modules 
 * Status can be: PC_STATUS_OK, PC_STATUS_GREEDY, PC_STATUS_RELEASE, PC_STATUS_ASK_DEF 
 * tbr is > 0 when PC_STATUS_RELEASE , 0 otherwise 
 * X_status is the plugin status 
 * X_ask_def means we must ask for our power  */

uint get_powercap_status(domain_status_t *status)
{
    uint ctbr;
    uint t_pstate_pc[MAX_CPUS_SUPPORTED], c_pstate_pc[MAX_CPUS_SUPPORTED];
    memset(c_pstate_pc, 0, MAX_CPUS_SUPPORTED*sizeof(uint));
    memset(t_pstate_pc, 0, MAX_CPUS_SUPPORTED*sizeof(uint));
    status->ok = PC_STATUS_OK;
    status->exceed = 0;
    status->stress = 0;
    status->requested = 0;
    status->current_pc = current_dvfs_pc;

    //debug("DVFS: get_powercap_status");
    if (!dvfs_monitor_initialized) return 0;
    /* Return 0 means we cannot release power */
    if (current_dvfs_pc == PC_UNLIMITED) return 0;
    /* If we don't know the req_f we cannot release power */

    if (is_null_f(c_req_f)) { 
        if (c_status != PC_STATUS_IDLE) debug("unknown req_f"); 
        return 0; 
    }

    frequency_get_cpufreq_list(node_size,c_freq);
    status->stress = powercap_get_stress(frequency_pstate_to_freq(0), frequency_pstate_to_freq(num_pstates-1), c_req_f[0], c_freq[0]); 
    frequency_nfreq_to_npstate(c_freq, c_pstate_pc, node_size);
    frequency_nfreq_to_npstate(c_req_f, t_pstate_pc, node_size);
    debug("DVFS: current stress%d", status->stress);
    if (higher_pstate(c_pstate_pc, t_pstate_pc)){
        if (dvfs_ask_def && current_dvfs_pc < default_dvfs_pc) status->ok = PC_STATUS_ASK_DEF;
        else{ 
            status->requested = compute_extra_power(power_rapl, num_pstates, c_pstate_pc, t_pstate_pc);
            debug("DVFS: Requested power %u",status->requested);
            status->ok = PC_STATUS_GREEDY;
        }
        debug("DVFS status %u", status->ok);
        return 0; /* Pending for NJObs */
    }
    ctbr = (my_limit - power_rapl) * 0.5;
    status->ok = PC_STATUS_RELEASE;
    status->exceed = ctbr; 
    if (ctbr < MIN_CPU_POWER_MARGIN){
        status->exceed = 0;
        status->ok = PC_STATUS_OK;
        debug("DVFS status %u", status->ok);
        return 0;
    }
    dvfs_status = status->ok;
    debug("DVFS status %u", status->ok);
    return 1;
}
