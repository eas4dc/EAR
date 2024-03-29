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
 * EAR is an open source software, and it is licensed under both the BSD-3 license
 * and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD * and COPYING.EPL files.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sched.h>
// #define SHOW_DEBUGS 1
#include <pthread.h>
#include <common/config.h>
#include <common/config/config_install.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/system/loadavg.h>
#include <common/system/symplug.h>
#include <common/system/monitor.h>
#include <common/hardware/topology.h>
#include <common/hardware/hardware_info.h>
#include <common/types/configuration/cluster_conf.h>

#include <daemon/shared_configuration.h>
#include <daemon/power_monitor.h>
#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>
#if USE_GPUS
#include <metrics/gpu/gpu.h>
#include <management/gpu/gpu.h>
#endif
#include <management/cpupow/cpupow.h>


#ifndef SYN_TEST
extern cluster_conf_t my_cluster_conf;
extern node_powercap_opt_t my_pc_opt;
extern my_node_conf_t *my_node_conf;
#else
cluster_conf_t my_cluster_conf;
#endif

extern powermon_app_t *current_ear_app[MAX_NESTED_LEVELS];
extern int max_context_created;
extern int num_contexts;
ulong  pmgt_idle_def_freq;

typedef struct powercap_symbols {
    state_t (*enable)        (suscription_t *sus);
    state_t (*disable)       ();
    state_t (*set_powercap_value)(uint pid, uint domain, uint limit, ulong *util);
    state_t (*get_powercap_value)(uint pid, uint *powercap);
    uint    (*is_powercap_policy_enabled)(uint pid);
    void    (*set_status)(uint status);
    void    (*set_pc_mode)(uint mode);
    uint    (*get_powercap_strategy)();
    void    (*powercap_to_str)(char *b);
    void    (*print_powercap_value)(int fd);
    void    (*set_app_req_freq)(ulong *f);
    void    (*set_verb_channel)(int fd);    
    void    (*set_new_utilization)(ulong *util);    
    uint    (*get_powercap_status)(domain_status_t *status);
    state_t (*reset_powercap_value)();
    state_t (*release_powercap_allocation)(uint decrease);
    state_t (*increase_powercap_allocation)(uint increase);
    state_t (*plugin_set_burst)();
    state_t (*plugin_set_relax)();
    void 	(*plugin_get_settings)(domain_settings_t *settings);
} powercapsym_t;

static uint pmgt_limit;
static domain_status_t cdomain_status[NUM_DOMAINS];
static domain_settings_t cdomain_settings[NUM_DOMAINS];
static ulong *current_util[NUM_DOMAINS],*prev_util[NUM_DOMAINS];
static ulong  dom_util[NUM_DOMAINS],last_dom_util[NUM_DOMAINS],dom_changed[NUM_DOMAINS];
#if USE_GPUS || SHOW_DEBUGS
static ulong  dom_util_limit[NUM_DOMAINS];
#endif
#if USE_GPUS
/* pdomains is the power distribution currently in use. It changes depending on the current power balance between 
 * domains. It is reset to pdomains_def every time a job starts/ends. */
/* pdomains_idle is the power distribution that the powercap uses when there is no job running. It is currently
 * automatically set to pdomains_def at EARD start*/
/* pdomains_def is the default power distribution. It is recalculated at runtime once the domains have been
 * loaded, using the TDP as reference. */
/* pdomains_def_dummy is used when GPUs are enabled but the domain loads the dummy plugin*/
/* pdomains_min defines the minimum power balance that a certain domain can reach when rebalancing 
 * the node due to excessive power consumption. */
/* pdomains_min_dummy is used when GPUs are enabled but the domain loads the dummy plugin*/
static float pdomains[NUM_DOMAINS]={0.6,0.45,0,GPU_PERC_UTIL};
static float pdomains_idle[NUM_DOMAINS]={0.6,0.45,0,GPU_PERC_UTIL};
static float pdomains_def[NUM_DOMAINS]={0.6,0.5,0,GPU_PERC_UTIL};
static float pdomains_def_dummy[NUM_DOMAINS]={1.0,0.9,0,0};
static float pdomains_min[NUM_DOMAINS]={0.6,0.4,0,GPU_PERC_UTIL};
static float pdomains_min_dummy[NUM_DOMAINS]={1.0,0.75,0,0};
#else
static float pdomains[NUM_DOMAINS]={1.0, 0.9, 0, 0};
static float pdomains_idle[NUM_DOMAINS]={1.0, 0.9, 0, 0};
static float pdomains_def[NUM_DOMAINS]={1.0,0.9,0,0};
static float pdomains_min[NUM_DOMAINS]={1.0,0.7575,0,0};
#endif
static uint domains_loaded[NUM_DOMAINS]={0,0,0,0};
static powercapsym_t pcsyms_fun[NUM_DOMAINS];

static float security_range = 1.0;
//static void *pcsyms_obj[NUM_DOMAINS] ={ NULL,NULL,NULL,NULL};
const int   pcsyms_n = 20;

static suscription_t *sus[NUM_DOMAINS];

const char     *pcsyms_names[] ={
    "enable",
    "disable",
    "set_powercap_value",
    "get_powercap_value",
    "is_powercap_policy_enabled",
    "set_status",
    "set_pc_mode",
    "get_powercap_strategy",
    "powercap_to_str",
    "print_powercap_value",
    "set_app_req_freq",    
    "set_verb_channel",
    "set_new_utilization",
    "get_powercap_status",
    "reset_powercap_value",
    "release_powercap_allocation",
    "increase_powercap_allocation",
    "plugin_set_burst",
    "plugin_set_relax",
    "plugin_get_settings",
};

#ifndef SYN_TEST
#define UTIL_BURST_PERIOD 1000
#else
int UTIL_BURST_PERIOD = 1000;
void pmgt_set_burst(int b)
{
    UTIL_BURST_PERIOD = b;
}
#endif
#define UTIL_RELAX_PERIOD 60000
#define CHECK_STATUS_PERIOD 500

#define DOMAINS_REDIST 0.1
#define DOMAINS_REDIST_MAX 20


#define freturn(call, ...) ((call==NULL)?EAR_UNDEFINED:call(__VA_ARGS__));

#define DEFAULT_PC_PLUGIN_NAME_NODE "noplugin"
#define DEFAULT_PC_PLUGIN_NAME_CPU  "dvfs"
#define DEFAULT_PC_PLUGIN_NAME_DRAM "noplugin"
#if USE_GPUS
#define DEFAULT_PC_PLUGIN_NAME_GPU  "gpu_dvfs"
#else
#define DEFAULT_PC_PLUGIN_NAME_GPU  "noplugin"
#endif


#define CHECK_ASK_DEFAULT 0
#define CHECK_POWER_RED        1
//static uint pc_plugin_loaded=0;

#if USE_GPUS
static suscription_t *sus_util_detection;
static gpu_t *gpu_detection_raw_data;
static uint gpu_pc_num_gpus=0;
static uint gpu_pc_model = 0;
static uint util_period = 0;
#else
static uint gpu_pc_num_gpus=1;
#endif
static topology_t pc_topology_info;
/* These functions identies and monitors load changes */


static uint rapl_pck_tdps[MAX_PACKAGES];
static uint rapl_dram_tdps[MAX_PACKAGES];
#define TDP_CPU 150
#define TDP_DRAM 30
#define TDP_GPU 250

void init_tdps()
{
    mgt_cpupow_load(&pc_topology_info, NO_EARD);
    mgt_cpupow_tdp_get(CPUPOW_SOCKET, rapl_pck_tdps);
    mgt_cpupow_tdp_get(CPUPOW_DRAM, rapl_dram_tdps);
}

void compute_power_distribution()
{
    float node_ratio = cdomain_settings[DOMAIN_CPU].node_ratio;
    uint pd_total=0,pd_cpu=0,pd_dram=0, pd_gpu=0, pd_pckg;
    uint bpd_cpu,bpd_dram,bpd_gpu = 0;

    // Initialize TDP values
    init_tdps();

    /* CPU and DRAM are read in eard.c during the initialization */
    bpd_cpu = rapl_pck_tdps[0];
    bpd_dram = rapl_dram_tdps[0];
    debug("Using CPU TDP %u DRAM TDP %u", bpd_cpu, bpd_dram);
#if USE_GPUS
    if ((domains_loaded[DOMAIN_GPU]) && (gpu_pc_model != API_DUMMY)) bpd_gpu = TDP_GPU;
#endif
    pd_cpu = bpd_cpu * pc_topology_info.socket_count;
    pd_dram = bpd_dram * pc_topology_info.socket_count;
    pd_gpu = bpd_gpu * gpu_pc_num_gpus;
    pd_pckg = pd_cpu + pd_dram;
    pd_total = pd_pckg + pd_gpu + pd_pckg * node_ratio;        
    pdomains_def[DOMAIN_NODE] = ((float)pd_pckg +(float)pd_pckg * node_ratio) / (float)pd_total;
    pdomains_def[DOMAIN_CPU] = (float) pd_pckg / (float)pd_total;
    pdomains_def[DOMAIN_DRAM] = (float) pd_dram / (float)pd_total;
    pdomains_def[DOMAIN_GPUS] = 0;
    debug("pd_cpu: %u with %u sockets", pd_cpu, pc_topology_info.socket_count);
#if USE_GPUS
    pdomains_def[DOMAIN_GPUS] = (float) pd_gpu / (float)pd_total;
#endif
    memcpy(pdomains_idle,pdomains_def,sizeof(pdomains_def));
    debug("W[NODE]=%.2f W[CPU]=%.2f W[GPU]=%.2f",pdomains_def[DOMAIN_NODE],pdomains_def[DOMAIN_CPU],pdomains_def[DOMAIN_GPUS]);
}


uint pmgt_utilization_changed()
{
    uint ret=0,i;
    for (i=0;i<NUM_DOMAINS;i++){
        dom_changed[i]=0;
        if (i == DOMAIN_CPU || i == DOMAIN_NODE){
            if (util_changed(dom_util[i],last_dom_util[i])){
                ret=1;
                dom_changed[i]=1;
            }
        }
#if USE_GPUS
        else if ((i == DOMAIN_GPU )){
            uint g;
            for (g=0;g<gpu_pc_num_gpus;g++){
                //debug("Comparing utilization for GPU %d : %lu vs %lu",g,current_util[DOMAIN_GPU][g],prev_util[DOMAIN_GPU][g]);
                if (util_changed(current_util[DOMAIN_GPU][g],prev_util[DOMAIN_GPU][g])){ 
                    ret =1;
                    dom_changed[i]=1;
                }
            }
        }
#endif
    }
    return ret;
}

/* These two functions monitors the system utilization : CPU and GPU */
#if USE_GPUS
static state_t util_detect_main(void *p)
{

    int i;
    float min,vmin,xvmin;
    uint runnable,total,lastpid;

    util_period += CHECK_STATUS_PERIOD;
    if ((pmgt_powercap_status_per_domain(CHECK_ASK_DEFAULT) == 0) && ((util_period % UTIL_BURST_PERIOD) == 0)){
        util_period = 0;
        loadavg(&min,&vmin,&xvmin,&runnable,&total,&lastpid);
        //debug("%sCPU load %.2f%s ",COL_MGT,min,COL_CLR);
        /* DOMAIN_NODE and DOMAIN_CPU are evaluated as a whole, GPU is evaluated in more detail */
        dom_util[DOMAIN_NODE] = min;
        prev_util[DOMAIN_CPU][0] = current_util[DOMAIN_CPU][0];
        current_util[DOMAIN_CPU][0] = min;
        dom_util[DOMAIN_CPU] = min;
        /* It's pending to define Utilization per CPU */
        for (i=0;i<pc_topology_info.cpu_count;i++){ 
            prev_util[DOMAIN_CPU][i] = current_util[DOMAIN_CPU][i];
            current_util[DOMAIN_CPU][i] = min;
        }

        dom_util[DOMAIN_DRAM] = pc_topology_info.socket_count;
        dom_util[DOMAIN_GPU]=0;
        if (gpu_read_raw(no_ctx, gpu_detection_raw_data) != EAR_SUCCESS){
            error("Error reading GPU data in powercap");
        }
        for (i=0;i<gpu_pc_num_gpus;i++){
            // dom_util[DOMAIN_GPU] += gpu_detection_raw_data[i].util_gpu;
            /* Using the utilization is too fine grain */
            dom_util[DOMAIN_GPU] += (gpu_detection_raw_data[i].working > 0);
            prev_util[DOMAIN_GPU][i] = current_util[DOMAIN_GPU][i];
            current_util[DOMAIN_GPU][i]=gpu_detection_raw_data[i].util_gpu;

        }
#if SHOW_DEBUGS
        float ratio;
        for (i=0;i<NUM_DOMAINS;i++){
            ratio = (float)dom_util[i]/(float)dom_util_limit[i];
            //debug("%sDomain %d perc_use %.2f (%.2f/%.1f)%s",COL_RED,i,ratio,(float)dom_util[i],(float)dom_util_limit[i],COL_CLR);
        }
#endif
        pmgt_powercap_status_per_domain(CHECK_POWER_RED);
        if (pmgt_utilization_changed()){
            /* Internal reallocation */
            pmgt_powercap_node_reallocation();
        }
        memcpy(last_dom_util,dom_util,sizeof(ulong)*NUM_DOMAINS);
    }
    return EAR_SUCCESS;
}

static state_t util_detect_init(void *p)
{
    float min,vmin,xvmin;
    uint runnable,total,lastpid;
    loadavg(&min,&vmin,&xvmin,&runnable,&total,&lastpid);

    dom_util_limit[DOMAIN_NODE] = pc_topology_info.core_count;
    dom_util_limit[DOMAIN_CPU] = pc_topology_info.core_count;
    dom_util_limit[DOMAIN_DRAM] = pc_topology_info.socket_count ;
    dom_util_limit[DOMAIN_GPU] =  gpu_pc_num_gpus;
    last_dom_util[DOMAIN_NODE] = min;
    last_dom_util[DOMAIN_CPU] = min;
    last_dom_util[DOMAIN_DRAM] = pc_topology_info.socket_count;
    last_dom_util[DOMAIN_GPU]=0;
    if (gpu_data_alloc(&gpu_detection_raw_data)!=EAR_SUCCESS){
        error("Initializing GPU data in powercap");
    }
    return EAR_SUCCESS;
}
#endif

/* This function initilices the utilization monitoring */
void util_monitoring_init()
{
#if USE_GPUS
    if (gpu_pc_model != API_DUMMY) {
        sus_util_detection=suscription();
        sus_util_detection->call_main = util_detect_main;
        sus_util_detection->call_init = util_detect_init;
        sus_util_detection->time_relax = CHECK_STATUS_PERIOD;
        sus_util_detection->time_burst = CHECK_STATUS_PERIOD;
        sus_util_detection->suscribe(sus_util_detection);
    }
#endif
}

/* This function will load any plugin , detect components etc . It must me executed just once */
state_t pmgt_init()
{
    state_t ret;
    int i;
    policy_conf_t *my_policy;
    char basic_path[SZ_PATH];
    topology_init(&pc_topology_info);
#if USE_GPUS
    gpu_load(NO_EARD);
    gpu_get_api(&gpu_pc_model);
    if (gpu_init(no_ctx)!=EAR_SUCCESS){
        error("Initializing GPU in powercap");
    }
    if (gpu_count_devices(no_ctx, &gpu_pc_num_gpus)!=EAR_SUCCESS){
        error("Getting num gpus");
    }
    if (gpu_pc_model == API_DUMMY) {
        memcpy(pdomains_def,pdomains_def_dummy,sizeof(pdomains_def));
        memcpy(pdomains,pdomains_def_dummy,sizeof(pdomains_def));
        memcpy(pdomains_idle,pdomains_def_dummy,sizeof(pdomains_def));
        memcpy(pdomains_min,pdomains_min_dummy,sizeof(pdomains_def));
        debug("Setting the percentage of powercap to GPUS to 0 because DUMMY");
    }
#endif
    my_policy = get_my_policy_conf(my_node_conf,my_cluster_conf.default_policy);
    pmgt_idle_def_freq = frequency_pstate_to_freq(my_policy->p_state);

    /* DOMAIN_NODE is not cmpatible with CPU+DRAM */
    /* DOMAIN_NODE */
    char *obj_path = ear_getenv("EAR_POWERCAP_POLICY_NODE");
    if (obj_path==NULL){
        /* Plugin per domain node defined */
        if (strcmp(DEFAULT_PC_PLUGIN_NAME_NODE,"noplugin")){
            sprintf(basic_path,"%s/powercap/%s.so",my_cluster_conf.install.dir_plug,DEFAULT_PC_PLUGIN_NAME_NODE);
            obj_path=basic_path;
        }
    }
    if (obj_path!=NULL){
        verbose(VEARD_PC, "Loading %s powercap plugin domain node",obj_path);
        ret=symplug_open(obj_path, (void **)&pcsyms_fun[DOMAIN_NODE], pcsyms_names, pcsyms_n);
        if (ret==EAR_SUCCESS) domains_loaded[DOMAIN_NODE]=1;
    }else{
        verbose(VEARD_PC, "DOMAIN_NODE not loaded");
    }
    /* We have a single domain node */
    debug("Initialzing Node util");
    current_util[DOMAIN_NODE]=calloc(1,sizeof(ulong));
    if (current_util[DOMAIN_NODE]!=NULL) current_util[DOMAIN_NODE][0]=100;
    prev_util[DOMAIN_NODE]=calloc(1,sizeof(ulong));
    if (prev_util[DOMAIN_NODE]!=NULL) prev_util[DOMAIN_NODE][0]=100;
    debug("Static NODE utilization set to 100");
    if (!domains_loaded[DOMAIN_NODE]){
        /* DOMAIN_CPU */
        obj_path = ear_getenv("EAR_POWERCAP_POLICY_CPU"); //environment variable takes priority
        if (obj_path == NULL){ //if envar is not defined, we try with the ear.conf var
            obj_path = my_node_conf->powercap_plugin;
            if (obj_path != NULL && strlen(obj_path) > 1) {
                sprintf(basic_path,"%s/powercap/%s",my_cluster_conf.install.dir_plug,obj_path);
                obj_path=basic_path;
            }
            /* Plugin per domain node defined */
            else if (strcmp(DEFAULT_PC_PLUGIN_NAME_CPU,"noplugin")){
                sprintf(basic_path,"%s/powercap/%s.so",my_cluster_conf.install.dir_plug,DEFAULT_PC_PLUGIN_NAME_CPU);
                obj_path=basic_path;
            }
        } 
        if (obj_path!=NULL){
            verbose(VEARD_PC, "Loading %s powercap plugin domain cpu",obj_path);
            ret=symplug_open(obj_path, (void **)&pcsyms_fun[DOMAIN_CPU], pcsyms_names, pcsyms_n);
            if (ret==EAR_SUCCESS){ 
                domains_loaded[DOMAIN_CPU]=1;
                debug("CPU plugin correctly loaded");
            }else{
                error("Domain CPU not loaded");
            }
        }else{
            debug("DOMAIN_CPU not loaded");
        }
        debug("Initialzing CPU util");
        current_util[DOMAIN_CPU]=calloc(pc_topology_info.cpu_count,sizeof(ulong));
        for (i=0;i<pc_topology_info.cpu_count;i++) current_util[DOMAIN_CPU][i]=100;
        prev_util[DOMAIN_CPU]=calloc(pc_topology_info.cpu_count,sizeof(ulong));
        for (i=0;i<pc_topology_info.cpu_count;i++) prev_util[DOMAIN_CPU][i]=100;
        debug("Static CPU utilization set to 100 for %d cpus",pc_topology_info.cpu_count);

        /* DOMAIN_DRAM */
        obj_path = ear_getenv("EAR_POWERCAP_POLICY_DRAM");
        if (obj_path==NULL){
            /* Plugin per domain node defined */
            if (strcmp(DEFAULT_PC_PLUGIN_NAME_DRAM,"noplugin")){
                sprintf(basic_path,"%s/powercap/%s.so",my_cluster_conf.install.dir_plug,DEFAULT_PC_PLUGIN_NAME_DRAM);
                obj_path=basic_path;
            }
        } 
        if (obj_path!=NULL){
            verbose(VEARD_PC, "Loading %s powercap plugin domain dram",obj_path);
            ret=symplug_open(obj_path, (void **)&pcsyms_fun[DOMAIN_DRAM], pcsyms_names, pcsyms_n);
            if (ret==EAR_SUCCESS) domains_loaded[DOMAIN_DRAM]=1;
        }else{
            debug("DOMAIN_DRAM not loaded");
        }
    }
    debug("Initialzing DRAM util");
    current_util[DOMAIN_DRAM]=calloc(pc_topology_info.socket_count,sizeof(ulong));
    for (i=0;i<pc_topology_info.socket_count;i++) current_util[DOMAIN_DRAM][i]=100;
    prev_util[DOMAIN_DRAM]=calloc(pc_topology_info.socket_count,sizeof(ulong));
    for (i=0;i<pc_topology_info.socket_count;i++) prev_util[DOMAIN_DRAM][i]=100;
    debug("Static DRAM utilization set to 100 for %d sockets",pc_topology_info.socket_count);

#if USE_GPUS
    /* DOMAIN_GPU */
    obj_path = ear_getenv("EAR_POWERCAP_POLICY_GPU");
    if (gpu_pc_model != API_DUMMY) {
        if (obj_path==NULL){
            obj_path = my_node_conf->powercap_gpu_plugin;
            if (obj_path != NULL && strlen(obj_path) > 1) {
                sprintf(basic_path,"%s/powercap/%s",my_cluster_conf.install.dir_plug,obj_path);
                obj_path=basic_path;
            }
            /* Plugin per domain node defined */
            else if (strcmp(DEFAULT_PC_PLUGIN_NAME_GPU,"noplugin")){
                sprintf(basic_path,"%s/powercap/%s.so",my_cluster_conf.install.dir_plug,DEFAULT_PC_PLUGIN_NAME_GPU);
                obj_path=basic_path;
            }
        } 
        if (obj_path!=NULL){
            verbose(VEARD_PC, "Loading %s powercap plugin domain gpu",obj_path);
            ret=symplug_open(obj_path, (void **)&pcsyms_fun[DOMAIN_GPU], pcsyms_names, pcsyms_n);
            if (ret==EAR_SUCCESS) domains_loaded[DOMAIN_GPU]=1;
            else verbose(VEARD_PC, "Error loading GPU powercap plugin");
        }else{
            debug("DOMAIN_GPU not loaded");
            memset(&pcsyms_fun[DOMAIN_GPU], 0, sizeof(powercapsym_t));
            memcpy(pdomains_def,pdomains_def_dummy,sizeof(pdomains_def));
            memcpy(pdomains,pdomains_def_dummy,sizeof(pdomains_def));
            memcpy(pdomains_idle,pdomains_def_dummy,sizeof(pdomains_def));
            memcpy(pdomains_min,pdomains_min_dummy,sizeof(pdomains_def));
        } 
    } else {
        debug("DOMAIN_GPU not loaded");
        memset(&pcsyms_fun[DOMAIN_GPU], 0, sizeof(powercapsym_t));
        memcpy(pdomains_def,pdomains_def_dummy,sizeof(pdomains_def));
        memcpy(pdomains,pdomains_def_dummy,sizeof(pdomains_def));
        memcpy(pdomains_idle,pdomains_def_dummy,sizeof(pdomains_def));
        memcpy(pdomains_min,pdomains_min_dummy,sizeof(pdomains_def));
    }
    debug("Initialzing GPU util");
    if (domains_loaded[DOMAIN_GPU]){
        current_util[DOMAIN_GPU]=calloc(gpu_pc_num_gpus,sizeof(ulong));
        for (i=0;i<gpu_pc_num_gpus;i++) current_util[DOMAIN_GPUS][i]=0;
        prev_util[DOMAIN_GPU]=calloc(gpu_pc_num_gpus,sizeof(ulong));
        for (i=0;i<gpu_pc_num_gpus;i++) prev_util[DOMAIN_GPUS][i]=0;
        debug("Static GPU utilization set to 100 for %d GPUS",gpu_pc_num_gpus);
    }
    if (domains_loaded[DOMAIN_NODE]  || domains_loaded[DOMAIN_CPU] || domains_loaded[DOMAIN_DRAM] || domains_loaded[DOMAIN_GPU]) ret=EAR_SUCCESS;
    else ret=EAR_ERROR;
#else
    if (domains_loaded[DOMAIN_NODE]  || domains_loaded[DOMAIN_CPU] || domains_loaded[DOMAIN_DRAM]) ret=EAR_SUCCESS;
    else ret=EAR_ERROR;
#endif
    debug("Initializing Util monitoring");
    util_monitoring_init();
    memset(cdomain_status,0,NUM_DOMAINS*sizeof(domain_status_t));
    memset(&cdomain_settings, 0, sizeof(domain_settings_t)*NUM_DOMAINS);
    for (i = 0; i<NUM_DOMAINS; i++) {
        if (domains_loaded[i]) {
            freturn(pcsyms_fun[i].plugin_get_settings, &cdomain_settings[i]);
            security_range -= cdomain_settings[i].security_range; //security_range starts at 100 and every domain takes a bit from it
        }
    }
    compute_power_distribution(); //power distribution can only be computed AFTER we have all the domain's settings (for now only CPU matters)
    debug("Returng from pmgt_init with %d (%d = SUCCESS, %d ERROR)", ret, EAR_SUCCESS, EAR_ERROR);
    return ret;
}

state_t pmgt_enable(pwr_mgt_t *phandler)
{
    state_t ret,gret=EAR_SUCCESS;
    int i;
    for (i=0;i<NUM_DOMAINS;i++){
        if (pcsyms_fun[i].set_verb_channel!=NULL) pcsyms_fun[i].set_verb_channel(verb_channel);
    }
    for (i=0;i<NUM_DOMAINS;i++){
        debug("Enabled domain %i , loaded %d",i,domains_loaded[i]);
        if (domains_loaded[i]){
            sus[i]=suscription();
            debug("Enabling domain %d",i);
            ret=freturn(pcsyms_fun[i].enable,sus[i]);
            if ((ret!=EAR_SUCCESS) && (domains_loaded[i])) gret=ret;
        }
    }
    if (gret!=EAR_SUCCESS) return gret;
    return gret;
}
state_t pmgt_disable(pwr_mgt_t *phandler)
{
    state_t ret;
    int i;
    for (i=0;i<NUM_DOMAINS;i++){
        ret=freturn(pcsyms_fun[i].disable);
    }
    return ret;
}
state_t pmgt_handler_alloc(pwr_mgt_t **phandler)
{
    *phandler=(pwr_mgt_t*)calloc(1,sizeof(pwr_mgt_t *));
    if (*phandler!=NULL) return EAR_SUCCESS;
    return EAR_ERROR;
}

state_t pmgt_set_powercap_value(pwr_mgt_t *phandler,uint pid,uint domain,ulong limit)
{
    state_t ret,gret=EAR_SUCCESS;
    uint value;
    int cc;
    int i;
    debug("entering set_powercap_value with domaind %u and limit %lu", domain, limit);
    for (i=0;i<NUM_DOMAINS;i++){
        value = ear_max(limit*pdomains[i], 1);
        debug("Using %f weigth for domain %d: Total %lu allocated %u",pdomains[i],i,limit,value);
        ret = freturn(pcsyms_fun[i].set_powercap_value, pid, domain, value, current_util[i]);
#ifndef SYN_TEST
        for (cc = 0;cc <= max_context_created; cc++){
            if (current_ear_app[cc] != NULL){
                /* PENDING to distribute power among running jobs proportionally to the number of resources*/
                current_ear_app[cc]->settings->pc_opt.pper_domain[i] = limit*pdomains[i]/num_contexts;
            }
        }
#endif
        if ((ret!=EAR_SUCCESS) && (domains_loaded[i])) gret=ret;
        if (value == 1) {
            ret = freturn(pcsyms_fun[i].plugin_set_relax);
        } else {
            ret = freturn(pcsyms_fun[i].plugin_set_burst);
        }
    }
    pmgt_limit=limit;
    return gret;
}

static void reallocate_power_between_domains()
{

    if (pmgt_limit == POWER_CAP_UNLIMITED) return;
    int i;
    char buffer[1024],buffer2[1024];
    uint pid=0, domain=0; // we must manage this
    state_t ret;
    strcpy(buffer2,"");
    for (i=0;i<NUM_DOMAINS;i++){
        sprintf(buffer,"[DOM=%d W=%.2f Allocated %.1f]",i,pdomains[i],pmgt_limit*pdomains[i]);
        strcat(buffer2,buffer);
        ret=freturn(pcsyms_fun[i].set_powercap_value,pid,domain,pmgt_limit*pdomains[i],current_util[i]);
        if (ret == EAR_ERROR) debug("error when setting powercap value");
    }
    debug("%s",buffer2);
}

state_t pmgt_reset_pdomain()
{
    debug("Doing a reset of powercap limits per domain");
    reallocate_power_between_domains();

    return EAR_SUCCESS;
}

state_t pmgt_get_powercap_value(pwr_mgt_t *phandler,uint pid,ulong *powercap)
{
    state_t ret,gret;
    uint parc;
    int i;
    if (powercap == NULL) return EAR_ERROR;
    for (i=0;i<NUM_DOMAINS;i++){
        powercap[i] = 0;
        ret=freturn(pcsyms_fun[i].get_powercap_value,pid,&parc);
        if (domains_loaded[i]) powercap[i] = (ulong) parc;
        if ((ret!=EAR_SUCCESS) && (domains_loaded[i])) gret=ret;
    }
    return gret;
}

uint pmgt_is_powercap_enabled(pwr_mgt_t *phandler,uint pid)
{
    uint ret=0,gret=0;
    int i;
    for (i=0;i<NUM_DOMAINS;i++){
        ret=freturn(pcsyms_fun[i].is_powercap_policy_enabled,pid);
        if (domains_loaded[i]) gret+=ret;
    }
    return gret;
}
void pmgt_print_powercap_value(pwr_mgt_t *phandler,int fd)
{
    int i;
    if (pmgt_limit == POWER_CAP_UNLIMITED){
        dprintf(fd, "Node powercap unlimited");
        return;
    }  
    for (i=0;i<NUM_DOMAINS;i++){
        if (domains_loaded[i]) freturn(pcsyms_fun[i].print_powercap_value,fd);
    }
}
void pmgt_powercap_to_str(pwr_mgt_t *phandler,char *b)
{
    int i;
    char new_s[512];
    if (pmgt_limit == POWER_CAP_UNLIMITED){
        sprintf(b, "Node powercap unlimited");
        return;
    }

    for (i=0;i<NUM_DOMAINS;i++){
        if (domains_loaded[i]){ 
            freturn(pcsyms_fun[i].powercap_to_str,new_s);
            strcat(b,new_s);
        }
    }
}

void pmgt_set_status(pwr_mgt_t *phandler,uint status)
{
    debug("pmgt_set_status");
    int i;
    for (i=0;i<NUM_DOMAINS;i++){
        if (domains_loaded[i]) freturn(pcsyms_fun[i].set_status,status);
    }
}

uint pmgt_get_powercap_cpu_strategy(pwr_mgt_t *phandler)
{
    uint ret,gret=PC_POWER;
    debug("pmgt_cpu_strategy");
    if (domains_loaded[DOMAIN_CPU]){
        ret=freturn(pcsyms_fun[DOMAIN_CPU].get_powercap_strategy);
        if (ret==PC_DVFS) gret=ret;    
    }
    return gret;
}

uint pmgt_get_powercap_gpu_strategy(pwr_mgt_t *phandler)
{
    uint ret,gret=PC_POWER;
    debug("pmgt_gpu_strategy");
    if (domains_loaded[DOMAIN_GPU]){
        ret=freturn(pcsyms_fun[DOMAIN_GPU].get_powercap_strategy);
        if (ret==PC_DVFS) gret=ret;    
    }
    return gret;
}

void pmgt_set_pc_mode(pwr_mgt_t *phandler,uint mode)
{
    int i;
    for (i=0;i<NUM_DOMAINS;i++){
        freturn(pcsyms_fun[i].set_pc_mode,mode);
    }
}

void pmgt_set_power_per_domain(pwr_mgt_t *phandler,dom_power_t *pdomain,uint st)
{
    int i;
    if (pmgt_limit == POWER_CAP_UNLIMITED) return;
#if USE_GPUS && SHOW_DEBUGS
    float pnode,pcpu,pdram,pgpu = 0.0;
    pnode=(float)(pdomain->platform-pdomain->gpu)/(float)pdomain->platform;
    pcpu=(float)(pdomain->cpu+pdomain->dram)/(float)pdomain->platform;
    pdram=(float)(pdomain->dram)/(float)pdomain->platform;
    pgpu=(float)(pdomain->gpu)/(float)pdomain->platform;
    debug("[NODE-GPU %f CPU %f DRAM %f GPU %f][NODE-GPU %f CPU %f DRAM %f GPU %f]",pnode,pcpu,pdram,pgpu,pdomains[DOMAIN_NODE],pdomains[DOMAIN_CPU],pdomains[DOMAIN_DRAM],pdomains[DOMAIN_GPU]);
#endif
    if (my_pc_opt.last_t1_allocated < my_pc_opt.def_powercap) return;
    if (pdomain->platform > pmgt_limit){
        verbose(VEARD_PC,"%s Warning! we are over the limit: Limit %u Power %u%s",COL_RED,pmgt_limit,pdomain->platform,COL_CLR);
        float excess = ((float)(pdomain->platform - pmgt_limit))/(float)pmgt_limit;
        for (i=0;i< NUM_DOMAINS; i++){
            if (pdomains[i] > 0)
                pdomains[i] = ear_max(pdomains_min[i], pdomains[i] - excess*pdomains[i]);
        }
        debug("reduced power domains by %.2f, new CPU %.2f new GPU %.2f", excess, pdomains[DOMAIN_CPU], pdomains[DOMAIN_GPU]);
        pmgt_reset_pdomain();
    }else if (pdomain->platform < (pmgt_limit * security_range)){  //check if we can comfortably give power back to the domains without risking an excess of power use   
        for (i=0;i< NUM_DOMAINS; i++){
            pdomains[i] = ear_min(pdomains[i] + 0.02,pdomains_def[i]);
        }
        debug("increase power domains new CPU %.2f new GPU %.2f", pdomains[DOMAIN_CPU], pdomains[DOMAIN_GPU]);
        pmgt_reset_pdomain();
    }
}

void pmgt_get_app_req_freq(uint domain, ulong *f, uint dom_size)
{
    int cc, i;
    powermon_app_t *pmapp;	
    job_context_t *alloc;
    int pos_job_in_node;
    if (domain == DOMAIN_CPU) for (i = 0; i < MAX_CPUS_SUPPORTED; i++) f[i] = 0;
#if USE_GPUS
    if (domain == DOMAIN_GPU) for (i = 0; i < MAX_GPUS_SUPPORTED; i++) f[i] = 0;
#endif
    for (cc = 1; cc  <= max_context_created; cc ++){
        if (current_ear_app[cc] != NULL){
            pmapp = current_ear_app[cc];
            /* For each app, we must merge CPUfreq and GPUfreq requested */
#ifndef SYN_TEST
            if (domain == DOMAIN_CPU) {
                //memcpy(f,pc_app_info_data->req_f,sizeof(ulong)*MAX_CPUS_SUPPORTED);
                cpu_set_t m;
                pos_job_in_node = is_job_in_node(pmapp->app.job.id, &alloc);
                if (pmapp->app.is_mpi) {  //job step with EARL
                    debug("Using EARL mask");
                    m = pmapp->app_info->node_mask;
                } else if (!pmapp->is_job) { //job step without EARL
                    debug("Using plugin mask (step without EARL)");
                    m = pmapp->plug_mask;
                } else if (pmapp->is_job && pos_job_in_node && alloc->num_ctx == 1) { //srun without an sbatch
                    debug("Using plugin mask (step without EARL neither sbatch)");
                    m = pmapp->plug_mask;
                } else { //sbatch with other steps inside
                    continue;
                }
                //verbose_affinity_mask(&m, dom_size);


                for (i=0; i < MAX_CPUS_SUPPORTED; i++){
                    if (CPU_ISSET(i,&m)) f[i] = pmapp->pc_app_info->req_f[i];
                }
            }
#if USE_GPUS
            if (domain == DOMAIN_GPU){ 
                //memcpy(f,pc_app_info_data->req_gpu_f,sizeof(ulong)*MAX_GPUS_SUPPORTED);
                for (i=0; i < MAX_GPUS_SUPPORTED; i++){
                    if (pmapp->pc_app_info->req_gpu_f[i] != 0) f[i] = pmapp->pc_app_info->req_gpu_f[i];
                }
            }
#endif
#endif
        }

    }
#define USE_IDLE_PC_MINFREQ 1
    if (domain == DOMAIN_CPU){
        for (i=0; i < MAX_CPUS_SUPPORTED; i++){
#if USE_IDLE_PC_MINFREQ
            if (!f[i]) f[i] = frequency_pstate_to_freq(frequency_get_num_pstates() -1 );
#else
            if (!f[i]) f[i] = pmgt_idle_def_freq;
#endif
        }
    }
}

void pmgt_set_app_req_freq(pwr_mgt_t *phandler)
{
    ulong cpuf[MAX_CPUS_SUPPORTED];
    memset(cpuf, 0, sizeof(ulong)*MAX_CPUS_SUPPORTED);
    if (pcsyms_fun[DOMAIN_NODE].set_app_req_freq != NULL){
        pmgt_get_app_req_freq(DOMAIN_NODE, cpuf, MAX_CPUS_SUPPORTED);
        freturn(pcsyms_fun[DOMAIN_NODE].set_app_req_freq,cpuf);
    }
    if (pcsyms_fun[DOMAIN_CPU].set_app_req_freq != NULL){
        pmgt_get_app_req_freq(DOMAIN_CPU, cpuf, MAX_CPUS_SUPPORTED);
        freturn(pcsyms_fun[DOMAIN_CPU].set_app_req_freq,cpuf);
    }
#if USE_GPUS
    if (gpu_pc_model != API_DUMMY) {
        ulong gpuf[MAX_GPUS_SUPPORTED];
        memset(gpuf, 0, sizeof(ulong)*MAX_GPUS_SUPPORTED);
        if (pcsyms_fun[DOMAIN_GPU].set_app_req_freq != NULL) {
            pmgt_get_app_req_freq(DOMAIN_GPU, gpuf, MAX_GPUS_SUPPORTED);
            freturn(pcsyms_fun[DOMAIN_GPU].set_app_req_freq,gpuf);
        }
    }
#endif
}


void pmgt_set_default_powercap()
{
    int i;
    state_t ret;
    /* Calls the set_default functions for plugins */
    for (i = 0; i < NUM_DOMAINS; i++) {
        ret = freturn(pcsyms_fun[i].reset_powercap_value);
        if (ret == EAR_ERROR) debug("error when setting powercap value");
    }
}

/* Power needs some reallocation internally in components */
void pmgt_powercap_node_reallocation()
{
    int i;

#if SHOW_DEBUGS
    float ratio;
    for (i=0; i< NUM_DOMAINS;i++){
        ratio = (float)dom_util[i]/(float)dom_util_limit[i];
        debug("%sDomain %d perc_use %.2f (%.2f/%.1f)%s",COL_RED,i,ratio,(float)dom_util[i],(float)dom_util_limit[i],COL_CLR);
    }
#endif
    for (i=0; i< NUM_DOMAINS;i++){
        if (dom_changed[i]){
            freturn(pcsyms_fun[i].set_new_utilization,current_util[i]);
            dom_changed[i] = 0;
        }
    }

}

void pmgt_new_job(pwr_mgt_t *phandler)
{
#if USE_GPUS
    monitor_burst(sus_util_detection, 0);
#endif
    memcpy(pdomains,pdomains_def,sizeof(float)*NUM_DOMAINS);
    reallocate_power_between_domains();
}

void pmgt_end_job(pwr_mgt_t *phandler)
{
    memcpy(pdomains,pdomains_def,sizeof(float)*NUM_DOMAINS);
    reallocate_power_between_domains();
#if USE_GPUS
    monitor_relax(sus_util_detection);
#endif
}
void pmgt_idle_to_run(pwr_mgt_t *phandler)
{
    memcpy(pdomains,pdomains_def,sizeof(float)*NUM_DOMAINS);
    reallocate_power_between_domains();
    debug("sending PC_STATUS_RUN to all domain");
    pmgt_set_status(phandler,PC_STATUS_RUN);
}
void pmgt_run_to_idle(pwr_mgt_t *phandler)
{
    memcpy(pdomains,pdomains_idle,sizeof(float)*NUM_DOMAINS);
    reallocate_power_between_domains();
    pmgt_set_status(phandler,PC_STATUS_IDLE);
}

void pmgt_get_status(pmgt_status_t *status)
{
    int i;
    uint ret;

    memset(cdomain_status, 0, sizeof(domain_status_t)*NUM_DOMAINS);
    memset(status, 0, sizeof(pmgt_status_t));
    if (pmgt_limit == POWER_CAP_UNLIMITED) {
        status->status = PC_STATUS_OK;
        return;
    }
    /* We ask each domain its curent status to compute the node's status */
    for (i=0; i< NUM_DOMAINS;i++) {
        if (pcsyms_fun[i].get_powercap_status != NULL){
            ret=pcsyms_fun[i].get_powercap_status(&cdomain_status[i]);
            if (ret == EAR_ERROR) debug("error getting domain powercap");
        }
    }

    debug("pmgt_get_status: DOMAIN_CPU: %d DOMAIN_GPU: %d", cdomain_status[DOMAIN_CPU].ok, cdomain_status[DOMAIN_GPU].ok);
    debug("pmgt_get_status: DOMAIN_CPU stress: %d", cdomain_status[DOMAIN_CPU].stress);
    debug("pmgt_get_status: DOMAIN_GPU stress: %d", cdomain_status[DOMAIN_GPU].stress);
    if (cdomain_status[DOMAIN_CPU].ok == PC_STATUS_OK && cdomain_status[DOMAIN_GPU].ok == PC_STATUS_OK) { //OK + OK
        status->status = PC_STATUS_OK;
        debug("pmgt_get_status: OK + OK");
    } else if (cdomain_status[DOMAIN_CPU].ok == PC_STATUS_GREEDY || cdomain_status[DOMAIN_GPU].ok == PC_STATUS_GREEDY) {
        if (cdomain_status[DOMAIN_CPU].ok == PC_STATUS_RELEASE || cdomain_status[DOMAIN_GPU].ok == PC_STATUS_RELEASE) { //if we are GREDDY + RELEASE, we can still rebalance so we are ok
            status->status = PC_STATUS_OK;
            debug("pmgt_get_status: GREEDY + RELEASE");
        } else { // GREEDY + GREEDY or GREEDY + OK
            status->status = PC_STATUS_GREEDY;
            status->requested += cdomain_status[DOMAIN_CPU].requested; 
            status->requested += cdomain_status[DOMAIN_GPU].requested;
            status->requested = ear_min(status->requested, 256); // a total maximum step of 256 since it is the max value we can pass up
            status->stress = ear_max(cdomain_status[DOMAIN_CPU].stress, cdomain_status[DOMAIN_GPU].stress);
            debug("pmgt_get_status: GREEDY + OK/GREEDY Node requested power %u",status->requested);
        }
    } else if (cdomain_status[DOMAIN_CPU].ok == PC_STATUS_RELEASE || cdomain_status[DOMAIN_GPU].ok == PC_STATUS_RELEASE) { // no need to check for greedy since we did that before, this includes OK+RELEASE
        status->status = PC_STATUS_RELEASE;
        for (i = 0; i < NUM_DOMAINS; i++)
            status->tbr += cdomain_status[i].exceed;
    }
    debug("pmgt_get_status: Final pmgt_status: %d", status->status);

}

uint pmgt_powercap_status_per_domain(uint action)
{
#if USE_GPUS
    int i;
    char is_release = 0;
    int totalok, totalrel, totalask;
    totalok = totalrel = totalask = 0;
    uint ret,from = 0,to = 0;
    memset(cdomain_status,0,sizeof(domain_status_t)*NUM_DOMAINS);
    if (pmgt_limit == POWER_CAP_UNLIMITED) return 0;
    /* We ask each domain its curent status to distribute power accross domains */
    for (i=0; i< NUM_DOMAINS;i++){
        if (pcsyms_fun[i].get_powercap_status != NULL){
            ret=pcsyms_fun[i].get_powercap_status(&cdomain_status[i]);
            /* status = 0 when it's ok and 1 when it can release power */
            totalok += (cdomain_status[i].ok == PC_STATUS_OK);
            totalrel += (cdomain_status[i].ok == PC_STATUS_RELEASE);
            totalask += (cdomain_status[i].ok == PC_STATUS_ASK_DEF);
        }
    }
    debug("GPU has stress %u and CPU has %u. Status GPU %u CPU %u. ask %u ok %u rel %u", 
            cdomain_status[DOMAIN_GPU].stress, cdomain_status[DOMAIN_CPU].stress, cdomain_status[DOMAIN_GPU].ok,cdomain_status[DOMAIN_CPU].ok,totalask,totalok,totalrel);
    if ((action == CHECK_ASK_DEFAULT) && (totalask > 0)){
        debug("action is CHECK_ASK_DEFAULT, not power redistribution");
        /* Some module needs an urgent power reallocation */
        if (totalask > 0){ 
            pmgt_set_default_powercap();
            return 1;
        }
        return 0;
    }
    /* If both are OK or both can release power there is nothing to do between modules */
    if (totalrel == 2) return 1;
    /* We check first the CPU */
    if ((cdomain_status[DOMAIN_CPU].ok == PC_STATUS_GREEDY) && (cdomain_status[DOMAIN_GPU].ok == PC_STATUS_RELEASE)) {
        /** Add N watts to the CPU */
        from = DOMAIN_GPU;
        to = DOMAIN_CPU;
        is_release = 1;
    }else if ((cdomain_status[DOMAIN_GPU].ok == PC_STATUS_GREEDY) && (cdomain_status[DOMAIN_CPU].ok == PC_STATUS_RELEASE)){
        from = DOMAIN_CPU;
        to = DOMAIN_GPU;
        is_release = 1;
    }else if ((cdomain_status[DOMAIN_GPU].ok == PC_STATUS_GREEDY) && (cdomain_status[DOMAIN_CPU].ok == PC_STATUS_GREEDY)){
        if (abs((int)cdomain_status[DOMAIN_GPU].stress - (int)cdomain_status[DOMAIN_CPU].stress) > 20){
            if (cdomain_status[DOMAIN_GPU].stress < cdomain_status[DOMAIN_CPU].stress){
                from = DOMAIN_GPU;
                to = DOMAIN_CPU;
            } else {
                from = DOMAIN_CPU;
                to = DOMAIN_GPU;
            }
            is_release = 1;
            debug("moving power between greedy domains");
            //min between %power and max_power_reallo
            cdomain_status[from].exceed = ear_min(cdomain_status[from].current_pc*DOMAINS_REDIST, DOMAINS_REDIST_MAX);
        } else {
            is_release = 0;
            debug("could not redistribute power, GPU has stress %u and CPU has %u", cdomain_status[DOMAIN_GPU].stress, cdomain_status[DOMAIN_CPU].stress);
        }
    }else if ((cdomain_status[DOMAIN_GPU].ok == PC_STATUS_GREEDY) && (cdomain_status[DOMAIN_CPU].ok == PC_STATUS_OK)){
        from = DOMAIN_CPU;
        to = DOMAIN_GPU;
        is_release = 2;
        debug("moving from ok CPU to greedy GPU");
        cdomain_status[from].exceed = ear_min(cdomain_status[from].current_pc*DOMAINS_REDIST, DOMAINS_REDIST_MAX);
    }else if ((cdomain_status[DOMAIN_GPU].ok == PC_STATUS_OK) && (cdomain_status[DOMAIN_CPU].ok == PC_STATUS_GREEDY)){
        from = DOMAIN_GPU;
        to = DOMAIN_CPU;
        is_release = 2;
        debug("moving from ok GPU to greedy CPU");
        cdomain_status[from].exceed = ear_min(cdomain_status[from].current_pc*DOMAINS_REDIST, DOMAINS_REDIST_MAX);
    }else return 1;

    uint new_power_send, new_power_recv;
    switch (is_release) {
        case 0:
            debug("No power redistribuition"); 
            break;
        case 1: 
            debug("%u Watts have been released from domain %u and added to domain %u",cdomain_status[from].exceed,from,to);
            ret=freturn(pcsyms_fun[from].release_powercap_allocation, cdomain_status[from].exceed);
            if (ret == EAR_ERROR) debug("error setting powercap value");
            ret=freturn(pcsyms_fun[to].increase_powercap_allocation, cdomain_status[from].exceed);
            if (ret == EAR_ERROR) debug("error setting powercap value");
            break;
        case 2:
            new_power_send = cdomain_status[from].current_pc - cdomain_status[from].exceed;
            new_power_recv = cdomain_status[to].current_pc + cdomain_status[from].exceed;
            debug("%u Watts have been moved from domain %u to domain %u. Previous values from %u to %u. New values from: %u to %u",
                    cdomain_status[from].exceed, from, to, cdomain_status[from].current_pc, cdomain_status[to].current_pc, new_power_send, new_power_recv);
            ret=freturn(pcsyms_fun[from].set_powercap_value, 0, from, new_power_send, current_util[from]); /* Receives less power than before */
            if (ret == EAR_ERROR) debug("error setting powercap value");
            ret=freturn(pcsyms_fun[to].set_powercap_value, 0, to, new_power_recv, current_util[to]); /* Receives more power that before */
            if (ret == EAR_ERROR) debug("error setting powercap value");
            break;
    }
    ulong total_power = 0;
    for (i = 0; i < NUM_DOMAINS; i++)
    {
        total_power += cdomain_status[i].current_pc;
    }
    debug("current node power %lu, current_limit %u", total_power, pmgt_limit);
    assert(total_power <= pmgt_limit);
#endif
    return 0;
}
