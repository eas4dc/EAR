/*
 *
 * This program is par of the EAR software.
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
#define POLICY_PHASES 2
#include <sched.h>
#include <math.h>
#if MPI
#include <mpi.h>
#endif
#include <dlfcn.h>
// #define SHOW_DEBUGS 1
#include <common/includes.h>
#include <common/config.h>
#include <common/system/symplug.h>
#include <common/environment.h>
#include <management/cpufreq/frequency.h>
#include <management/imcfreq/imcfreq.h>
#include <library/metrics/signature_ext.h>
#include <library/api/mpi_support.h>
#include <library/api/clasify.h>
#include <library/metrics/gpu.h>
#include <library/policies/policy_ctx.h>
#include <library/policies/policy.h>
#include <library/policies/common/imc_policy_support.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/common/gpu_support.h>
#include <library/policies/common/cpu_support.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/common/global_comm.h>
#include <common/environment.h>
#include <daemon/local_api/eard_api.h>
#if POWERCAP
#include <daemon/powercap/powercap_status_conf.h>
#include <common/types/pc_app_info.h>
#include <library/policies/common/pc_support.h>
#include <library/policies/policy_state.h>
#endif

extern masters_info_t masters_info;
extern cpu_set_t ear_process_mask;
extern int ear_affinity_is_set;
extern uint *imc_max_pstate,*imc_min_pstate;
extern uint imc_mgt_compatible;
extern uint imc_devices;
extern const pstate_t *imc_pstates;
int my_globalrank;
signature_t policy_last_local_signature,policy_last_global_signature;
uint dyn_unc = EAR_eUFS;
uint enable_load_balance = EAR_USE_LB;
uint use_turbo_for_cp = EAR_USE_TURBO_CP;
uint try_turbo_enabled = 0;
uint use_energy_models = 1;
double imc_extra_th = EAR_IMC_TH;

uint total_mpi = 0;
static uint first_print = 1;
#define IMC_STEP 100000
const char     *polsyms_nam[] = {
    "policy_init",
    "policy_apply",
    "policy_app_apply",
    "policy_get_default_freq",
    "policy_ok",
    "policy_max_tries",
    "policy_end",
    "policy_loop_init",
    "policy_loop_end",
    "policy_new_iteration",
    "policy_mpi_init",
    "policy_mpi_end",
    "policy_configure",
    "policy_domain",
    "policy_io_settings",
    "policy_busy_wait_settings",
    "policy_restore_settings",
};


#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define DEF_FREQ(f) (!ext_def_freq?f:ext_def_freq)
#else
#define DEF_FREQ(f) f
#endif

#if POWERCAP
extern pc_app_info_t *pc_app_info_data;
#endif


static polsym_t polsyms_fun;
const int       polsyms_n = 17;
polctx_t my_pol_ctx;
node_freqs_t nf,avg_nf;

static ulong *freq_per_core;
static node_freq_domain_t freqs_domain;
uint last_earl_phase_classification = APP_COMP_BOUND;
uint last_earl_gpu_phase_classification = GPU_COMP; 
static const ulong **gpuf_pol_list;
static const uint *gpuf_pol_list_items;
static uint exclusive_mode = 0;
static uint use_earl_phases = EAR_USE_PHASES;

int cpu_ready = EAR_POLICY_NO_STATE ,gpu_ready = EAR_POLICY_NO_STATE ;
uint gidle;
/**** Auxiliary functions ***/
state_t policy_load(char *obj_path,polsym_t *psyms)
{
    state_t st;
    //return symplug_open(obj_path, (void **)&polsyms_fun, polsyms_nam, polsyms_n);
    if ((st = symplug_open(obj_path, (void **)psyms, polsyms_nam, polsyms_n)) != EAR_SUCCESS){
        verbose_master(2,"Error loading policy file %s",state_msg);
    }
    return st;	
}

void print_policy_ctx(polctx_t *p)
{
    verbose_master(2,"user_type %u",p->app->user_type);
    verbose_master(2,"policy_name %s",p->app->policy_name);
    verbose_master(2,"setting 0 %lf",p->app->settings[0]);
    verbose_master(2,"def_freq %lu",DEF_FREQ(p->app->def_freq));
    verbose_master(2,"def_pstate %u",p->app->def_p_state);
    verbose_master(2,"reconfigure %d",p->reconfigure->force_rescheduling);
    verbose_master(2,"user_selected_freq %lu",p->user_selected_freq);
    verbose_master(2,"reset_freq_opt %lu",p->reset_freq_opt);
    verbose_master(2,"ear_frequency %lu",*(p->ear_frequency));
    verbose_master(2,"num_pstates %u",p->num_pstates);
    verbose_master(2,"use_turbo %u",p->use_turbo);
    verbose_master(2,"num_gpus %u",p->num_gpus);
}


static ulong compute_avg_freq(ulong *my_policy_list,int n)
{
    int i;
    ulong total=0;
    for (i=0; i<n;i++)
    {
        total +=my_policy_list[i];
    }
    return total/n;
}
static void print_freq_per_core()
{
    int i;
    for (i=0;i<arch_desc.top.cpu_count;i++)
    {
        if (freq_per_core[i]>0){ 
            verbosen_master(3,"CPU[%d]=%.3f ",i,(float)freq_per_core[i]/1000000.0);
        }else{	  							 
            verbosen_master(3,"CPU[%d]= --",i);
        }
    }
    verbose_master(3," ");
}
static void from_proc_to_core(ulong *cpuf)
{
    int p,c;
    ulong f;
    int ccount;
    int lv = 2;
    cpu_set_t m;
    if (first_print){ lv = 1;first_print = 0;}
    verbose_master(lv,"Generating list of freqs for %d procs",lib_shared_region->num_processes);
    ccount = arch_desc.top.cpu_count;
    for (p = 0; p < lib_shared_region->num_processes ; p++)
    {
        f = cpuf[p];
        m = sig_shared_region[p].cpu_mask;
        verbose_master(lv,"Frequency for process %d is %lu",p,f);
        for (c = 0 ; c < ccount; c++){
            if (CPU_ISSET(c,&m)){ 
                verbosen_master(lv+1,"CPU %d ",c);
                freq_per_core[c] = f;
            }
        }
        verbose_master(lv+1," ");
    }
    return;
}

void fill_cpus_out_of_cgroup(uint exc)
{
    int ccount, c;
    if (exc == 0) return;
    ccount = arch_desc.top.cpu_count;
    for (c = 0; c < ccount; c++){
        if (freq_per_core[c] == 0){
            freq_per_core[c] = frequency_pstate_to_freq(frequency_get_num_pstates()-1);
        }
    }	

}

ulong avg_to_khz(ulong freq_khz)
{
    ulong newf;
    float ff;

    ff = roundf((float) freq_khz / 100000.0);
    newf = (ulong) ((ff / 10.0) * 1000000);
    return newf;
}

//static void policy_cpu_freq_selection(signature_t *my_sig,node_freqs_t *freqs)
static state_t policy_cpu_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    ulong *freq_set = freqs->cpu_freq;
    int i;
    char pol_grain_str[128];
    verbose_master(2,"policy_cpu_freq_selection");
#if POWERCAP
    if ((pc_app_info_data != NULL) && (pc_app_info_data->cpu_mode == PC_DVFS)){
        /* Warning: POL_GRAIN_CORE not supported */
        pcapp_info_set_req_f(pc_app_info_data, freq_set, MAX_CPUS_SUPPORTED);
    }
#endif
    memset(freq_per_core,0,sizeof(ulong)*MAX_CPUS_SUPPORTED);
    if (dom->cpu == POL_GRAIN_CORE) sprintf(pol_grain_str,"POL_GRAIN_CORE");
    else sprintf(pol_grain_str,"POL_GRAIN_NODE");
    verbose_master(2,"Policy level set to %s and affinity %d",pol_grain_str,ear_affinity_is_set);
    /* Assumption: If affinity is set for master, it is set for all, we could check individually */
    if ((dom->cpu == POL_GRAIN_CORE) && (ear_affinity_is_set)){
        from_proc_to_core(freq_set);		
    }else{
        debug("Setting same freq in all node %lu",freq_set[0]);
        for (i=1;i<MAX_CPUS_SUPPORTED;i++) freq_set[i] = freq_set[0];
        if (ear_affinity_is_set){ 
            from_proc_to_core(freq_set);
        }else{
            for (i=0;i<MAX_CPUS_SUPPORTED;i++) freq_per_core[i] = freq_set[0];
        }
    }
    if (dom->cpu == POL_GRAIN_CORE){
        fill_cpus_out_of_cgroup(exclusive_mode);
    }

    if (*freq_set != *(c->ear_frequency) || dom->cpu == POL_GRAIN_CORE)
    {
        print_freq_per_core();
        *(c->ear_frequency) = eards_change_freq_with_list(arch_desc.top.cpu_count,freq_per_core);
        if (*(c->ear_frequency) == 0) *(c->ear_frequency) = freq_set[0];
    }
    avg_freq->cpu_freq[0] = compute_avg_freq(freq_set,lib_shared_region->num_processes);
    verbose_master(2,"Setting CPU freq to %lu",*(c->ear_frequency));
    return EAR_SUCCESS;
}
static state_t policy_mem_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    uint *imc_up,*imc_low;
    if ((freqs == NULL) || (freqs->imc_freq == NULL)){
        verbose_master(2,"ERror policy_mem_freq_selection NULL ");
        return EAR_SUCCESS;
    }
    verbose_master(2,"Lower IMC pstate %lu and upper %lu",freqs->imc_freq[IMC_MAX],freqs->imc_freq[IMC_MIN]);
    if (dyn_unc && imc_mgt_compatible){
        imc_up = calloc(imc_devices,sizeof(uint));
        imc_low = calloc(imc_devices,sizeof(uint));
        /* We are using the same imc pstates for all the devices for now */
        for (int sid = 0;sid <imc_devices ;sid++) {
            imc_low[sid] = freqs->imc_freq[sid*IMC_VAL+IMC_MAX];	
            imc_up[sid] = freqs->imc_freq[sid*IMC_VAL+IMC_MIN];	
            if (imc_up[sid] > imc_max_pstate[sid]){ 
                verbose_master(2,"ERROR imc_up greather than max %u",imc_up[sid] );
                return EAR_SUCCESS;
            }
            if (imc_low[sid] > imc_max_pstate[sid]){ 
                verbose_master(2,"ERROR imc_low greather than max %u",imc_low[sid] );
                return EAR_SUCCESS;
            }
            if (imc_up[sid] != ps_nothing){
                verbose_master(2,"Setting IMC freq:SID=%d to %llu and %llu",sid,imc_pstates[imc_low[sid]].khz,imc_pstates[imc_up[sid]].khz);
            }else{
                verbose_master(2,"Setting IMC freq:SID=%d to %llu and min",sid,imc_pstates[imc_low[sid]].khz);
            }
        }
        if (mgt_imcfreq_set_current_ranged_list(NULL, imc_low, imc_up) != EAR_SUCCESS){
            uint sid = 0;
            if (imc_up[sid] != ps_nothing){
                verbose_master(2,"Error setting IMC freq to %llu and %llu",imc_pstates[imc_low[0]].khz,imc_pstates[imc_up[0]].khz);
            }else{
                verbose_master(2,"Error setting IMC freq to %llu and min",imc_pstates[imc_low[0]].khz);
            }
        }			
        free(imc_up);
        free(imc_low);
    }else{
        verbose_master(2,"IMC not compatible");
    }

    return EAR_SUCCESS;
}
static state_t policy_gpu_mem_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    return EAR_SUCCESS;
}


//state_t policy_gpu_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs, int *ready)
static state_t policy_gpu_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    ulong *gpu_f = freqs->gpu_freq;
    int i;
    if (c->num_gpus == 0) return EAR_SUCCESS;
    /* Powercap limits are checked here but this code could be moved to a function */
#if USE_GPUS
#if POWERCAP
    for (i=0;i<c->num_gpus;i++) verbosen_master(2," GPU freq[%d] = %.2f ",i,(float)gpu_f[i]/1000000.0);
    verbose_master(2," ");
    pc_app_info_data->num_gpus_used = my_sig->gpu_sig.num_gpus;
    memcpy(pc_app_info_data->req_gpu_f,gpu_f,c->num_gpus*sizeof(ulong));
#endif
    /* Pending: We must filter the GPUs I'm using */
    if (gpu_lib_freq_limit_set(gpu_f) != EAR_SUCCESS){
        verbose_master(2,"GPU freq set error ");
    }
    avg_freq->gpu_freq[0] = compute_avg_freq(gpu_f,my_sig->gpu_sig.num_gpus);
#endif
    return EAR_SUCCESS;

}

state_t policy_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    if (masters_info.my_master_rank < 0 ) return EAR_SUCCESS;
    if (dom->cpu != POL_NOT_SUPPORTED){
        policy_cpu_freq_selection(c,my_sig,freqs,dom,avg_freq);
    }
    if (dom->mem != POL_NOT_SUPPORTED){
        policy_mem_freq_selection(c,my_sig,freqs,dom,avg_freq);
    }
    if (dom->gpu != POL_NOT_SUPPORTED){
        policy_gpu_freq_selection(c,my_sig,freqs,dom,avg_freq);
    }
    if (dom->gpumem != POL_NOT_SUPPORTED){
        policy_gpu_mem_freq_selection(c,my_sig,freqs,dom,avg_freq);
    }
    return EAR_SUCCESS;
}

/* This function confgures freqs based on IO criteria */
state_t policy_io_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    if (polsyms_fun.io_settings != NULL){
        polsyms_fun.io_settings(c,my_sig,freqs);
    }
    return EAR_SUCCESS;
}

/* This function sets again default frequencies after a setting modification */
state_t policy_restore_comp_configuration(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    if (polsyms_fun.restore_settings != NULL){
        polsyms_fun.restore_settings(c,my_sig,freqs);
        return policy_freq_selection(c,my_sig,freqs,dom,avg_freq);
    }else return EAR_SUCCESS;
}
state_t policy_busy_waiting_configuration(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    if (polsyms_fun.busy_wait_settings != NULL){
        polsyms_fun.busy_wait_settings(c,my_sig,freqs);
    }
    return EAR_SUCCESS;
}

state_t policy_domain_detail(int status, char* buff){
    switch (status){
        case POL_NOT_SUPPORTED:
            sprintf(buff, "%sNOT SUPPORTED%s", COL_RED, COL_CLR);
            return EAR_SUCCESS;
        case POL_GRAIN_CORE:
            sprintf(buff, "%sGRAIN CORE%s", COL_GRE, COL_CLR);
            return EAR_SUCCESS;
        case POL_GRAIN_NODE:
            sprintf(buff, "%sGRAIN NODE%s", COL_GRE, COL_CLR);
            return EAR_SUCCESS;
        default:
            break;
    }
    return EAR_ERROR;
}

state_t policy_show_domain(node_freq_domain_t *dom){
    char detail[64];
    if (policy_domain_detail(dom->cpu, detail) == EAR_SUCCESS) {
        verbose_master(2, "CPU: %s", detail);
    }
    else return EAR_ERROR;

    if (policy_domain_detail(dom->mem, detail) == EAR_SUCCESS) {
        verbose_master(2, "MEM: %s", detail);
    }
    else return EAR_ERROR;

    if (policy_domain_detail(dom->gpu, detail) == EAR_SUCCESS) {
        verbose_master(2, "GPU: %s", detail);
    }
    else return EAR_ERROR;

    if (policy_domain_detail(dom->gpumem, detail) == EAR_SUCCESS) {
        verbose_master(2, "GPU_MEM: %s", detail);
    }
    else return EAR_ERROR;

    return EAR_SUCCESS;
}


/**** End auxiliary functions ***/
/* This function is called automatically  by the init_power_policy that loads policy symbols */
state_t policy_init()
{
    polctx_t *c = &my_pol_ctx;

    int i;

    char *cdyn_unc = getenv(SCHED_SET_IMC_FREQ);
    char *cimc_extra_th = getenv(SCHED_IMC_EXTRA_TH);
    char *cexclusive_mode = getenv(SCHED_EXCLUSIVE_MODE);
    char *cuse_earl_phases = getenv(SCHED_USE_EARL_PHASES);
    char *cenable_load_balance = getenv(SCHED_ENABLE_LOAD_BALANCE);
    char *cuse_turbo_for_cp = getenv(SCHED_USE_TURBO_FOR_CP);
    char *ctry_turbo_enabled = getenv(SCHED_TRY_TURBO);
    char *cuse_energy_models = getenv(SCHED_USE_ENERGY_MODELS);

    state_t retg=EAR_SUCCESS, ret = EAR_SUCCESS;
    policy_gpu_init_ctx(c);
    // print_policy_ctx(c);

    /* We read SCHED_USE_ENERGY_MODELS before calling to polsyms_fun.init() because this function
     * may needs `use_energy_models` to configure itself. */
    if (cuse_energy_models != NULL){
        use_energy_models = atoi(cuse_energy_models);
    }
    verbose_master(2,"Using energy models set to %u",use_energy_models);

    if (polsyms_fun.init != NULL){
        ret=polsyms_fun.init(c);
    }
    /*  Read policy configuration */
    if (cexclusive_mode != NULL) exclusive_mode = atoi(cexclusive_mode);
    verbose_master(2,"Running in exclusive mode set to %u", exclusive_mode);

    if (cuse_earl_phases != NULL) use_earl_phases = atoi(cuse_earl_phases);
    verbose_master(2,"Phases detection set to %u",use_earl_phases);

    /* Even though the policy could be at the Node granularity, we will use a list */
#if USE_GPUS
    gpu_lib_freq_list(&gpuf_pol_list, &gpuf_pol_list_items);
#endif
    node_freqs_alloc(&nf);
    node_freqs_alloc(&avg_nf);
    freq_per_core = calloc(MAX_CPUS_SUPPORTED,sizeof(ulong));
    signature_init(&policy_last_local_signature);
    signature_init(&policy_last_global_signature);

    if (cenable_load_balance != NULL) enable_load_balance = atoi(cenable_load_balance);
    if (arch_desc.top.vendor == VENDOR_AMD) {
        enable_load_balance = 0;
    }
    verbose_master(2,"Load balance enabled ? %u", enable_load_balance);

    if (cuse_turbo_for_cp != NULL) use_turbo_for_cp = atoi(cuse_turbo_for_cp);
    verbose_master(2,"Using turbo for CP when load unbalance set to %u", use_turbo_for_cp);

   
       if (cdyn_unc != NULL && imc_mgt_compatible){
       dyn_unc = atoi(cdyn_unc);
       }

    if (cimc_extra_th != NULL){
        imc_extra_th = atof(cimc_extra_th);
    }

    try_turbo_enabled = c->use_turbo;
    if (ctry_turbo_enabled != NULL) try_turbo_enabled = atoi(ctry_turbo_enabled);
    verbose_master(2,"Using turbo for cbound app set to %u",try_turbo_enabled);

    if (polsyms_fun.domain != NULL){
        ret = polsyms_fun.domain(c,&freqs_domain);
    }else{
        freqs_domain.cpu = POL_GRAIN_NODE;
        freqs_domain.mem = POL_NOT_SUPPORTED;
        freqs_domain.gpu = POL_NOT_SUPPORTED;
        freqs_domain.gpumem = POL_NOT_SUPPORTED;
    }

    if (polsyms_fun.domain != NULL){
        ret = polsyms_fun.domain(c,&freqs_domain);
    }else{
        freqs_domain.cpu = POL_GRAIN_NODE;
        freqs_domain.mem = POL_NOT_SUPPORTED;
        freqs_domain.gpu = POL_NOT_SUPPORTED;
        freqs_domain.gpumem = POL_NOT_SUPPORTED;
    }
    if (masters_info.my_master_rank >= 0){
        for (i=0; i< lib_shared_region->num_processes;i++){
            sig_shared_region[i].unbalanced = 0;
        }
    }
    /*  Verboses policy's domain*/
    if (policy_show_domain(&freqs_domain) == EAR_ERROR){
        verbose_master(2, "%sERROR on getting domains.%s",COL_RED, COL_CLR);
    }
#if POWERCAP
    if (masters_info.my_master_rank>=0){
        pc_support_init(c);
    }
#endif
    if ((ret == EAR_SUCCESS) && (retg == EAR_SUCCESS)) return EAR_SUCCESS;
    else return EAR_ERROR;
}




/* This function loads specific CPU and GPU policy functions and calls the initialization functions */

state_t init_power_policy(settings_conf_t *app_settings,resched_t *res)
{
    char basic_path[SZ_PATH_INCOMPLETE];
    conf_install_t *data=&app_settings->installation;

    /* This part of code is to decide which policy to load based on default and hacks */
    char *obj_path = getenv(SCHED_EAR_POWER_POLICY);
    char *ins_path = getenv(SCHED_EARL_INSTALL_PATH);
    char *app_mgr_policy = getenv(USE_APP_MGR_POLICIES);
    char use_path[SZ_PATH_INCOMPLETE];

    int app_mgr=0;
    if (app_mgr_policy != NULL){
        app_mgr = atoi(app_mgr_policy);
    }
#if SHOW_DEBUGS
    if (masters_info.my_master_rank >=0){
        if (obj_path!=NULL){ 
            debug("%s = %s",SCHED_EAR_POWER_POLICY,obj_path);
        }else{
            debug("%s undefined",SCHED_EAR_POWER_POLICY);
        }
        if (ins_path !=NULL){
            debug("%s = %s",SCHED_EARL_INSTALL_PATH,ins_path);
        }else{
            debug("%s undefined",SCHED_EARL_INSTALL_PATH);
        }
    }
#endif
    if (((obj_path==NULL) && (ins_path == NULL)) || (app_settings->user_type!=AUTHORIZED)){
        if (!app_mgr){
            xsnprintf(basic_path,sizeof(basic_path),"%s/policies/%s.so",data->dir_plug,app_settings->policy_name);
        }else{
            xsnprintf(basic_path,sizeof(basic_path),"%s/policies/app_%s.so",data->dir_plug,app_settings->policy_name);
        }
        obj_path=basic_path;
    }else{
        if (obj_path == NULL){
            if (ins_path != NULL){
                xsnprintf(use_path,sizeof(use_path),"%s/plugins/policies", ins_path);	
            }else{
                xsnprintf(use_path,sizeof(use_path),"%s/policies", data->dir_plug);
            }
            if (!app_mgr){
                xsnprintf(basic_path,sizeof(basic_path),"%s/%s.so",use_path,app_settings->policy_name);		
            }else{
                xsnprintf(basic_path,sizeof(basic_path),"%s/app_%s.so",use_path,app_settings->policy_name);		
            }
            obj_path=basic_path;
        }
    }
    /* End policy file selection */
    if (masters_info.my_master_rank>=0) verbose_master(2,"loading policy %s",obj_path);
    /* loads the policy plugin */
    if (policy_load(obj_path,&polsyms_fun)!=EAR_SUCCESS){
        verbose_master(2,"Error loading policy %s:%s",obj_path,state_msg);
    }
    ear_frequency									= DEF_FREQ(app_settings->def_freq);
    my_pol_ctx.app								= app_settings;
    my_pol_ctx.reconfigure				= res;
    my_pol_ctx.user_selected_freq	= DEF_FREQ(app_settings->def_freq);
    my_pol_ctx.reset_freq_opt			= get_ear_reset_freq();
    my_pol_ctx.ear_frequency			=	&ear_frequency;
    my_pol_ctx.num_pstates				=	frequency_get_num_pstates();
    my_pol_ctx.use_turbo 					= ear_use_turbo;
    my_pol_ctx.affinity  					= ear_affinity_is_set;
    my_globalrank = masters_info.my_master_rank;
#if POWERCAP
    my_pol_ctx.pc_limit						= app_settings->pc_opt.current_pc;
#else
    my_pol_ctx.pc_limit           = 0;
#endif
    verbose_master(2,"Default frequency set to %lu",*(my_pol_ctx.ear_frequency));
#if MPI
    if (PMPI_Comm_dup(masters_info.new_world_comm,&my_pol_ctx.mpi.comm)!=MPI_SUCCESS){
        verbose_master(2,"Duplicating COMM_WORLD in policy");
    }
    if (masters_info.my_master_rank>=0){
        if (PMPI_Comm_dup(masters_info.masters_comm, &my_pol_ctx.mpi.master_comm)!=MPI_SUCCESS){
            verbose_master(2,"Duplicating master_comm in policy");
        }
    }
#endif
    return policy_init();
}


/* Policy functions previously included in models */

/*
 *
 * Policy wrappers
 *
 */



/* This is the main function for frequency selection at application level */
/* The signatures are used first the last signatur computed is a global variable, second signatures are in the shared memory */
/* freq_set and ready are output values */
/* There is not a GPU part here, still pending */
/* The CPU freq and GPu freq are set in this function, so freq_set is just a reference for the CPU freq selected */


state_t policy_app_apply(ulong *freq_set, int *ready)
{
    polctx_t *c=&my_pol_ctx;
    state_t st = EAR_SUCCESS;
    node_freqs_t *freqs = &nf;
    *ready = EAR_POLICY_READY;
    if (polsyms_fun.app_policy_apply == NULL){
        *ready = EAR_POLICY_LOCAL_EV;
        return st;
    }
    if (!eards_connected() || (masters_info.my_master_rank<0)){
        *ready = EAR_POLICY_CONTINUE;
        return st;
    }
    if (freqs_domain.cpu == POL_GRAIN_CORE) memset(freqs->cpu_freq,0,sizeof(ulong)*MAX_CPUS_SUPPORTED);
    st=polsyms_fun.app_policy_apply(c, &policy_last_global_signature, freqs,ready);
    if (*ready == EAR_POLICY_READY){
        debug("Average frequency after app_policy is %lu",*freq_set );
        //policy_cpu_freq_selection(&policy_last_global_signature,freqs);
        st = policy_freq_selection(c,&policy_last_global_signature,freqs,&freqs_domain,&avg_nf);
        *freq_set = avg_nf.cpu_freq[0];
    }
    return st;
}

static int is_all_ready()
{
    if (my_pol_ctx.num_gpus == 0) gpu_ready = EAR_POLICY_READY;
    if ((cpu_ready == EAR_POLICY_READY) && (gpu_ready == EAR_POLICY_READY)) return 1;
    return 0;
}

static int is_try_again()
{
    if ((cpu_ready == EAR_POLICY_TRY_AGAIN) || (gpu_ready == EAR_POLICY_TRY_AGAIN)) return 1;
    return 0;
}

/* This is the main function for frequency selection at node level */
/* my_sig is an input and freq_set and ready are output values */
/* The CPU freq and GPu freq are set in this function, so freq_set is just a reference for the CPU freq selected */
state_t policy_node_apply(signature_t *my_sig,ulong *freq_set, int *ready)
{
    polctx_t *c=&my_pol_ctx;
    signature_t node_sig;
    node_freqs_t *freqs = &nf;
    state_t st=EAR_ERROR;
    uint busy, iobound, mpibound,earl_phase_classification;
    sig_ext_t *se;
    float mpi_in_period;

    se = (sig_ext_t *)my_sig->sig_ext;
    cpu_ready = EAR_POLICY_NO_STATE;
    gpu_ready = EAR_POLICY_NO_STATE;
    if (!eards_connected() || (masters_info.my_master_rank<0)){
        *ready = EAR_POLICY_CONTINUE;
        return EAR_SUCCESS;
    }
#if 0
    if (!are_signatures_ready(lib_shared_region,sig_shared_region)){ 
        *ready = EAR_POLICY_CONTINUE;
        uint pending_sig = lib_shared_region->num_processes-compute_total_signatures_ready(lib_shared_region,sig_shared_region);
        if (pending_sig > (lib_shared_region->num_processes/2)){
            verbose_master(2,"Num sig pending %d:",pending_sig);	
        }
        return EAR_SUCCESS;
    }
#endif
    mpi_in_period = compute_mpi_in_period(se->mpi_stats);

    is_io_bound(my_sig->IO_MBS,&iobound);
    is_network_bound(mpi_in_period,&mpibound);
    classify(iobound, mpibound, &earl_phase_classification);
    is_cpu_busy_waiting(my_sig,&busy);
    is_gpu_idle(my_sig,&gidle);
    if (c->num_gpus && (gidle == GPU_IDLE)) verbose_master(2,"%sGPU idle phase%s",COL_BLU,COL_CLR);
    verbose_master(POLICY_PHASES,"EARL_phase %u busy waiting %u iobound %u mpibound %u using phases %u",earl_phase_classification,busy,iobound,mpibound,use_earl_phases);
    if (use_earl_phases){
        /* IO Use case */
        if (earl_phase_classification == APP_IO_BOUND){
            if (busy == BUSY_WAITING){
                verbose_master(POLICY_PHASES,"%sIO configuration%s",COL_BLU,COL_CLR);
                st = policy_io_freq_selection(c,my_sig,freqs,&freqs_domain,&avg_nf);
                /* GPU ? */
                *freq_set = avg_nf.cpu_freq[0];
                if (is_all_ready()){
                    last_earl_phase_classification = earl_phase_classification;
                    last_earl_gpu_phase_classification = gidle;
                    policy_freq_selection(c,my_sig,freqs,&freqs_domain,&avg_nf);
                    *ready = EAR_POLICY_READY;
                    clean_signatures(lib_shared_region,sig_shared_region);
                    return EAR_SUCCESS;
                }
            }else if (last_earl_phase_classification == APP_IO_BOUND){
                verbose_master(POLICY_PHASES,"IO and Not busy waiting: COMP-MPI configuration");
                st = policy_restore_comp_configuration(c,my_sig,freqs,&freqs_domain,&avg_nf);
                last_earl_phase_classification = APP_COMP_BOUND;
                last_earl_gpu_phase_classification = gidle;
                *ready = EAR_POLICY_CONTINUE;
                *freq_set = avg_nf.cpu_freq[0];
                clean_signatures(lib_shared_region,sig_shared_region);
                return EAR_SUCCESS;
            }else{
                verbose_master(POLICY_PHASES,"IO -> COMP");
                earl_phase_classification = APP_COMP_BOUND;
            }
        }
        /* Restore configuration to COMP or MPI if needed*/
        if ((earl_phase_classification == APP_COMP_BOUND) || (earl_phase_classification == APP_MPI_BOUND)){
            /* Are we in a busy waiting phase ? */
            if (busy == BUSY_WAITING){
                verbose_master(POLICY_PHASES,"%sCPU Busy waiting configuration%s",COL_BLU,COL_CLR);
                st = policy_busy_waiting_configuration(c,my_sig,freqs,&freqs_domain,&avg_nf);
                *freq_set = avg_nf.cpu_freq[0];
                if (is_all_ready()){
                    policy_freq_selection(c,my_sig,freqs,&freqs_domain,&avg_nf);
                    *ready = EAR_POLICY_READY;
                    last_earl_gpu_phase_classification = gidle;
                    last_earl_phase_classification = APP_BUSY_WAITING;
                    clean_signatures(lib_shared_region,sig_shared_region);
                    return EAR_SUCCESS;
                }

            }
            /* If not, we should restore to default settings */
            if ((last_earl_phase_classification == APP_BUSY_WAITING) || (last_earl_phase_classification == APP_IO_BOUND) || (last_earl_gpu_phase_classification != gidle)){
                verbose_master(POLICY_PHASES,"COMP-MPI configuration");
                st = policy_restore_comp_configuration(c,my_sig,freqs,&freqs_domain,&avg_nf);
                *freq_set = avg_nf.cpu_freq[0];
                *ready = EAR_POLICY_CONTINUE;
                last_earl_phase_classification = earl_phase_classification;
                last_earl_gpu_phase_classification = gidle;
                clean_signatures(lib_shared_region,sig_shared_region);
                return st;		
            }
        }
    }
    verbose_master(POLICY_PHASES,"%sCOMP or MPI bound and not busy waiting--> Node policy%s",COL_BLU,COL_CLR);
    last_earl_phase_classification = earl_phase_classification;
    last_earl_gpu_phase_classification = gidle;
    /* Here we are COMP or MPI bound AND not busy waiting*/
    if (polsyms_fun.node_policy_apply!=NULL){

        /* Initializations */
        signature_copy(&node_sig,my_sig);
        node_sig.DC_power=sig_node_power(my_sig);
        signature_copy(&policy_last_local_signature,&node_sig);
        if ((cpu_ready != EAR_POLICY_READY) && (freqs_domain.cpu == POL_GRAIN_CORE)) memset(freqs->cpu_freq,0,sizeof(ulong)*MAX_CPUS_SUPPORTED);

        /* CPU specific function is applied here */
        st=polsyms_fun.node_policy_apply(c, &node_sig,freqs,&cpu_ready);

        /* Depending on the policy status and granularity we adapt the CPU freq selection */
        if (is_all_ready() || is_try_again()){
            /* This functions  checks the powercap and the processor affinity, finally sets the CPU freq asking the eard to do it */
            // policy_cpu_freq_selection(my_sig,freqs);
            st = policy_freq_selection(c,&node_sig,freqs,&freqs_domain,&avg_nf);
            *freq_set = avg_nf.cpu_freq[0];
        } /* Stop*/
    } else{
        if (polsyms_fun.app_policy_apply != NULL ){
            *ready = EAR_POLICY_GLOBAL_EV;
        }else{
            if (c!=NULL) *freq_set=DEF_FREQ(c->app->def_freq);
        }
    }
    if (is_all_ready()) *ready = EAR_POLICY_READY;
    else *ready = EAR_POLICY_CONTINUE;
    if (*ready == EAR_POLICY_READY) verbose_master(2,"%s Node policy ready%s",COL_GRE,COL_CLR);
    verbose_master(3,"Policy cpufreq selection returns %d,gpu policy returns %d, Global node readiness %d",cpu_ready,gpu_ready,*ready);
    clean_signatures(lib_shared_region,sig_shared_region);
    return EAR_SUCCESS;
}

/* This function returns the default freq (only CPU) */
state_t policy_get_default_freq(node_freqs_t *freq_set)
{ 
    polctx_t *c=&my_pol_ctx;
    state_t ret;
    if (masters_info.my_master_rank < 0 ) return EAR_SUCCESS;
    if (polsyms_fun.get_default_freq != NULL){
        ret = polsyms_fun.get_default_freq( c, freq_set, NULL);
        return ret;
    }else{
        if (c!=NULL) freq_set->cpu_freq[0] = DEF_FREQ(c->app->def_freq);
        return EAR_SUCCESS;
    }
}


/* This function sets the default freq (only CPU) */
state_t policy_set_default_freq(signature_t *sig)
{
    polctx_t *c=&my_pol_ctx;
    state_t st;
    signature_t s;
    if (masters_info.my_master_rank < 0 ) return EAR_SUCCESS;
#if USE_GPUS
    s.gpu_sig.num_gpus = c->num_gpus;
#endif
    if (polsyms_fun.get_default_freq != NULL){
        st = polsyms_fun.get_default_freq(c,&nf,sig);
        if (masters_info.my_master_rank>=0) policy_freq_selection(c,&s,&nf,&freqs_domain , &avg_nf);
        return st;
    }else{ 
        return EAR_ERROR;
    }
}


/* This function checks the CPU freq selection accuracy (only CPU), ok is an output */
state_t policy_ok(signature_t *curr,signature_t *prev,int *ok)
{
    polctx_t *c=&my_pol_ctx;
    state_t ret,retg=EAR_SUCCESS;
    if (curr == NULL || prev == NULL){
        *ok = 1;
        return EAR_SUCCESS;
    }
    if (masters_info.my_master_rank < 0 ){*ok = 1; return EAR_SUCCESS;}
    if (polsyms_fun.ok != NULL){
        /* We wait for node signature computation */
        if (!are_signatures_ready(lib_shared_region,sig_shared_region)){
            *ok = 1;
            return EAR_SUCCESS;
        }
#if POWERCAP
        pc_support_compute_next_state(c,&my_pol_ctx.app->pc_opt,curr);
#endif
        ret = polsyms_fun.ok(c, curr,prev,ok);
        if (*ok == 0){
            last_earl_phase_classification = APP_COMP_BOUND;
            last_earl_gpu_phase_classification = GPU_COMP;
        }
        /* And then we mark as not ready */
        clean_signatures(lib_shared_region,sig_shared_region);
        if ((ret == EAR_SUCCESS) && (retg == EAR_SUCCESS)) return EAR_SUCCESS;
        else return EAR_ERROR;

    }else{
        *ok=1;
        return EAR_SUCCESS;
    }
}

/* This function returns the maximum mumber of attempts to select the optimal CPU frequency */
state_t policy_max_tries(int *intents)
{ 
    polctx_t *c=&my_pol_ctx;
    if (masters_info.my_master_rank < 0 ){*intents = 100000; return EAR_SUCCESS;}
    if (polsyms_fun.max_tries!=NULL){
        return	polsyms_fun.max_tries( c,intents);
    }else{
        *intents=1;
        return EAR_SUCCESS;
    }
}

/* This function is executed at application end */
state_t policy_end()
{
    polctx_t *c=&my_pol_ctx;
    preturn(polsyms_fun.end,c);
}
/** LOOPS */
/* This function is executed  at loop init or period init */
state_t policy_loop_init(loop_id_t *loop_id)
{
    polctx_t *c=&my_pol_ctx;
    preturn(polsyms_fun.loop_init,c, loop_id);
}

/* This function is executed at each loop end */
state_t policy_loop_end(loop_id_t *loop_id)
{
    polctx_t *c=&my_pol_ctx;
    preturn(polsyms_fun.loop_end,c, loop_id);
}
/* This function is executed at each loop iteration or beginning of a period */
state_t policy_new_iteration(signature_t *sig)
{
    state_t st = EAR_SUCCESS;
    polctx_t *c=&my_pol_ctx;
    /* This validation is common to all policies */
    /* Check cpu idle or gpu idle */
    if ((last_earl_phase_classification == APP_IO_BOUND) || (last_earl_phase_classification == APP_BUSY_WAITING)){
        verbose_master(3,"Validating CPU phase %lf ref %lf", sig->Gflops,GFLOPS_BUSY_WAITING);
        if (sig->Gflops > GFLOPS_BUSY_WAITING){ 
            /* We must reset the configuration */
            last_earl_phase_classification = APP_COMP_BOUND;
            last_earl_gpu_phase_classification = GPU_COMP;
            return EAR_ERROR;
        }
    }
#if USE_GPUS
    if ((last_earl_gpu_phase_classification == GPU_IDLE) && (sig->gpu_sig.num_gpus)){
        verbose_master(3,"Validating GPU phase");
        int i;
        for (i = 0; i< sig->gpu_sig.num_gpus; i++){
            if (sig->gpu_sig.gpu_data[i].GPU_util > 0){
                last_earl_gpu_phase_classification = GPU_COMP;
                return EAR_ERROR;
            }
        }
    }
#endif
    st = EAR_SUCCESS;
    if (polsyms_fun.new_iter != NULL) st = polsyms_fun.new_iter(c,sig);
    return st;
}

/** MPI CALLS */

/* This function is executed at the beginning of each mpi call: Warning! it could introduce a lot of overhead*/
state_t policy_mpi_init(mpi_call call_type)
{
    polctx_t *c=&my_pol_ctx;
    total_mpi ++;
    preturn_opt(polsyms_fun.mpi_init,c,call_type);
}

/* This function is executed after each mpi call: Warning! it could introduce a lot of overhead*/
state_t policy_mpi_end(mpi_call call_type)
{
    polctx_t *c=&my_pol_ctx;
    preturn_opt(polsyms_fun.mpi_end,c,call_type);
}

/** GLOBAL EVENTS */

/* This function is executed when a reconfiguration needs to be done*/
state_t policy_configure()
{
    polctx_t *c=&my_pol_ctx;
    preturn(polsyms_fun.configure, c);
}

/* This function is executed forces a given frequency independently of the policy */
state_t policy_force_global_frequency(ulong new_f)
{
    ear_frequency=new_f;
    if (masters_info.my_master_rank>=0) eards_change_freq(ear_frequency);
    return EAR_SUCCESS;
}

state_t policy_get_current_freq(node_freqs_t *freq_set)
{
    if (freq_set != NULL) node_freqs_copy(freq_set,&nf);
    return EAR_SUCCESS;
}

