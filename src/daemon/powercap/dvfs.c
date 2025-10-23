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
//#define SHOW_DEBUGS 1

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>

#include <common/config.h>
#include <common/colors.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/system/execute.h>
#include <common/system/monitor.h>
#include <common/hardware/hardware_info.h>

#include <management/cpufreq/frequency.h>

#include <metrics/energy/cpu.h>
#include <metrics/energy_cpu/energy_cpu.h>

#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_status_conf.h>


#define RAPL_VS_NODE_POWER 1
#define RAPL_VS_NODE_POWER_limit 0.85
#define DEBUG_PERIOD 15

#define DVFS_PERIOD 0.5
#define DVFS_BURST_DURATION 1000
#define DVFS_RELAX_DURATION 1000
//#define DVFS_RELAX_DURATION 60000

#define MAX_DVFS_TRIES 3

#define PSTATE_STEP 8
#define PSTATE0_STEP 30 

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
static float power_rapl,my_limit;
static uint *prev_pstate;
static int32_t prev_counter;
static ulong *c_freq;
static uint  *c_pstate,*t_pstate;
static uint node_size;
static uint dvfs_status = PC_STATUS_OK;
static uint dvfs_ask_def = 0;
static uint dvfs_monitor_initialized = 0;
static timestamp_t last_dvfs_time;

static domain_settings_t settings = { .node_ratio = 0.1, .security_range= 0.05};

extern ulong pmgt_idle_def_freq;

/************************ These functions must be implemented in cpufreq *****/


static void frequency_nfreq_to_npstate(ulong *f,uint *p,uint cpus)
{
    int i;
    for (i=0;i<cpus;i++) {
        if (f[i]) p[i] = frequency_freq_to_pstate(f[i]);
        //else p[i] = num_pstates - 1;
        else p[i] = 0; //modified to be more conservative towards user performance
    }
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
            if (tmp[i] <= 2) *changes_max += 1;
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
		//vector_print_pstates(current, node_size);
		//vector_print_pstates(target, node_size);

    memcpy(tmp, current, sizeof(uint)*MAX_CPUS_SUPPORTED);
    do{
        num_cpus_to_reduce_pstates(tmp, target, &changes, &changes_max);
        if (changes || changes_max){
            total += (PSTATE_STEP * (float)changes/(float)node_size) + (PSTATE0_STEP * (float)changes_max/(float)node_size);
						//debug("Total power estimated %u: changes (less than nominal) %u - %f changes (to nominal) %d - %f", total, changes, (float)changes/(float)node_size, changes_max, (float)changes_max/(float)node_size);
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

	// Init energy_cpu and alloc data
	if (energy_cpu_init(NULL)) {
		error("cannot initialize energy_cpu layer in dvfs_pc thread initialization");
		pthread_exit(NULL);
	}
	energy_cpu_data_alloc(NULL, &values_rapl_init, &num_packs);
	energy_cpu_data_alloc(NULL, &values_rapl_end,  &num_packs);
	energy_cpu_data_alloc(NULL, &values_diff, &num_packs);
    if ((values_rapl_init==NULL) || (values_rapl_end==NULL) || (values_diff==NULL))
    {
        error("values_rapl returns NULL in dvfs_pc thread initialization");
        pthread_exit(NULL);
    }

    // set the governor to userspace just when needed

    verbose(VEARD_PC, "Setting governor to userspace for DVFS powercap control");
		state_t ret_st = frequency_set_userspace_governor_all_cpus();
    check_usrspace_gov_set(ret_st, VEARD_PC);

	// read initial values
	energy_cpu_read(NULL, values_rapl_init);
    dvfs_monitor_initialized = 1;
    return EAR_SUCCESS;
}

/************************ This function is called by the monitor in iterative part ************************/
state_t dvfs_pc_thread_main(void *p)
{
    //debug("entering dvfs_pc_thread_main, current_dvfs_pc %u and c_status %u", current_dvfs_pc, c_status);
    uint extra, tmp_pstate[MAX_CPUS_SUPPORTED];
    timestamp_t curr_time;
    ulong elapsed;
    debug("--------------------------------------")
    debug("new_dvfs");
    debug("--------------------------------------")


    /* Get elapsed time since last call */
    timestamp_get(&curr_time);
    elapsed = timestamp_diff(&curr_time,  &last_dvfs_time, TIME_MSECS);
    last_dvfs_time = curr_time;

    /* Update target frequency */
    memset(c_pstate, 0, sizeof(uint)*MAX_CPUS_SUPPORTED);
    memset(t_pstate, 0, sizeof(uint)*MAX_CPUS_SUPPORTED);
    memset(c_req_f, 0, sizeof(ulong)*MAX_CPUS_SUPPORTED);
    pmgt_get_app_req_freq(DOMAIN_CPU, c_req_f, node_size);

    if (current_dvfs_pc == POWER_CAP_UNLIMITED || current_dvfs_pc == 0){
        debug("powercap unlimited or disabled");
        return EAR_SUCCESS;
    }   

    dvfs_pc_secs=(dvfs_pc_secs+1)%DEBUG_PERIOD;
	energy_cpu_read(NULL, values_rapl_end);
    /* Calculate power */
	energy_cpu_data_diff(NULL, values_rapl_init, values_rapl_end, values_diff);

    /* Copy init=end */
	energy_cpu_data_copy(NULL, values_rapl_init, values_rapl_end);

    /* Changed from acum_rapl to use the new (and proper) energy_cpu API */
    power_rapl = 0;
    for (int32_t i = 0; i < node_desc.socket_count; i++) {
        //DRAM power
        power_rapl += energy_cpu_compute_power(values_diff[i], elapsed/1000.0);
        //CPU power
        power_rapl += energy_cpu_compute_power(values_diff[node_desc.socket_count + i], elapsed/1000.0);
    }

    my_limit = (float)current_dvfs_pc;
    debug("Current power %f current limit %f (%u cpus)", power_rapl, my_limit,node_size);

    if (power_rapl < 0.5) return EAR_SUCCESS;

    /* Apply limits */
    frequency_get_cpufreq_list(node_size,c_freq);
    frequency_nfreq_to_npstate(c_freq,c_pstate,node_size);
    frequency_nfreq_to_npstate(c_req_f,t_pstate,node_size);

    verbose_frequencies(node_size, c_freq);

    if (power_rapl > my_limit){ /* We are above the PC , reduce freq*/
        verbose(VEARD_PC+1,"%sCurrent freq freq0 %lu freqn %lu%s", COL_RED, c_freq[0], c_freq[node_size-1],COL_CLR);
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
            if (!pstate_are_equal(prev_pstate, c_pstate)) {
                prev_counter = 0;
            }
            memcpy(prev_pstate,c_pstate,sizeof(uint)*MAX_CPUS_SUPPORTED);
        }
        /* Finally we set */
        memcpy(c_pstate,tmp_pstate,sizeof(uint)*MAX_CPUS_SUPPORTED);
        //vector_print_pstates(c_pstate, node_size);
        frequency_npstate_to_nfreq(c_pstate,c_freq,node_size);
        //print_frequencies(node_size, c_freq);
        frequency_set_with_list(node_size,c_freq);
        
        verbose(VEARD_PC+1,"%spower above limit. Curr %.2f Limit %.2f, setting freq0 %lu freqm %lu%s", COL_RED, power_rapl, my_limit, c_freq[0], c_freq[node_size-1], COL_CLR);

    }else{ /* We are below the PC */
        frequency_nfreq_to_npstate(c_req_f,t_pstate,node_size);
        if (higher_pstate(c_pstate,t_pstate)){

            extra = compute_extra_power(power_rapl, 1, c_pstate, t_pstate);
            if (((power_rapl+extra)<my_limit) && (c_mode==PC_MODE_TARGET)){ 
                uint ish = higher_pstate(c_pstate, prev_pstate);
                if (ish) {
                    //vector_print_pstates(prev_pstate, node_size);
                    if (((extra*2.0+power_rapl) > my_limit) && (prev_counter >= MAX_DVFS_TRIES)) { //the second condition is to prevent drastic power decreases from being unrecoverable
                        debug("Can't raise frequency without a powercap increase (would exceed power usage), number of tries: %d", prev_counter);
                        return EAR_SUCCESS;
                    }
                    prev_counter++;
                    debug("increasing prev counter to %d", prev_counter);
                }

                reduce_one_pstate(c_pstate,t_pstate);
                frequency_npstate_to_nfreq(c_pstate,c_freq,node_size);
                frequency_set_with_list(node_size,c_freq);
                debug("%spower below limit, setting freq0 %lu freqm %lu (ish %u, extra %u)%s", COL_GRE, c_freq[0], c_freq[node_size-1], ish, extra, COL_CLR);
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

	/* Load energy cpu plugin */
	if (energy_cpu_load(&node_desc, 0)) {
		error("energy_cpu cannot be loaded");
		return EAR_ERROR;
	}

    debug("DVFS:Initializing frequency in dvfs_pc %u cpus",node_size);
    frequency_init(0);
    state_t ret_st = frequency_set_userspace_governor_all_cpus();
	check_usrspace_gov_set(ret_st, VEARD_PC);
    num_pstates = frequency_get_num_pstates();

    debug("DVFS: found %lu pstates", num_pstates);

    /* node_size or MAX_CPUS_SUPPORTED */
    c_freq = calloc(MAX_CPUS_SUPPORTED,sizeof(ulong));
    c_req_f = calloc(MAX_CPUS_SUPPORTED,sizeof(ulong));
    c_pstate = calloc(MAX_CPUS_SUPPORTED,sizeof(uint));
    t_pstate = calloc(MAX_CPUS_SUPPORTED,sizeof(uint));
    prev_pstate = calloc(MAX_CPUS_SUPPORTED,sizeof(uint));
    prev_counter = 0;
    int i;
    for (i = 0; i < MAX_CPUS_SUPPORTED; i++) prev_pstate[i] = UINT_MAX;

#if SHOW_DEBUGS
    ulong *freqlist = frequency_get_freq_rank_list();
    verbose_frequencies(node_size, freqlist);
#endif

    sus->suscribe(sus);
    return EAR_SUCCESS;
}

state_t plugin_set_burst()
{
    debug("setting dvfs to burst mode");
    return monitor_burst(sus_dvfs, 0);
}

state_t plugin_set_relax()
{
    return monitor_relax(sus_dvfs);
}

void plugin_get_settings(domain_settings_t *s)
{
	memcpy(s, &settings, sizeof(domain_settings_t));
}

void restore_frequency()
{
    int i;
    /* Get target frequency */
    memset(c_req_f , 0, sizeof(ulong)*MAX_CPUS_SUPPORTED);
    pmgt_get_app_req_freq(DOMAIN_CPU, c_req_f, node_size);
    //frequency_get_cpufreq_list(node_size,c_freq); //necessary??
    for (i = 0; i < node_size; i++) {
        //if (c_req_f[i] == 0) c_req_f[i] = frequency_get_nominal_freq();
        if (c_req_f[i] == 0) c_req_f[i] = pmgt_idle_def_freq;
    }
    verbose_frequencies(node_size, c_req_f);
    frequency_set_with_list(node_size, c_req_f);
}

state_t set_powercap_value(uint pid, uint domain, uint *limit, uint *cpu_util)
{
    int i;
    /* Set data */
    debug("%sDVFS:set_powercap_value %u%s", COL_BLU, *limit, COL_CLR);
    if (domain == LEVEL_DEVICE) {
        uint32_t new_limit = 0;
        for (int32_t i = 0; i < node_size; i++) {
            new_limit += limit[i];
        }
        warning("Per device powercap not implemented for DVFS, adding up the values (new limit %u)", new_limit);
        default_dvfs_pc = new_limit;
    } else {
        default_dvfs_pc = *limit;
    }
    current_dvfs_pc = default_dvfs_pc;
    dvfs_status = PC_STATUS_OK;
    dvfs_ask_def = 0;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++) prev_pstate[i] = UINT_MAX; //reset flipflop measures
    prev_counter = 0;
    if (current_dvfs_pc == POWER_CAP_UNLIMITED) {
        restore_frequency();
    }
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
    prev_counter = 0;
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
    prev_counter = 0;
    return EAR_SUCCESS;
}

state_t release_powercap_allocation(uint decrease)
{
    int i;
    debug("Decreasing powercap allocation by %u", decrease);
    current_dvfs_pc -= decrease;
    if (current_dvfs_pc < default_dvfs_pc) dvfs_status = PC_STATUS_RELEASE;
    for (i=0;i<MAX_CPUS_SUPPORTED;i++) prev_pstate[i] = UINT_MAX; //reset flipflop measures
    prev_counter = 0;
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
    if (current_dvfs_pc == POWER_CAP_UNLIMITED) return 0;
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
