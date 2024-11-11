/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1


#define _GNU_SOURCE


#if !SHOW_DEBUGS
#define NDEBUG // Disable asserts
#endif

#define TEMP_VERB 0


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/mman.h>
#include <assert.h>
#include <semaphore.h>

#include <common/config.h>
#include <common/states.h>
#include <common/system/time.h>
#include <common/output/verbose.h>
#include <common/types/signature.h>
#include <common/math_operations.h>
#include <common/hardware/hardware_info.h>
#include <common/system/folder.h>

#include <library/api/clasify.h>
#include <library/api/eard_dummy.h>
#include <library/loader/module_mpi.h>
#include <library/loader/module_cuda.h>
#include <library/common/utils.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/common/library_shared_data.h>
#include <library/metrics/metrics.h>
#include <library/metrics/energy_node_lib.h>
#include <library/policies/common/mpi_stats_support.h> /* Should we move this file out out policies? */
#include <library/tracer/tracer.h>
#include <library/tracer/tracer_paraver_comp.h>
#include <metrics/io/io.h>
#include <metrics/proc/stat.h>
#include <metrics/cpi/cpi.h>
#include <metrics/cache/cache.h>
#include <metrics/flops/flops.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/temperature/temperature.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/energy_cpu/energy_cpu.h>
#include <management/cpufreq/cpufreq.h>
#include <management/imcfreq/imcfreq.h>
#include <daemon/local_api/eard_api.h>
#include <report/report.h>
#if USE_GPUS
#include <metrics/gpu/gpu.h>
#include <management/gpu/gpu.h>
#if USE_CUPTI
#include <metrics/gpu/gpuproc.h>
#endif
#if DCGMI
#include <library/metrics/dcgmi_lib.h>
#endif
#endif // USE_GPUS

#if METRICS_OVH
#include <common/utils/overhead.h>
#endif

#if DLB_SUPPORT
#include <library/metrics/dlb_talp_lib.h>
#endif

#define FAKE_POWER_READING  0
#define FAKE_ENERGY_READING 0

#define USE_PROC_PS            1

#define INFO_METRICS           2
#define IO_VERB                3
#define VERB_PROCPS 					 2
#define SET_DYN_UNC_BY_DEFAULT 1
#define MPI_VERB               3
#define GPU_VERB               2
#define VERBOSE_CUPTI          2
#define PHASES_SUMMARY_VERB    2
#define CPUPRIO_VERB           2
#define ERROR_VERB             2
#define TPI_DEBUG              4
#define ENERGY_EFF_VERB        2

#define DEF_READ_CUPTI_METRICS 0

// Registers
#define LOO         0 // loop
#define APP         1 // application
#define LOO_NODE    2 // We will share the same pos than for accumulated values
#define APP_NODE    3
#define APP_ACUM    4 // Used to store the accumulated values for metrics which also use
                      // LOO_NODE position
#define SIG_BEGIN 	0
#define SIG_END		1

#if USE_GPUS & USE_CUPTI
#define EAR_USE_CUPTI "EAR_USE_CUPTI"
#endif

#define verb_state_fail(action, vl, msg) \
	if (state_fail(action)){ \
    	verbose_master(vl,msg); \
	}


#if METRICS_OVH
uint id_ovh_partial_stop;
uint id_ovh_compute_data;
uint id_ovh_dc_energy;
uint id_ovh_rapl;
uint id_ovh_cpufreq;
uint id_ovh_imcfreq;

static uint partial_stop_cnt = 0;
static uint compute_sig_cnt = 0;
#endif
// #define MASTER(master_rank) (master_rank >= 0)

// extern configuration from the main ear library
extern cluster_conf_t cconf;
extern sem_t *lib_shared_lock_sem;

extern ear_classify_t phases_limits; // Get from ear.c
extern uint earl_config_dummy;

extern int         dispose;
// Hardware
static double      hw_cache_line_size;

// Options
static int         rapl_elements;
static ulong       node_energy_datasize;
static uint        node_energy_units;

/* These vectors have position APP for application granularity and LOO for loop granularity */
static int         num_packs = 0;
static ull        *metrics_rapl[2];  // nJ (vec)
#if MPI_OPTIMIZED
static ull        *metrics_rapl_fine_monitor_start, *metrics_rapl_fine_monitor_end, *metrics_rapl_fine_monitor_diff;
double max_app_power = 0;

/* This constant must be migrated to config_env */
#define EAR_REPORT_POWER_TRACE "EAR_REPORT_POWER_TRACE"
static uint report_power_trace = 0;

#endif // MPI_OPTIMIZED
static ull        *aux_rapl;         // nJ (vec)
static ull        *last_rapl;        // nJ (vec)
static uint        rapl_size;
static llong       aux_time;
static edata_t     aux_energy = NULL;
static edata_t     metrics_ipmi[2] = { NULL, NULL};  // mJ
static ulong       acum_ipmi[2];
static double      total_acum_time = 0;
static edata_t     aux_energy_stop = NULL;
static llong       metrics_usecs[2]; // uS


static  uint    estimate_power = 0;
static  uint    estimate_gbs = 0;
static  uint    estimate_met_cpuf = 0;

/* In the case of IO we will collect per process, per loop,
 * per node, and per-loop node accumulated. */
static io_data_t   metrics_io_init[4];
static io_data_t   metrics_io_end[4];
static io_data_t   metrics_io_diff[5]; // Another position was added to store per-loop node
                                       // accumulated, to be used at the end if some read fails
static ctx_t       ctx_io;
int ear_get_num_threads();

/* Per proc statistics using OS API */
uint proc_ps_ok = 0; 
#if USE_PROC_PS
#define metric_cond(cond, call, vlevel, msg) \
  if (cond){ \
    if (state_fail(call)){ \
    verbose_master(vlevel, msg); \
    } \
  }

static proc_stat_t metrics_proc_init[2];
static proc_stat_t metrics_proc_end[2];
static proc_stat_t metrics_proc_diff[2];
static ctx_t       proc_ctx;
#else
#define metric_cond(cond, call, vlevel, msg)
#endif


/* MPI statistics */
mpi_information_t *metrics_mpi_info[2];             // Contains the data of the period
mpi_information_t *metrics_last_mpi_info[2];
#if MPI_CALL_TYPES
mpi_calls_types_t *metrics_mpi_calls_types[2];      // Contains the data of the period
mpi_calls_types_t *metrics_last_mpi_calls_types[2];
#endif



/* These vectors have position APP for application granularity and LOO for loop granularity. */
static llong       last_elap_sec = 0;
static llong       app_start;

/* Node mgr data */
extern uint        exclusive;
extern node_mgr_sh_data_t *node_mgr_job_info;

/* New MEM FREQ mgt API */
uint        imc_devices;
pstate_t   *imc_pstates; //*
uint        imc_num_pstates; //*
ulong       def_imc_max_khz; //*
ulong       def_imc_min_khz; //*
ulong       imc_max_khz; //*
ulong       imc_min_khz; //*
uint       *imc_max_pstate; //*
uint       *imc_min_pstate; //*
uint        imc_curr_max_pstate; //*
uint        imc_curr_min_pstate; //*

/* New apis */
static imcfreq_t  *imcfreq_read1[2];
static imcfreq_t  *imcfreq_read2[2];
static ulong       imcfreq_avrg[2];
static ulong      *imcfreq_diff[2];
static llong      *temp_read1[2];
static llong      *temp_read2[2];
static llong      *temp_diff[2];
static cpufreq_t  *cpufreq_read1[2];
static cpufreq_t  *cpufreq_read2[2];
       ulong      *cpufreq_diff;
static ulong       cpufreq_avrg[2];
static bwidth_t   *bwidth_read1[2];
static bwidth_t   *bwidth_read2[2];
static bwidth_t   *bwidth_diff[2];
static ullong      bwidth_cas[2];
static flops_t     flops_read1[2];
static flops_t     flops_read2[2];
static flops_t     flops_diff[2];
static cache_t     cache_read1[2];
static cache_t     cache_read2[2];
static cache_t     cache_diff[2];
static char        cache_description[1024];
static double      cache_bwidth[2];
static cpi_t       cpi_read1[2];
static cpi_t       cpi_read2[2];
static cpi_t       cpi_diff[2];
static double      cpi_avrg[2];
#if USE_GPUS
static gpu_t      *gpu_metrics_read1[2];
static gpu_t      *gpu_metrics_read2[2];
static gpu_t      *gpu_metrics_diff[2];
static gpu_t      *gpu_metrics_busy;
static gpu_t      *gpu_metrics_busy_diff;
char        gpu_str[1024];
#endif // USE_GPUS


/* New metrics */
static metrics_t         mgt_cpufreq;
static metrics_t         mgt_imcfreq;
static metrics_t         met_temp;
static metrics_t         met_cpufreq;
static metrics_t         met_imcfreq;
static metrics_t         met_bwidth;
static metrics_t         met_energy;
static metrics_t         met_flops;
static metrics_t         met_cache;
static metrics_t         met_cpi;
static metrics_t         mgt_cpuprio;


#if USE_GPUS
static metrics_gpus_t mgt_gpu;
static metrics_gpus_t met_gpu;
#if USE_CUPTI
static uint read_cupti_metrics = DEF_READ_CUPTI_METRICS;
static metrics_gpus_t proc_gpu;
/* 0 for LOOP, 1 for APP */
static gpuproc_t     *proc_gpu_data_init[2];
static gpuproc_t     *proc_gpu_data_end[2];
static gpuproc_t     *proc_gpu_data_diff[2];
static char gpu_proc_buffer[256];
#endif
#endif // USE_GPUS

/* New status */
static int  master = -1;
static uint eard;

static double dc_power_ratio;
static uint   dc_power_ratio_instances  = 0;

/* Energy saving app event */
static ear_event_t app_energy_saving;
static ear_event_t app_phase_summary;
extern report_id_t rep_id;


extern uint last_earl_phase_classification;
extern timestamp init_last_phase, init_last_comp;
extern uint policy_cpu_bound ;
extern uint policy_mem_bound ;

uint validate_cpu_busy = EARL_CPUBUSYWAIT_VALIDATE;


/** Compute the total node I/O data. */
static state_t metrics_compute_total_io(io_data_t *total);


/** This function is called before any API initialization. It's used to read envorironment
 * variables that enable/disable the load of some APIs. By now, these are CUPTI. */
static void read_metrics_options();


/** Loads the energy plug-in. */
static state_t energy_lib_init(settings_conf_t *conf);

/* Overhead evaluation */
static uint validation_new_cpu = 0;
static uint validation_new_gpu = 0;

/** Private function to perfom last operations before returning from dispose call. */
static void metrics_static_dispose();


const metrics_t *metrics_get(uint api)
{
    switch(api) {
        case MGT_CPUFREQ: return &mgt_cpufreq;
        case MGT_IMCFREQ: return &mgt_imcfreq;
        case MET_TEMP   : return &met_temp;
        case MET_CPUFREQ: return &met_cpufreq;
        case MET_IMCFREQ: return &met_imcfreq;
        case MET_BWIDTH:  return &met_bwidth;
        case MET_FLOPS:   return &met_flops;
        case MET_CACHE:   return &met_cache;
        case MET_CPI:     return &met_cpi;
        case API_ISST:    return &mgt_cpuprio;
        default: return NULL;
    }
}

#if USE_GPUS
const metrics_gpus_t *metrics_gpus_get(uint api)
{
    switch(api) {
        case MGT_GPU:     return &mgt_gpu;
        case MET_GPU:     return &met_gpu;
        default: return NULL;
    }
}
#endif // USE_GPUS

uint metrics_CPU_phase_ok()
{
    if (!met_cpi.ok || !met_bwidth.ok || !met_flops.ok) return 0;
    return 1;
}


static void metrics_configuration(topology_t *tp)
{
	char topo_str[1024];
	cfb_t bf;

	if (!EARL_LIGHT) return;

	if (eards_connected())
	{
		verbose_info2_master("EARD connected.");
	} else
	{
		verbose_warning_master("EARD not connected. Switch to monitoring ?");
	}

	if (!mgt_cpufreq.ok)
	{
		verbose_warning_master("Mgt cpufreq is not ok, switching to monitoring.");

		// earl_default_domains.cpu = POL_NOT_SUPPORTED;

		topology_tostr(tp, topo_str, sizeof(topo_str));
		verbose_master(ERROR_VERB, "%s", topo_str);

		cpufreq_base_init(tp, &bf);
		verbose_info2_master("Frequency detected %lu", (ulong) bf.frequency);
	}else{
			/* If the CPU freq based has not been detected during the dummy initialization phase */
		  if (system_conf->def_freq == 0 && earl_config_dummy){
      	uint nominal;
      	pstate_t *pst_list;
      	mgt_cpufreq_get_nominal(no_ctx, &nominal);
      	pst_list                    = mgt_cpufreq.avail_list;
      	verbose_master(ERROR_VERB, "Using base_freq cpufreq_list[%u] = %llu based on MGT cpufreq",nominal, pst_list[nominal].khz);

      	eard_dummy_replace_base_freq(pst_list[nominal].khz);
    	}    


	}

	if (!mgt_imcfreq.ok)
	{
		verbose_warning_master("Mgt imcfreq is not ok, swtching to monitoring for IMC");
		// earl_default_domains.mem = POL_NOT_SUPPORTED;
	}

#if USE_GPUS
    if (!mgt_gpu.ok)
		{
        verbose_warning_master("GPU dummy API, swtching to monitoring for GPU.");
        // earl_default_domains.gpu = POL_NOT_SUPPORTED;
    }
#endif // USE_GPUS

	if (!met_cpufreq.ok && mgt_cpufreq.ok) {
		verbose_warning_master("Avg cpufreq is not ok. Using mgt_cpufreq");
	}

	if (!met_imcfreq.ok && mgt_imcfreq.ok)
	{
		verbose_warning_master("Avg imcfreq is not ok. Using mgt_imcfreq");
	}

  if (!met_bwidth.ok)
  {
		verbose_warning_master("Mem bandwith not ok. swtching to monitoring");
		// earl_default_domains.cpu = POL_NOT_SUPPORTED;
  }

  if(!met_flops.ok)
	{
		verbose_warning_master("FLOPS not available");
	}

  if (!met_cache.ok)
	{
		verbose_warning_master("Cache not available");
	}

  if (!met_cpi.ok)
	{
		verbose_warning_master("CPI not available. swtching to monitoring");
		// earl_default_domains.cpu = POL_NOT_SUPPORTED;
	}
}


#if DCGMI & USE_GPUS
uint metrics_dcgmi_enabled()
{
  return dcgmi_lib_is_enabled();
}
#endif

void metrics_lib_reset()
{
    cpi_dispose(no_ctx);
  flops_dispose(no_ctx);
  cache_dispose(no_ctx);
}


static void metrics_static_init(topology_t *tp)
{
    char buffer[SZ_BUFFER_EXTRA];

    #define sa(function) \
        function;

    master = (masters_info.my_master_rank >= 0);
    eard   = ((master) ? API_EARD : NO_EARD);
    // Load
    debug("mgt_cpufreq_load");
    sa(mgt_cpufreq_load(tp, API_EARD));

    debug("mgt_imcfreq_load");
    sa(mgt_imcfreq_load(tp, eard, NULL));

		debug("temperature load");
		sa(temp_load(tp, eard));

    debug("energy_cpu_load");
    sa( energy_cpu_load(tp, eard));

    debug("cpufreq_load");
    sa(    cpufreq_load(tp, eard));

    debug("imcfreq_load");
    sa(    imcfreq_load(tp, eard));

    debug("bwidth_load");
    sa(     bwidth_load(tp, eard));

    debug("flops_load");
    sa(      flops_load(tp, eard));

    debug("cache_load");
    sa(      cache_load(tp, eard));

    debug("cpi_load");
    sa(        cpi_load(tp, eard));
    sa(mgt_cpuprio_load(tp, eard));

#if USE_GPUS
		if (eards_connected() || !EARL_LIGHT){ // If we are connected or NOT EAR-lite version
        	gpu_load(eard);
		}else{
			/* If master but EARD not connected */
			if ((eard == API_EARD) && !eards_connected()){
					  gpu_load(API_DEFAULT);
			}	else{
						gpu_load(API_FREE);
			}
		}
    debug("mgt_gpu_load");
    mgt_gpu_load(eard);
#if USE_CUPTI
    if (read_cupti_metrics) {
      gpuproc_load(eard);
    }
#endif

    debug("proc_stat_load");
#if USE_PROC_PS
    if (state_fail(proc_stat_load())){
      verbose_master(2, "proc_stat API failed");
      proc_ps_ok = 0;
    }else{
      verbose_master(2, "proc_stat API loaded");
      proc_ps_ok = 1;
    }
#endif 

    debug("proc_stat_init");
    metric_cond(proc_ps_ok, proc_stat_init(&proc_ctx, getpid()), VERB_PROCPS, "EARL proc_stat_init failed");


#endif // USE_GPUS
    debug("metics_load ready");

    frequency_init(1);

    debug("Asking for APIS");
    // Get API
    sa(mgt_cpufreq_get_api(&mgt_cpufreq.api));
    sa(mgt_imcfreq_get_api(&mgt_imcfreq.api));
		sa(      temp_get_info(&met_temp));
    sa( energy_cpu_get_api(&met_energy.api ));
    sa(    cpufreq_get_api(&met_cpufreq.api));
    sa(    imcfreq_get_api(&met_imcfreq.api));
    sa(     bwidth_get_api(&met_bwidth.api ));
    sa(        cpi_get_api(&met_cpi.api    ));
    sa(mgt_cpuprio_get_api(&mgt_cpuprio.api));
		flops_get_info(&met_flops);

		if (met_cpufreq.api == API_DUMMY) estimate_met_cpuf = 1;
		if (met_bwidth.api  == API_DUMMY) estimate_gbs = 1;

    if ((met_bwidth.api == API_DUMMY) || (met_cpi.api == API_DUMMY) || (met_flops.api == API_DUMMY)) validate_cpu_busy = 0;

#if USE_GPUS
    mgt_gpu_get_api(&mgt_gpu.api);
        gpu_get_api(&met_gpu.api);
#if USE_CUPTI
        if (read_cupti_metrics) {
            gpuproc_get_api(&proc_gpu.api);
        }
#endif
#endif // USE_GPUS

    // Working
    mgt_cpufreq.ok = (mgt_cpufreq.api > API_DUMMY);
    mgt_imcfreq.ok = (mgt_imcfreq.api > API_DUMMY);
    met_temp.ok    = (met_temp.api    > API_DUMMY);
    met_cpufreq.ok = (met_cpufreq.api > API_DUMMY);
    mgt_cpuprio.ok = (mgt_cpuprio.api > API_DUMMY);
    met_imcfreq.ok = (met_imcfreq.api > API_DUMMY);
    met_bwidth.ok  = (met_bwidth.api  > API_DUMMY);
    met_energy.ok  = (met_energy.api  > API_DUMMY);
    met_cache.ok   = 1;
    met_flops.ok   = 1;
    met_cpi.ok     = (met_cpi.api     > API_DUMMY);

#if USE_GPUS
    mgt_gpu.ok     = (mgt_gpu.api     > API_DUMMY);
    met_gpu.ok     = (met_gpu.api     > API_DUMMY);

		#if EARL_LIGHT
		if (mgt_gpu.ok && (mgt_gpu.ok != API_EARD)){
			verbose_info2_master("GPU API is %u", met_gpu.api);
		}
		#endif

#if USE_CUPTI
    if (read_cupti_metrics){
        proc_gpu.ok = (proc_gpu.api   > API_DUMMY);
    }
#endif

#if DCGMI
		/* Just the master loads DCGMI module. */
    if (master && met_gpu.ok)
    {
			state_t ret_st = dcgmi_lib_load();
	    if (state_is(ret_st, EAR_WARNING))
	    {
				// The warning is not relevant for a normal user
				verbose_warning("Loading EARL DCGMI module: %s", state_msg);
	    } else if (state_fail(ret_st))
			{
				// An EAR_ERROR may be relevant for a normal user, as it explictely requests DCGMI metrics.
				verbose_error("Loading EARL DCGMI module: %s", state_msg);
			}
    }
#endif // DCGMI
#endif // USE_GPUS

  debug("metrics_api ready");

  debug("Metrics initialization phase 2");
    // Init
    sa(mgt_cpufreq_init(no_ctx));
    sa(mgt_imcfreq_init(no_ctx));
		sa(       temp_init(      ));
    sa( energy_cpu_init(no_ctx));
    sa(    cpufreq_init(no_ctx));
    sa(    imcfreq_init(no_ctx));
    sa(     bwidth_init(no_ctx));
    sa(      flops_init(no_ctx));
    sa(      cache_init(no_ctx));
    sa(        cpi_init(no_ctx));
    sa(mgt_cpuprio_init());

#if USE_GPUS
    sa(    mgt_gpu_init(no_ctx));
    sa(        gpu_init(no_ctx));
#if USE_CUPTI
    if (read_cupti_metrics){
        sa(    gpuproc_init(no_ctx));
    }
#endif
#endif // USE_GPUS

	debug("metrics_init ready");

    // Count
    sa(mgt_cpufreq_count_devices(no_ctx, &mgt_cpufreq.devs_count));
    sa(mgt_imcfreq_count_devices(no_ctx, &mgt_imcfreq.devs_count));
    sa( energy_cpu_count_devices(no_ctx, &met_energy.devs_count ));
    sa(    cpufreq_count_devices(no_ctx, &met_cpufreq.devs_count));
    sa(    imcfreq_count_devices(no_ctx, &met_imcfreq.devs_count));
    sa(     bwidth_count_devices(no_ctx, &met_bwidth.devs_count ));
#if USE_GPUS
    sa(    mgt_gpu_count_devices(no_ctx, &mgt_gpu.devs_count    ));
    sa(        gpu_count_devices(no_ctx, &met_gpu.devs_count    ));
    #if USE_CUPTI
    if (read_cupti_metrics) {
      sa(    gpuproc_count_devices(no_ctx, &proc_gpu.devs_count   ));
      verbose_master(VERBOSE_CUPTI, "CUPTI devices %u", proc_gpu.devs_count);
    }
    #endif
#endif // USE_GPUS
    debug("metrics_count_device ready\n mgt cpufreq %u\nmgt imcfreq %u\n  mgt gpu %u\n    cpufreq %u\n    imcfreq %u\n     bwidth %u\n        gpu %u temp %u",
            mgt_cpufreq.devs_count, mgt_imcfreq.devs_count, mgt_gpu.devs_count, met_cpufreq.devs_count,
            met_imcfreq.devs_count, met_bwidth.devs_count, met_gpu.devs_count, met_temp.devs_count);

    // Available lists
    sa(mgt_cpufreq_get_available_list(no_ctx, (const pstate_t **) &mgt_cpufreq.avail_list,
                &mgt_cpufreq.avail_count));
    sa(mgt_imcfreq_get_available_list(no_ctx, (const pstate_t **) &mgt_imcfreq.avail_list,
                &mgt_imcfreq.avail_count));

    // Allocation is needed before requesting available and devices list
    sa(mgt_cpuprio_data_alloc((cpuprio_t **) &mgt_cpuprio.avail_list, (uint **) &mgt_cpuprio.current_list));

    /* List of priorities */
    sa(mgt_cpuprio_get_available_list((cpuprio_t *) mgt_cpuprio.avail_list));

    /* List of devices */
    sa(mgt_cpuprio_get_current_list((uint *) mgt_cpuprio.current_list));

    sa(mgt_cpuprio_data_count((uint *) &mgt_cpuprio.avail_count, (uint *) &mgt_cpuprio.devs_count)); 

    if (VERB_ON(CPUPRIO_VERB)) {
        verbose_master(CPUPRIO_VERB, "[CPUPRIO] %s / %u priorities / %u devices.",
                       mgt_cpuprio.ok ? "OK" : "DUMMY", mgt_cpuprio.avail_count, mgt_cpuprio.devs_count);

        mgt_cpuprio_data_tostr((cpuprio_t *) mgt_cpuprio.avail_list, (uint *) mgt_cpuprio.current_list,
                               buffer, SZ_BUFFER_EXTRA);
        verbose_master(CPUPRIO_VERB, "CPUPRIO list of available priorities\n %s", buffer);
    }

#if USE_GPUS
    sa(mgt_gpu_freq_list(no_ctx,
                (const ulong ***) &mgt_gpu.avail_list,
                (const uint **) &mgt_gpu.avail_count));

#endif // USE_GPUS

    pstate_tostr(mgt_cpufreq.avail_list, mgt_cpufreq.avail_count, buffer, SZ_BUFFER_EXTRA);
    verbose_info2_master("CPUFREQ available list of frequencies:\n%s", buffer);
    pstate_tostr(mgt_imcfreq.avail_list, mgt_imcfreq.avail_count, buffer, SZ_BUFFER_EXTRA);
    verbose_info2_master("IMCFREQ available list of frequencies:\n%s", buffer);

		if (VERB_ON(VEARL_INFO) && (master)) {
			verbose(0, "--- EARL loaded APIs ---");
			apis_print(mgt_cpufreq.api, "MGT_CPUFREQ: ");
			apis_print(mgt_imcfreq.api, "MGT_IMCFREQ: ");
			apis_print(mgt_cpuprio.api, "MGT_PRIO   : ");
			apis_print(met_cpufreq.api, "MET_CPUFREQ: ");
			apis_print(met_imcfreq.api, "MET_IMCFREQ: ");
			apis_print(met_energy.api , "MET_ENERGY : ");
			apis_print(met_temp.api,    "MET_TEMP:    ");
			apis_print(met_bwidth.api , "MET_BWIDTH : ");
			apis_print(met_flops.api  , "MET_FLOPS  : ");
			apis_print(met_cpi.api    , "MET_CPI    : ");
#if USE_GPUS
			apis_print(mgt_gpu.api    , "MGT_GPU    : ");
			apis_print(met_gpu.api    , "MET_GPU    : ");
#if USE_CUPTI
			if (read_cupti_metrics) {
				apis_print(proc_gpu.api   , "PROC_GPU    : ");
			}
#endif
#endif // USE_GPUS
			cache_internals_tostr(cache_description, sizeof(cache_description));
			verbose(0, "%s", cache_description);
			verbose(0, "------------------------");
		}

    cpufreq_data_alloc(&cpufreq_read1[0], empty);
    cpufreq_data_alloc(&cpufreq_read1[1], empty);
    cpufreq_data_alloc(&cpufreq_read2[0], empty);
    cpufreq_data_alloc(&cpufreq_read2[1], &cpufreq_diff);

    imcfreq_data_alloc(&imcfreq_read1[0], empty);
    imcfreq_data_alloc(&imcfreq_read1[1], empty);
    imcfreq_data_alloc(&imcfreq_read2[0], &imcfreq_diff[0]);
    imcfreq_data_alloc(&imcfreq_read2[1], &imcfreq_diff[1]);

    temp_data_alloc(&temp_read1[LOO]);
    temp_data_alloc(&temp_read1[APP]);
    temp_data_alloc(&temp_read2[LOO]);
    temp_data_alloc(&temp_read2[APP]);
    temp_data_alloc(&temp_diff[LOO]);
    temp_data_alloc(&temp_diff[APP]);

    bwidth_data_alloc(&bwidth_read1[0]);
    bwidth_data_alloc(&bwidth_read1[1]);
    bwidth_data_alloc(&bwidth_read2[0]);
    bwidth_data_alloc(&bwidth_read2[1]);
    bwidth_data_alloc(&bwidth_diff[0]);
    bwidth_data_alloc(&bwidth_diff[1]);

#if USE_GPUS
    gpu_data_alloc(&gpu_metrics_read1[LOO]);
    gpu_data_alloc(&gpu_metrics_read1[APP]);
    gpu_data_alloc(&gpu_metrics_read2[LOO]);
    gpu_data_alloc(&gpu_metrics_read2[APP]);
    gpu_data_alloc(&gpu_metrics_diff[LOO]);
    gpu_data_alloc(&gpu_metrics_diff[APP]);
    gpu_data_alloc(&gpu_metrics_busy);
    gpu_data_alloc(&gpu_metrics_busy_diff);
#if USE_CUPTI
		if (read_cupti_metrics) {
			gpuproc_data_alloc(&proc_gpu_data_init[LOO]);
			gpuproc_data_alloc(&proc_gpu_data_init[APP]);
			gpuproc_data_alloc(&proc_gpu_data_end[LOO]);
			gpuproc_data_alloc(&proc_gpu_data_end[APP]);
			gpuproc_data_alloc(&proc_gpu_data_diff[LOO]);
			gpuproc_data_alloc(&proc_gpu_data_diff[APP]);
			verbose_master(VERBOSE_CUPTI, "PROC_GPU data allocated");
		}
#endif
#endif // USE_GPUS

    energy_cpu_data_alloc(no_ctx, &metrics_rapl[LOO], &rapl_size);
    energy_cpu_data_alloc(no_ctx, &metrics_rapl[APP], &rapl_size);
    energy_cpu_data_alloc(no_ctx, &aux_rapl, &rapl_size);
    energy_cpu_data_alloc(no_ctx, &last_rapl, &rapl_size);

	rapl_elements = rapl_size;

    debug("metrics_allocation ready");

    metrics_configuration(tp); // Check the status of loaded APIs

    #if METRICS_OVH
    overhead_suscribe("partial_stop", &id_ovh_partial_stop);
    overhead_suscribe("compute_signature_data", &id_ovh_compute_data);
    overhead_suscribe("DC_energy", &id_ovh_dc_energy);
    overhead_suscribe("RAPL", &id_ovh_rapl);
    overhead_suscribe("AVG_CPU_freq", &id_ovh_cpufreq);
    overhead_suscribe("AVG_IMC_freq", &id_ovh_imcfreq);
    #endif
}


// Legacy code
static void cpufreq_my_avgcpufreq(ulong *nodecpuf, ulong *avgf)
{
	cpu_set_t m;
	ulong total = 0, total_cpus = 0;
	ulong lavg, lcpus;
	int p;
    for (p = 0; p < lib_shared_region->num_processes; p++)
    {
        m = sig_shared_region[p].cpu_mask;
		pstate_freqtoavg(m, nodecpuf, met_cpufreq.devs_count, &lavg, &lcpus);
		total += lavg*lcpus;
		total_cpus += lcpus;
	}
	*avgf = total/total_cpus;
}

static void cpufreq_process_avgcpufreq(uint proc, ulong *nodecpuf, ulong *avgf)
{
  cpu_set_t m;
  ulong lavg = 0, lcpus = 0;

  m = sig_shared_region[proc].cpu_mask;
  pstate_freqtoavg(m, nodecpuf, met_cpufreq.devs_count, &lavg, &lcpus);
  *avgf = lavg;
}

void set_null_dc_energy(edata_t e)
{
    memset((void *)e,0,node_energy_datasize);
}

void set_null_rapl(ull *erapl)
{
    memset((void*)erapl,0,rapl_elements*sizeof(ull));
}

long long metrics_time()
{
    return (llong) timestamp_getconvert(TIME_USECS);
}

static void metrics_global_start()
{
	aux_time = metrics_time();
	app_start = aux_time;
    state_t s;
	int ret;

    // Init energy savings fields
	app_energy_saving.jid     = application.job.id;
	app_energy_saving.step_id = application.job.step_id;
  app_phase_summary.jid     = application.job.id;
  app_phase_summary.step_id = application.job.step_id;

	strcpy(app_energy_saving.node_id, application.node_id);
	strcpy(app_phase_summary.node_id, application.node_id);

	metrics_usecs[APP] = 0 ;

	if (master)
	{
		/* Avg CPU freq */
        if (state_fail(s = cpufreq_read(no_ctx, cpufreq_read1[APP]))) {
            debug("CPUFreq data read in global_start");
        }
        /* To be used in partial_start */
        cpufreq_data_copy(cpufreq_read2[LOO], cpufreq_read1[APP]);
		/* Reads IMC data */
		if (mgt_imcfreq.ok){
		    debug("imcfreq_read");
		    if (state_fail(s = imcfreq_read(no_ctx, imcfreq_read1[APP]))){
			    debug("IMCFreq data read in global_start");
		    }
        /* To be used in partial_start */
        imcfreq_data_copy(imcfreq_read2[LOO], imcfreq_read1[APP]);
		}


    verb_state_fail(temp_read(temp_read1[APP], NULL), TEMP_VERB, "Reading APP temperature in global_start");

#if USE_GPUS
        gpu_data_null(gpu_metrics_read1[APP]);
        if (state_ok(gpu_read(no_ctx, gpu_metrics_read1[APP]))) {

            // We set here the starting sample for partial start/stop flow
            gpu_data_copy(gpu_metrics_read2[LOO], gpu_metrics_read1[APP]);

            if (VERB_ON(GPU_VERB + 1)) {
                gpu_data_tostr(gpu_metrics_read1[APP], gpu_str, sizeof(gpu_str));
                verbose(GPU_VERB + 1, "Initial global GPU data\n-----------------------\n%s", gpu_str);
            }

        } else {
            verbose_error("Reading GPU metrics at application start");
        }
#endif // USE_GPUS

		/* Only the master computes per node I/O metrics */
		if (state_fail(metrics_compute_total_io(&metrics_io_init[APP_NODE]))) {
            verbose_warning_master("Node total I/O could not be read (%s)." , state_msg);
    }
    /* To be used in partial_start */
    io_copy(&metrics_io_end[LOO_NODE], &metrics_io_init[APP_NODE]);

  	/* To be used in partial_start */
  	temp_data_copy(temp_read2[LOO], temp_read1[APP]);
	}

  /* OS procs statistics */
  metric_cond(proc_ps_ok, proc_stat_read(&proc_ctx, &metrics_proc_init[APP]), VERB_PROCPS, "proc_stat_read failed in global start");
  metric_cond(proc_ps_ok, proc_stat_data_copy(&metrics_proc_init[LOO], &metrics_proc_init[APP]), VERB_PROCPS, "proc_stat_data_copy failed in global start");

  #if USE_CUPTI
  if (read_cupti_metrics){
    if (state_fail(gpuproc_read(no_ctx, proc_gpu_data_init[APP]))){
      verbose_warning("gpuproc_read failed");
    }
    /* We will copy in partial start */
    gpuproc_data_copy(proc_gpu_data_end[LOO], proc_gpu_data_init[APP]);
  }
  #endif


	/* Per process IO data */
	if (io_read(&ctx_io, &metrics_io_init[APP]) != EAR_SUCCESS){
		verbose_warning_master("I/O data not available.");
	}
  io_copy(&metrics_io_end[LOO], &metrics_io_init[APP]);

	// New metrics
	/* Energy */
	debug(" node_dc_energy");
	ret = energy_read(NULL, aux_energy);
	if ((ret == EAR_ERROR) || energy_data_is_null(aux_energy)){
		verbose_error_master("Reading Node energy at application start.");
	}

	/* RAPL energy metrics */
	energy_cpu_read(no_ctx, aux_rapl);
	debug("energy_cpu_read ready");
  /* No need to copy aux_rapl in other metric for partial_start */

	bwidth_read(no_ctx, bwidth_read1[APP]);
	debug("bwidth_read ready");
  /* To be used in partial_start */
  bwidth_data_copy(bwidth_read2[LOO], bwidth_read1[APP]);

	flops_read(no_ctx, &flops_read1[APP]);
	debug("flops_read ready");
  /* To be used in partial_start */
  flops_data_copy(&flops_read2[LOO], &flops_read1[APP]);
	
	cache_read(no_ctx, &cache_read1[APP]);
	debug("cache_read ready");
  /* To be used in partial_start */
  cache_data_copy(&cache_read2[LOO], &cache_read1[APP]);

	cpi_read(no_ctx,   &cpi_read1[APP]);
	debug("cpi_read ready");
  /* To be used in partial_start */
  cpi_data_copy(&cpi_read2[LOO], &cpi_read1[APP]);



}


static void metrics_global_stop()
{
	char io_info[256];
	char *verb_path=ear_getenv(FLAG_EARL_VERBOSE_PATH);
	state_t s;
	int i;

	debug("%s--- (l. rank) %d  METRICS GLOBAL STOP ---%s", COL_YLW, my_node_id, COL_CLR);

    if (!master)
    {
        /* Synchro 4: Waiting for master signature */
        while(!lib_shared_region->global_stop_ready)
        {
            usleep(100);
            if (msync(lib_shared_region, sizeof(lib_shared_data_t), MS_SYNC) < 0)
            {
                verbose_warning("Memory sync. for lib shared region failed: %s", strerror(errno));
            }
        }
    }

	imcfreq_avrg[APP] = 0;
	cpufreq_avrg[APP] = 0;

	/* Only the master will collect these metrics */
	if (master) {

		/* MPI statistics */
		if (module_mpi_is_enabled()) {
			read_diff_node_mpi_info(lib_shared_region, sig_shared_region,
					metrics_mpi_info[APP], metrics_last_mpi_info[APP]);
#if MPI_CALL_TYPES
            // Below is useful to have loop's MPI call types info. It is disabled as now MPI
            // call types data is just collected by using the flag FLAG_GET_MPI_STATS.
			read_diff_node_mpi_types_info(lib_shared_region, sig_shared_region,
					metrics_mpi_calls_types[APP], metrics_last_mpi_calls_types[APP]);
#endif
        }

        /* Avg CPU frequency */
        if (state_fail(s = cpufreq_read(no_ctx, cpufreq_read2[APP]))) {
            verbose_warning("cpufreq data read failed in global_stop.");
        }
        if (state_fail(s = cpufreq_data_diff(cpufreq_read2[APP], cpufreq_read1[APP],
                        cpufreq_diff, &cpufreq_avrg[APP]))) {
            verbose_warning("cpufreq data diff failed in global_stop.");
        } else {
            verbose(3, "%sINFO%s CPUFreq average for the application is %.2fGHz.",
                    COL_BLU, COL_CLR, (float) cpufreq_avrg[APP] / 1000000.0);
        }

        cpufreq_my_avgcpufreq(cpufreq_diff, &cpufreq_avrg[APP]);

        // Copy the average CPU freq. in the shared signature
        memcpy(lib_shared_region->avg_cpufreq, cpufreq_diff,
                met_cpufreq.devs_count * sizeof(cpufreq_diff[0]));

        /* AVG IMC frequency */
        if (mgt_imcfreq.ok) {
            if (state_fail(s = imcfreq_read(no_ctx, imcfreq_read2[APP]))) {
                error_lib("IMC freq. data read in global stop.");
            }
            if (state_fail(s = imcfreq_data_diff(imcfreq_read2[APP], imcfreq_read1[APP],
                            imcfreq_diff[APP], &imcfreq_avrg[APP]))){
                error_lib("IMC freq. data diff failed.");
            }

            verbose(VEARL_INFO2, "Avg. IMC frequency: %.2f.",
                    (float) imcfreq_avrg[APP] / 1000000.0);

            for (i=0; i< mgt_imcfreq.devs_count;i++) {
                imc_max_pstate[i]= imc_num_pstates-1;
                imc_min_pstate[i] = 0;
            }
            if (state_fail(s = mgt_imcfreq_set_current_ranged_list(no_ctx,
                            imc_min_pstate, imc_max_pstate))){
                error_lib("Setting the IMC freq. in global stop.");
            } else
            {
                verbose(VEARL_INFO2, "IMC freq. restored to %.2f-%.2f",
                        (float) imc_pstates[imc_min_pstate[0]].khz / 1000000.0,
                        (float) imc_pstates[imc_max_pstate[0]].khz / 1000000.0);
            }
        }

				/* Temperature */
    		verb_state_fail(temp_read(temp_read2[APP], NULL), TEMP_VERB, "Reading APP temperature in global stop");
    		temp_data_diff(temp_read2[APP], temp_read1[APP], temp_diff[APP], NULL);


#if USE_GPUS
        /* GPU */
        if (state_ok(gpu_read(no_ctx, gpu_metrics_read2[APP]))) {

            gpu_data_diff(gpu_metrics_read2[APP],
                    gpu_metrics_read1[APP], gpu_metrics_diff[APP]);

            if (VERB_ON(GPU_VERB + 1)) {
                gpu_data_tostr(gpu_metrics_diff[APP], gpu_str, sizeof(gpu_str));
                verbose(GPU_VERB + 1, "Global GPU data\n---------------\n%s", gpu_str);
            }
        } else {
            verbose_warning("Error reading GPU metrics at application ending.");
        }

#endif // USE_GPUS
		/* IO node data */
		if (state_fail(metrics_compute_total_io(&metrics_io_end[APP_NODE]))) {

			// As we could not read the global node IO, we
			// put as global IO the total accumulated until now.
			metrics_io_diff[APP_NODE] = metrics_io_diff[APP_ACUM];

            verbose_warning("Node total I/O could not be read (%s), working with"
                            " the acumulated data read until now.", state_msg);
		} else {
			io_diff(&metrics_io_diff[APP_NODE], &metrics_io_init[APP_NODE],
					&metrics_io_end[APP_NODE]);
		}
	} // master

	/* Per-process IO */
	if (state_fail(io_read(&ctx_io, &metrics_io_end[APP]))) {
		verbose_warning_master("I/O data not available.");
	} else {
		io_diff(&metrics_io_diff[APP], &metrics_io_init[APP], &metrics_io_end[APP]);
	}

	if (VERB_ON(IO_VERB)) {
		int fd;
		io_tostr(&metrics_io_diff[APP], io_info, sizeof(io_info));
		if (verb_path != NULL) {

			char file_name[256];

			sprintf(file_name, "%s/earl_io_info.%d.%d", verb_path, my_step_id, my_job_id);

			fd = open(file_name,O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP);
		} else {
			fd = 2;
		}

		char *io_buffer = malloc(strlen(io_info) + 200);
		assert(io_buffer != NULL);

		sprintf(io_buffer, "[%d] APP I/O DATA: %s\n", ear_my_rank, io_info);
		if (write(fd, io_buffer, strlen(io_buffer)) < 0)
		{
			error_lib("Writing I/O info to fd %d: %s", fd, strerror(errno));
		}

		free(io_buffer);

		if (verb_path != NULL) close(fd);
	}

	// New metrics

	bwidth_read_diff(no_ctx, bwidth_read2[APP], bwidth_read1[APP], NULL, &bwidth_cas[APP], NULL);
  if (master) lib_shared_region->cas_counters = bwidth_cas[APP];
	flops_read_diff(no_ctx, &flops_read2[APP], &flops_read1[APP], NULL, NULL);
	cache_read_diff(no_ctx, &cache_read2[APP], &cache_read1[APP], &cache_diff[APP], &cache_bwidth[APP]);
	cpi_read_diff(no_ctx,   &cpi_read2[APP],   &cpi_read1[APP],   &cpi_diff[APP],   &cpi_avrg[APP]);

  #if USE_CUPTI
  if (read_cupti_metrics){
    if (state_fail(gpuproc_read_diff(no_ctx, proc_gpu_data_end[APP], proc_gpu_data_init[APP], proc_gpu_data_diff[APP]))){
     verbose(VERBOSE_CUPTI, "gpuproc_read_diff global stop failed");
    }
    gpuproc_data_tostr(proc_gpu_data_diff[APP], gpu_proc_buffer, sizeof(gpu_proc_buffer));
    verbose(VERBOSE_CUPTI,"GPU_PROC APP DATA for [%d][%d] : %s", masters_info.my_master_rank, my_node_id, gpu_proc_buffer);
  }
  #endif
}

static void metrics_partial_start()
{


	debug("metrics_partial_start starts %p %p %lu",
			metrics_ipmi[LOO], aux_energy, node_energy_datasize);

	/* This must be replaced by a copy of energy */
	/* Energy node */
	memcpy(metrics_ipmi[LOO], aux_energy, node_energy_datasize);

	/* Time */
	metrics_usecs[LOO] = aux_time;
	last_elap_sec = 0;


	/* Per-process IO data */
	debug("starting IO for process");
  io_copy(&metrics_io_init[LOO], &metrics_io_end[LOO]);
	debug("IO read done");


  metric_cond(proc_ps_ok, proc_stat_data_copy(&metrics_proc_init[LOO], &metrics_proc_end[LOO]), VERB_PROCPS, "proc_stat_data_copy failed in partial start");


	/* These data is measured only by the master */
	if (master) {
		/* Avg CPU freq */
    cpufreq_data_copy(cpufreq_read1[LOO], cpufreq_read2[LOO]);
		debug("cpufreq_read ");

		/* Avg IMC freq */
		if (mgt_imcfreq.ok){
			/* Reads the IMC */
      imcfreq_data_copy(imcfreq_read1[LOO], imcfreq_read2[LOO]);
		}

		/* Per NODE IO data */
    io_copy(&metrics_io_init[LOO_NODE], &metrics_io_end[LOO_NODE]);

#if USE_GPUS
        /* GPUS */
        /*  Avoidable since at global start: gpu_metrics_read2[LOO] <- gpu_metrics_read1[APP]
            if (gpu_loop_stopped) {
            */
        gpu_data_copy(gpu_metrics_read1[LOO], gpu_metrics_read2[LOO]);
        /*
           } else {
           gpu_data_copy(gpu_metrics_read1[LOO], gpu_metrics_read1[APP]);
           }
           */
        debug("gpu_copy in partial start");
        #if USE_CUPTI
        if (read_cupti_metrics){
          gpuproc_data_copy(proc_gpu_data_init[LOO], proc_gpu_data_end[LOO]);
        }
        #endif
#endif // USE_GPUS

				temp_data_copy(temp_read1[LOO], temp_read2[LOO]);

    } // master_rank

	/* Energy RAPL */
  energy_cpu_data_copy(no_ctx, last_rapl, aux_rapl);
	debug("RAPL copied");

	// New metrics
  bwidth_data_copy(bwidth_read1[LOO], bwidth_read2[LOO]);
  flops_data_copy(&flops_read1[LOO], &flops_read2[LOO]);
  cache_data_copy(&cache_read1[LOO], &cache_read2[LOO]);
  cpi_data_copy(&cpi_read1[LOO], &cpi_read2[LOO]);

	debug("partial_start ends");
}

/****************************************************************************************************************************************************************/
/*************************** This function is executed every N seconds to check if signature can be compute *****************************************************/
/****************************************************************************************************************************************************************/

extern uint ear_iterations;
extern int ear_guided;


static int metrics_partial_stop(uint where)
{
	ulong c_energy;
	long long c_time;
	float c_power;
	long long aux_time_stop;
	char stop_energy_str[1024], start_energy_str[1024];
	state_t s;


    if (!eards_connected() && (master)) {
        if (!EARL_LIGHT)
        {
        	verbose_error_master("Disconnecting library because EARD not connected.");
			lib_shared_region->earl_on = 0;
        }
    }

    /* If the signature of the master is not ready, we cannot compute our signature */
    //debug("My rank is %d",masters_info.my_master_rank);
    if (masters_info.my_master_rank < 0 && !sig_shared_region[0].ready) {
        debug("Iterations %u ear_guided %u", ear_iterations, ear_guided);
        debug("%s: master not ready", node_name);
        return EAR_NOT_READY;
    }

#if SHOW_DEBUGS
	float elapsed = (metrics_time() - app_start) / 1000000;
	debug("Master signature ready at time %f", elapsed);
#endif

	// Manual IPMI accumulation
	/* Energy node */
	//int ret = eards_node_dc_energy(aux_energy_stop, node_energy_datasize);
 
  #if METRICS_OVH
  overhead_start(id_ovh_dc_energy);
  #endif
	int ret = energy_read(NULL, aux_energy_stop);
  #if METRICS_OVH
  overhead_stop(id_ovh_dc_energy);
  #endif
	if (ret == EAR_ERROR) {
		verbose_error_master("Reading Node energy at partial stop.");
	}

	/* If EARD is present, we wait until the power is computed */
	/*if (energy_lib_data_is_null(aux_energy_stop) || (ret == EAR_ERROR)) {
		if (eards_connected()) return EAR_NOT_READY;
		else return EAR_SUCCESS;
	}*/

	energy_data_accumulated(&c_energy,metrics_ipmi[LOO],aux_energy_stop);
	energy_data_to_str(start_energy_str,metrics_ipmi[LOO]);
	energy_data_to_str(stop_energy_str,aux_energy_stop);
	debug("start energy str %s", start_energy_str);
	debug("end energy str %s", stop_energy_str);

  #if FAKE_ENERGY_READING
  c_energy = 0;
  verbose_master(0,"FAKE_ENERGY_READING used ");
  #endif



	/* If EARD is present, we wait until the power is computed */
	if ((where==SIG_END) && (c_energy==0) && (master)) {
		debug("EAR_NOT_READY because of accumulated energy %lu\n",c_energy);
		if (dispose) fprintf(stderr,"partial stop and EAR_NOT_READY\n");
		if (!EARL_LIGHT) return EAR_NOT_READY;
	}

	/* Time */
	aux_time_stop = metrics_time();

	/* Sometimes energy is not zero but power is not correct */
	c_time = ear_max(metrics_usecs_diff(aux_time_stop, metrics_usecs[LOO]), 1);

	/* energy is computed in node_energy_units and time in usecs */
	c_power=(float)(c_energy*(1000000.0/(double)node_energy_units))/(float)c_time;
	verbose_master(2, "EARL[%d][%d] %f = %lu mJ (units = %d) / %lf usecs", ear_my_rank, getpid(), c_power, c_energy, node_energy_units, (float)c_time);

  #if FAKE_POWER_READING
  c_power = 1000000;
  verbose_master(0,"FAKE_POWER_READING used");
  #endif

	if (master) {
    /* Error is manage later in this function */
		if (dispose && ((c_power<=0) || (c_power>system_conf->max_sig_power))){
			debug("dispose and c_power %lf\n",c_power);
			debug("power %f energy %lu time %llu\n",c_power,c_energy,c_time);
		}
	}

	/* If we are not the node master, we will continue */
	if ((where==SIG_END) && (c_power<system_conf->min_sig_power) && (master)) {
		debug("EAR_NOT_READY because of power %f\n",c_power);
		if (!EARL_LIGHT) return EAR_NOT_READY;
	}

	/* TODO: This commment can not be above GPU code: This is new to avoid cases where uncore gets frozen */

	if (master) {
#if USE_GPUS
        if (state_ok(gpu_read(no_ctx, gpu_metrics_read2[LOO]))) {

            // gpu_loop_stopped = 1;
            gpu_data_diff(gpu_metrics_read2[LOO], gpu_metrics_read1[LOO], gpu_metrics_diff[LOO]);

            if (VERB_ON(GPU_VERB + 1)) {
                gpu_data_tostr(gpu_metrics_diff[LOO], gpu_str, sizeof(gpu_str));
                verbose(GPU_VERB + 1, "Partial GPU data\n---------------\n%s", gpu_str);
            }
        } else {
            gpu_data_null(gpu_metrics_read2[LOO]);
            gpu_data_null(gpu_metrics_diff[LOO]);
            verbose_warning("Error reading GPU data in partial stop.");
        }

#endif // USE_GPUS
    }

    /* End new section to check frozen uncore counters */
    energy_data_copy(aux_energy,aux_energy_stop);
    aux_time=aux_time_stop;

    if (master) {
		if ((c_power > system_conf->min_sig_power) && (c_power < system_conf->max_sig_power)) {
			acum_ipmi[LOO] = c_energy;

		} else {
      /* If there is an error, power is computed based on power from CPu+GPU. Computed in 
       * compute_signature_data */
			verbose_master(ERROR_VERB, "%sWARNING%s Computed power (%lf) is not correct [%lf-%lf]. Estimate power enabled \n",
                    COL_YLW, COL_CLR, c_power, system_conf->min_sig_power, system_conf->max_sig_power);
			// acum_ipmi[LOO] = system_conf->min_sig_power*c_time;
      estimate_power = 1;
		}
    /* Power error control: Delay this accumulation to the job power computation */ 
		 //acum_ipmi[APP] += acum_ipmi[LOO];
	}

	/* Per process IO data */
	if (io_read(&ctx_io, &metrics_io_end[LOO]) != EAR_SUCCESS)
    {
		verbose_warning_master("I/O data not available");
	}
	io_diff(&metrics_io_diff[LOO], &metrics_io_init[LOO], &metrics_io_end[LOO]);


  /* Process statistics */
  metric_cond(proc_ps_ok, proc_stat_read(&proc_ctx, &metrics_proc_end[LOO]), VERB_PROCPS, "EARL proc_stat_read failed in partial_stop");


	//  Manual time accumulation
	metrics_usecs[LOO] = c_time;
	metrics_usecs[APP] = metrics_usecs[APP] + metrics_usecs[LOO];

	// Daemon metrics
	if (master) {
		/* MPI statistics */
		if (module_mpi_is_enabled()) {
			read_diff_node_mpi_info(lib_shared_region, sig_shared_region,
					metrics_mpi_info[LOO], metrics_last_mpi_info[LOO]);
#if MPI_CALL_TYPES
            // Below is useful to have loop's MPI call types info. It is disabled as now
            // MPI call types data is just collected by using the flag FLAG_GET_MPI_STATS.
            read_diff_node_mpi_types_info(lib_shared_region, sig_shared_region,
					metrics_mpi_calls_types[LOO], metrics_last_mpi_calls_types[LOO]);
#endif
        }

#if METRICS_OVH
    overhead_start(id_ovh_cpufreq);
#endif

		/* Avg CPU freq */
		if (state_fail(s = cpufreq_read(no_ctx, cpufreq_read2[LOO]))) {
			verbose_warning_master("CPUFreq data read in partial stop failed.");
		}

#if METRICS_OVH
    overhead_stop(id_ovh_cpufreq);
#endif

		if (xtate_fail(s, cpufreq_data_diff(cpufreq_read2[LOO], cpufreq_read1[LOO], cpufreq_diff, &cpufreq_avrg[LOO]))){
			verbose_warning_master("CPUFreq data diff failed");
		} else {
			//verbose_master(1, "CPUFreq average is %.2f GHz", (float) cpufreq_avrg[LOO] / 1000000.0);
		}
		// Simple verbose
		if (verb_level >= 3) {
			for (int cpu = 0; cpu < mtopo.cpu_count; cpu++) {
				verbosen_master(3, "AVGCPU[%d] = %.2f ",cpu,(float)cpufreq_diff[cpu]/1000000.0);
			}
			verbose_master(3, "%s", "");
		}
		cpufreq_my_avgcpufreq(cpufreq_diff, &cpufreq_avrg[LOO]);
		memcpy(lib_shared_region->avg_cpufreq, cpufreq_diff, met_cpufreq.devs_count * sizeof(cpufreq_diff[0]));

		/* Avg IMC frequency */
		if (mgt_imcfreq.ok){
#if METRICS_OVH
      overhead_start(id_ovh_imcfreq);
#endif
			if (state_fail(s = imcfreq_read(no_ctx,imcfreq_read2[LOO]))){
				verbose_warning_master("Error IMC freq. data read in partial stop");
			}
#if METRICS_OVH
      overhead_stop(id_ovh_imcfreq);
#endif
			if (state_fail(s = imcfreq_data_diff(imcfreq_read2[LOO],imcfreq_read1[LOO], imcfreq_diff[LOO], &imcfreq_avrg[LOO]))){
				verbose_warning_master("IMC data diff fails.");
			}
			debug("AVG IMC frequency %.2f",(float)imcfreq_avrg[LOO]/1000000.0);
		}

    

		/* Only the master read RAPL because is not per-process metric */
		/* RAPL energy */
#if METRICS_OVH
    overhead_start(id_ovh_rapl);
#endif
		energy_cpu_read(no_ctx, aux_rapl);
#if METRICS_OVH
    overhead_stop(id_ovh_rapl);
#endif
		// We read acuumulated energy
		energy_cpu_data_diff(no_ctx, last_rapl, aux_rapl, metrics_rapl[LOO]);
	
		// Manual RAPL accumulation
    ullong total_RAPL_energy = 0;
		for (int i = 0; i < rapl_elements; i++) {
			metrics_rapl[APP][i] += metrics_rapl[LOO][i];
      total_RAPL_energy += metrics_rapl[LOO][i];
		  debug("energy CPU: %llu", metrics_rapl[LOO][i]);
		}

    verb_state_fail(temp_read(temp_read2[LOO], NULL), TEMP_VERB, "Reading LOO temperature in partial stop");
		temp_data_diff(temp_read2[LOO], temp_read1[LOO], temp_diff[LOO], NULL);

	} //master_rank (metrics collected by node_master)

	/* This code needs to be adapted to read , compute the difference, and copy begin=end 
	 * diff_uncores(values,values_end,values_begin,num_counters);
	 * copy_uncores(values_begin,values_end,num_counters);
	 */
	// New metrics
	bwidth_read_diff(no_ctx, bwidth_read2[LOO], bwidth_read1[LOO], bwidth_diff[LOO], &bwidth_cas[LOO], NULL);
  if (master) lib_shared_region->cas_counters = bwidth_cas[LOO];
	flops_read_diff(no_ctx, &flops_read2[LOO], &flops_read1[LOO], &flops_diff[LOO], NULL);
	cache_read_diff(no_ctx, &cache_read2[LOO], &cache_read1[LOO], &cache_diff[LOO], &cache_bwidth[LOO]);
	cpi_read_diff(no_ctx,   &cpi_read2[LOO],   &cpi_read1[LOO],   &cpi_diff[LOO],   &cpi_avrg[LOO]);

  #if USE_CUPTI
  if (read_cupti_metrics){
    if (state_fail(gpuproc_read_diff(no_ctx, proc_gpu_data_end[LOO], proc_gpu_data_init[LOO], proc_gpu_data_diff[LOO]))){
     verbose_warning("gpuproc_read_diff failed");
    }
    gpuproc_data_tostr(proc_gpu_data_diff[LOO], gpu_proc_buffer, sizeof(gpu_proc_buffer));
    verbose(VERBOSE_CUPTI, "GPU_PROC LOOP DATA for [%d][%d] : %s", masters_info.my_master_rank, my_node_id, gpu_proc_buffer);
  }
  #endif

	// New accumulations: by now these application metrics are taken accumulating loop values.
	// Not sure if required this way in the future, is just because if there are too long apps
	// that could overflow these metric counters.
	flops_data_accum(&flops_diff[APP], &flops_diff[LOO], NULL);

	return EAR_SUCCESS;
}


ull metrics_vec_inst(signature_t *metrics)
{
	ullong fops = 0LLU;
	if (metrics->FLOPS[INDEX_512F] > 0) fops += metrics->FLOPS[INDEX_512F] / WEIGHT_512F;
	if (metrics->FLOPS[INDEX_512D] > 0) fops += metrics->FLOPS[INDEX_512D] / WEIGHT_512D;
	return fops;
}


void copy_node_data(signature_t *dest, signature_t *src)
{
	dest->DC_power	=	src->DC_power;
	dest->DRAM_power=	src->DRAM_power;
	dest->PCK_power	=	src->PCK_power;
	dest->avg_f			= src->avg_f;
	dest->avg_imc_f = src->avg_imc_f;
  dest->GBS       = src->GBS;
  dest->TPI       = src->TPI;
#if USE_GPUS
	memcpy(&dest->gpu_sig,&src->gpu_sig,sizeof(gpu_signature_t));
#endif
}


static void estimate_metrics_not_available_by_processes(signature_t *metrics)
{

  /* This function is executed after all processes compute their signature */
  if (!master) { return;}
  verbose_master(3, "Cache BDWITH  %f",  lib_shared_region->total_cache_bwidth);
  if (estimate_gbs){ 
    metrics->GBS = lib_shared_region->total_cache_bwidth;

    /* Node signature */
    lib_shared_region->node_signature.GBS = metrics->GBS;
    lib_shared_region->node_signature.TPI = (metrics->GBS * sig_shared_region[my_node_id].period * 1024*1024*1024) / sig_shared_region[my_node_id].sig.instructions;
    lib_shared_region->cas_counters        = metrics->GBS*1024*1024*1024/(hw_cache_line_size * sig_shared_region[my_node_id].period);

    /* Job Signature */
    lib_shared_region->job_signature.GBS = lib_shared_region->node_signature.GBS;
    lib_shared_region->job_signature.TPI = lib_shared_region->node_signature.TPI;

    verbose_info2_master("NODE GBS estimated %lf GB/s (period %lf)", metrics->GBS , sig_shared_region[my_node_id].period);
  }
}
static void estimate_metrics_not_available_by_node(uint sign_app_loop_idx, signature_t *metrics, ulong procs, float gpup)
{
    /* This function is executed before all processes compute their signature */

		if (!master) {
			debug("Warning, estimate metrics and not master task");
			return;
		}


    if (estimate_power){
      /* Power error control: There was an error on power computation and we use an estimation */
      double current_ratio;
      if (dc_power_ratio_instances && dc_power_ratio){
        current_ratio = dc_power_ratio / (double)dc_power_ratio_instances;
      }else current_ratio = 1.0;
      metrics->DC_power = (gpup + metrics->DRAM_power + metrics->PCK_power) * current_ratio;
      verbose_info_master("Warning: Estimated power %.2lf W (DC vs CPU+GPU %.2lf)", metrics->DC_power, current_ratio);
      estimate_power = 0;
    }


		if (estimate_met_cpuf)
        {
			if (mgt_cpufreq.ok){ 
				verbose_info2_master("Estimating AVG CPU freq using mgt CPU freq for %u CPUs", met_cpufreq.devs_count);
				metrics->avg_f = metrics->def_f;
				verbose_info2_master("Using def_freq %lu as avg_freq", metrics->def_f);
        cpufreq_avrg[sign_app_loop_idx] = metrics->def_f;
				for (uint i=0; i< mgt_cpufreq.devs_count; i ++) cpufreq_diff[i] = metrics->def_f;
        // Copy the average CPU freq. in the shared signature

        memcpy(lib_shared_region->avg_cpufreq, cpufreq_diff,
                mgt_cpufreq.devs_count * sizeof(cpufreq_diff[0]));

			}
		}
}


/* This function computes the signature: per loop (global = LOO) or application ( global = APP)
 * The node master has computed the per-node metrics and the rest of processes uses this information. */
static void metrics_compute_signature_data(uint sign_app_loop_idx, signature_t *metrics,
		uint iterations, ulong procs)
{
	// If sign_app_loop_idx is 1, it means that the global application metrics are required
	// instead the small time metrics for loops. 'sign_app_loop_idx' is just a
	// signature index.
	sig_ext_t *sig_ext;
	uint num_th = ear_get_num_threads();

	//verbose(0, "process %d - %sapp/loop(1/0) %u%s",
  //			my_node_id, COL_YLW, sign_app_loop_idx, COL_CLR);

	/* Total time */
	metrics_usecs[sign_app_loop_idx] = ear_max(metrics_usecs[sign_app_loop_idx], 1);
	double time_s = (double) metrics_usecs[sign_app_loop_idx] / 1000000.0;

	if (VERB_ON(VEARL_INFO2) && (sign_app_loop_idx == APP)) {
		verbose_info2_master("Process ending in exclusive mode: %u", exclusive);
	}

	/* Avg CPU frequency */
	metrics->avg_f = cpufreq_avrg[sign_app_loop_idx];

	/* Avg IMC frequency */
	metrics->avg_imc_f = imcfreq_avrg[sign_app_loop_idx];

#if WF_SUPPORT
	/* Temperature data */
	metrics->cpu_sig.devs_count = metrics_get(MET_TEMP)->devs_count;
	temp_data_copy(metrics->cpu_sig.temp, temp_diff[sign_app_loop_idx]);
#endif

	

	/* Time per iteration */
	iterations = ear_max(iterations, 1);
	metrics->time = time_s / (double) iterations;

	/* Cache misses */
	metrics->L1_misses = cache_diff[sign_app_loop_idx].l1d_misses;
	metrics->L2_misses = cache_diff[sign_app_loop_idx].l2_misses;
	metrics->L3_misses = cache_diff[sign_app_loop_idx].l3_misses;

	if (metrics->sig_ext == NULL) {
		metrics->sig_ext = (void *) calloc(1, sizeof(sig_ext_t));
	}
	sig_ext = metrics->sig_ext;

#if USE_GPUS & DCGMI
	if (master && dcgmi_lib_is_enabled() && (sig_ext->dcgmis.set_cnt == 0))
	{
		dcgmi_lib_dcgmi_sig_init(&sig_ext->dcgmis);
    }
#endif
	sig_ext = metrics->sig_ext;
	sig_ext->elapsed = time_s;


	/* FLOPS */
	/* flops_data_accum used in this way just computes the Gflops */
	flops_data_accum(&flops_diff[sign_app_loop_idx], NULL, &metrics->Gflops);
	/* Copies the FP instruction vector in FLOPS */
	flops_help_toold(&flops_diff[sign_app_loop_idx], metrics->FLOPS);

	/* num_th is 1 if perf supports accumulating per thread counters */
	metrics->Gflops = metrics->Gflops * num_th;
	for (uint i = 0; i < FLOPS_EVENTS; i++) metrics->FLOPS[i] = metrics->FLOPS[i] * num_th;

	/* CPI */
	metrics->cycles       = cpi_diff[sign_app_loop_idx].cycles * num_th;;
	metrics->instructions = ear_max(cpi_diff[sign_app_loop_idx].instructions, 1) * num_th;;
	metrics->CPI          = cpi_avrg[sign_app_loop_idx];

	/* TPI and Memory bandwith */
	if (master) {
		bwidth_data_accum(bwidth_diff[APP], bwidth_diff[LOO], NULL, NULL);

	  metrics->TPI = bwidth_help_castotpi(bwidth_cas[sign_app_loop_idx], metrics->instructions);
	  metrics->GBS = bwidth_help_castogbs(bwidth_cas[sign_app_loop_idx], time_s);
    sem_wait(lib_shared_lock_sem);
    lib_shared_region->total_cache_bwidth = cache_bwidth[sign_app_loop_idx];
    sem_post(lib_shared_lock_sem);
	} else {
		bwidth_cas[sign_app_loop_idx] = (ullong) lib_shared_region->cas_counters;
		metrics->GBS = bwidth_help_castogbs(bwidth_cas[sign_app_loop_idx], time_s);
    /* TPI normalized by total instructions */
	  metrics->TPI = bwidth_help_castotpi(bwidth_cas[sign_app_loop_idx], metrics->instructions);
    sem_wait(lib_shared_lock_sem);
    lib_shared_region->total_cache_bwidth += cache_bwidth[sign_app_loop_idx];
    sem_post(lib_shared_lock_sem);
    // verbose(2,"CACHE_BW[%u] local %lf total %lf", my_node_id, cache_bwidth[sign_app_loop_idx], lib_shared_region->total_cache_bwidth);
	}
  verbose_master(TPI_DEBUG, "GBS %.2lf TPI %.2lf", metrics->GBS, metrics->TPI);

  #if USE_CUPTI
  if (read_cupti_metrics){
    gpuproc_data_tostr(proc_gpu_data_diff[sign_app_loop_idx], gpu_proc_buffer, sizeof(gpu_proc_buffer));
    verbose_master(VERBOSE_CUPTI, "GPU PROC data for master: %s", gpu_proc_buffer);
  }
  #endif

	/* Per process IO data */
	io_copy(&(sig_ext->iod), &metrics_io_diff[sign_app_loop_idx]);
	io_copy(&sig_shared_region[my_node_id].sig.iod, &metrics_io_diff[sign_app_loop_idx]);

	/* Per Node IO data */
	int io_app_loop_idx = sign_app_loop_idx;


  /* OS process statistics */

  metric_cond(proc_ps_ok, proc_stat_data_diff(&metrics_proc_end[sign_app_loop_idx], &metrics_proc_init[sign_app_loop_idx], &metrics_proc_diff[sign_app_loop_idx]), VERB_PROCPS, "EARL proc_stat_data_diff failed in compute signature_data");
  metric_cond(proc_ps_ok, proc_stat_data_copy(&(sig_ext->proc_ps), &metrics_proc_diff[sign_app_loop_idx]), VERB_PROCPS, "EARL proc_stat_data_copy in compute_signature");


	/* Each process will compute its own IO_MBS, except
	 * the master, that computes per node IO_MBS */
	if (master) {
		if (io_app_loop_idx == APP) {
			io_app_loop_idx = APP_NODE;
		} else if (io_app_loop_idx == LOO) {
			io_app_loop_idx = LOO_NODE;
		}
	}

	double iogb = (double) metrics_io_diff[io_app_loop_idx].rchar / (double) (1024 * 1024) +
    (double) metrics_io_diff[io_app_loop_idx].wchar / (double) (1024 * 1024);

	metrics->IO_MBS = iogb/time_s;
    #if 0
    if (metrics->IO_MBS > phases_limits.io_th) {
        verbose(1," %s.[%d] With high IO activity %.2lf", node_name, my_node_id, metrics->IO_MBS);
    }
    #endif

	if (master) {
		sig_ext->max_mpi = 0;
		sig_ext->min_mpi = 100;

		/* MPI statistics */
		if (module_mpi_is_enabled()) {
			double mpi_time_perc = 0.0;
			for (int i = 0; i < lib_shared_region->num_processes; i++) {

				ullong mpi_time_usecs = ((mpi_information_t *) metrics_mpi_info[sign_app_loop_idx])[i].mpi_time;
				ullong exec_time_usecs = ear_max(((mpi_information_t *) metrics_mpi_info[sign_app_loop_idx])[i].exec_time, 1);

				// The percentage of time spent in MPI calls is computed on metrics_[partial|global]_stop.
				double local_mpi_time_perc = ((mpi_information_t *) metrics_mpi_info[sign_app_loop_idx])[i].perc_mpi;

				sig_shared_region[i].mpi_info.perc_mpi = local_mpi_time_perc;
				mpi_time_perc += local_mpi_time_perc;

				sig_ext->max_mpi = ear_max(local_mpi_time_perc, sig_ext->max_mpi);
				sig_ext->min_mpi = ear_min(local_mpi_time_perc, sig_ext->min_mpi);

				verbose_master(MPI_VERB,
						"%%MPI process %d: %.2lf | MPI time (us): %llu"
						" | Wall time (us): %llu", i, local_mpi_time_perc,
						mpi_time_usecs, exec_time_usecs);
			}

			metrics->perc_MPI = mpi_time_perc / lib_shared_region->num_processes;

		} else { // MPI not enabled
			metrics->perc_MPI = 0.0;
		}

		sig_ext->mpi_stats = metrics_mpi_info[sign_app_loop_idx];
#if MPI_CALL_TYPES
		sig_ext->mpi_types = metrics_mpi_calls_types[sign_app_loop_idx];
#else
		sig_ext->mpi_types = NULL;
#endif

		/* TALP data */
#if DLB_SUPPORT
			earl_dlb_talp_read(&sig_ext->earl_talp_data);
#endif

		/* Power: Node, DRAM, PCK */
    /* If power is not estimated, we computed here, later otherwise */
    metrics->DC_power = 0;
    /* Loop power */
		if (!estimate_power && acum_ipmi[sign_app_loop_idx] && (sign_app_loop_idx == LOO)){
			metrics->DC_power = (double) acum_ipmi[sign_app_loop_idx] / (time_s * node_energy_units);
		}
    /* App power */
    if (!estimate_power && acum_ipmi[sign_app_loop_idx] && (sign_app_loop_idx == APP)){
			metrics->DC_power = (double) acum_ipmi[sign_app_loop_idx] / (total_acum_time * node_energy_units);
    }


		/* DRAM and PCK PENDING for Njobs */
		metrics->PCK_power=0;
		metrics->DRAM_power=0;

		for (int p = 0; p < num_packs; p++) {
			double rapl_dram = (double) metrics_rapl[sign_app_loop_idx][p];
			metrics->DRAM_power = metrics->DRAM_power + rapl_dram;

			double rapl_pck = (double) metrics_rapl[sign_app_loop_idx][num_packs+p];
			metrics->PCK_power = metrics->PCK_power + rapl_pck;
		}

		metrics->PCK_power   = energy_cpu_compute_power(metrics->PCK_power, time_s);
		metrics->DRAM_power  = energy_cpu_compute_power(metrics->DRAM_power, time_s);
		metrics->EDP = time_s * time_s * metrics->DC_power;
		#if EARL_LIGHT
		verbose_info2_master("computing dram power %.2lf", metrics->DRAM_power);
		verbose_info2_master("computing pck power %.2lf time %lf", metrics->PCK_power, time_s);
		#else
		debug("computing dram power %.2lf (time %.2lf)", metrics->DRAM_power, time_s);
		debug("computing pck power %.2lf (time %.2lf)", metrics->PCK_power, time_s);
		#endif

		float total_gpu_power = 0;
		ulong total_gpu_util  = 0;
#if USE_GPUS
        /* GPUS */
        metrics->gpu_sig.num_gpus = met_gpu.devs_count;
        if (!met_gpu.ok) {
            metrics->gpu_sig.num_gpus = 0;
        }
        verbose_master(GPU_VERB + 1, "Storing signature data for %d GPUs...", metrics->gpu_sig.num_gpus);

        for (int p = 0; p < metrics->gpu_sig.num_gpus; p++) {
            metrics->gpu_sig.gpu_data[p].GPU_power    = gpu_metrics_diff[sign_app_loop_idx][p].power_w;
            metrics->gpu_sig.gpu_data[p].GPU_freq     = gpu_metrics_diff[sign_app_loop_idx][p].freq_gpu;
            metrics->gpu_sig.gpu_data[p].GPU_mem_freq = gpu_metrics_diff[sign_app_loop_idx][p].freq_mem;
            metrics->gpu_sig.gpu_data[p].GPU_util     = gpu_metrics_diff[sign_app_loop_idx][p].util_gpu;
            metrics->gpu_sig.gpu_data[p].GPU_mem_util = gpu_metrics_diff[sign_app_loop_idx][p].util_mem;
#if WF_SUPPORT
            metrics->gpu_sig.gpu_data[p].GPU_temp     = gpu_metrics_diff[sign_app_loop_idx][p].temp_gpu;
            metrics->gpu_sig.gpu_data[p].GPU_temp_mem = gpu_metrics_diff[sign_app_loop_idx][p].temp_mem;
#endif
						total_gpu_util  += metrics->gpu_sig.gpu_data[p].GPU_util;
			      total_gpu_power += metrics->gpu_sig.gpu_data[p].GPU_power;
        }
#if DCGMI
				if (dcgmi_lib_is_enabled())
				{
					/* If there is no utilization we reset the number of instances.
					 * Instances are used by policies */
					if (total_gpu_util == 0)
					{
						dcgmi_lib_reset_instances(&sig_ext->dcgmis);
					}

					if (sign_app_loop_idx == LOO) // Loop signature
					{
						if (state_fail(dcgmi_lib_get_current_metrics(&sig_ext->dcgmis)))
						{
							error_lib("Getting current DCGMI metrics: %s", state_msg);
						}
					} else // App signature
					{
						if (state_fail(dcgmi_lib_get_global_metrics(&sig_ext->dcgmis)))
						{
							error_lib("Getting global DCGMI metrics: %s", state_msg);
						}
					}
					// If WF_SUPPORT disabled, metrics is not filled.
					dcgmi_lib_compute_gpu_gflops(&sig_ext->dcgmis, metrics);
				}
#endif // DCGMI
#endif // USE_GPUS

    if (!estimate_power){
      /* Power error control: computing the ratio */
      dc_power_ratio += metrics->DC_power / (total_gpu_power + metrics->DRAM_power + metrics->PCK_power);
      dc_power_ratio_instances++;
    }

		estimate_metrics_not_available_by_node(sign_app_loop_idx, metrics, procs, total_gpu_power);
#if MPI_OPTIMIZED
    max_app_power = ear_max(max_app_power, metrics->DC_power);
#endif

    if (sign_app_loop_idx == LOO) acum_ipmi[sign_app_loop_idx] = metrics->DC_power * time_s * node_energy_units;


    /* Power error control: We don't accumulate if we are computing application metrics, not loop  metrics */
    if (!lib_shared_region->global_stop_ready){
      //acum_ipmi[APP] += master->DC_power *  se->elapsed * node_energy_units;
      acum_ipmi[APP] += acum_ipmi[LOO];
      total_acum_time += time_s;
      verbose_master(3,"Accumulating %.2lu J to the application. Total energy %lu J acum_time %.2lf sec", acum_ipmi[LOO], acum_ipmi[APP] , total_acum_time);
    }


	} else {
    // Copies DC, DRAM, PCK, avg_f, avg_imc_f, GBS and TPI
    // Node metrics are not needed in shared signature, IF we are
    // collecting traces we don't copy to avoid having node
    // metrics in per-process data. We maintain is tracing is not
    // used since GPU metrics are not computed per-process
    // WARNING: GPU metrics are replicated per-process
    if (!traces_are_on()){
		  copy_node_data(metrics, &lib_shared_region->node_signature);
    }
#if MPI_OPTIMIZED
    max_app_power = ear_max(lib_shared_region->node_signature.DC_power, max_app_power);
#endif
	}


	/* Copying my info in the shared signature */
	ssig_from_signature(&sig_shared_region[my_node_id].sig, metrics);
  /* Computing avg_cpufreq in shared sig (new) */
  cpufreq_process_avgcpufreq(my_node_id, lib_shared_region->avg_cpufreq, &sig_shared_region[my_node_id].sig.avg_f);

	/* If I'm the master, I have to copy in the special section */
	if (master) {
		signature_copy(&lib_shared_region->node_signature, metrics);
		signature_copy(&lib_shared_region->job_signature, metrics);

		if (VERB_ON(2))
		{
			signature_print_simple_fd(verb_channel, metrics);
		}
	}

	sig_shared_region[my_node_id].period = time_s; // TODO: We can ommit this attribute because the shared signature has this metric.

	update_earl_node_mgr_info();

	debug("Signature ready - local rank %d - time %lld", my_node_id, (metrics_time() - app_start)/1000000);
	signature_ready(&sig_shared_region[my_node_id], 0);

  debug("%s signature ready %d", node_name, my_node_id);

    verbose_master(INFO_METRICS + 1,
            "End compute signature for (l. rank) %d. Period %lf secs",
            my_node_id, sig_shared_region[my_node_id].period);
#if 0
	if (msync(sig_shared_region, sizeof(shsignature_t) * lib_shared_region->num_processes,MS_SYNC) < 0) {
		verbose_master(ERROR_VERB, "Memory sync fails: %s", strerror(errno));
	}
  #endif
	if (msync(&sig_shared_region[my_node_id], sizeof(shsignature_t),MS_SYNC|MS_INVALIDATE) < 0) {
		verbose_warning_master("Memory sync fails: %s", strerror(errno));
	}
  

	/* Compute and print the total savings */
  if (sign_app_loop_idx == APP){
    verbose_info_master("Scope %s master %u elapsed %f", (sign_app_loop_idx == APP?"APP":"LOOP"),
                        masters_info.my_master_rank >= 0, sig_ext->telapsed);
  }
	if ((sign_app_loop_idx == APP) && (masters_info.my_master_rank >= 0) && sig_ext->telapsed) {


		float avg_esaving = sig_ext->saving / sig_ext->telapsed;
		float avg_psaving = sig_ext->psaving / sig_ext->telapsed;
		float avg_tpenalty = sig_ext->tpenalty / sig_ext->telapsed;
		verbose_master(ENERGY_EFF_VERB, "Policy savins[%s]. MR[%d] Average estimated energy savings %.2f. Average Power reduction %.2f. Time penalty %.2f. Elapsed %.2f", node_name, masters_info.my_master_rank, avg_esaving, avg_psaving, avg_tpenalty, sig_ext->telapsed);
		
    app_energy_saving.event = ENERGY_SAVING_AVG;

		/* We will use freq for the energy saving */
		app_energy_saving.value = (llong)avg_esaving;
		report_events(&rep_id, &app_energy_saving, 1);

		app_energy_saving.event = POWER_SAVING_AVG;
		app_energy_saving.value = (llong)avg_psaving;
		report_events(&rep_id, &app_energy_saving, 1);
#if 0
		/* Not yet reported */
		app_energy_saving.event = PERF_PENALTY;
#endif
	}

  /* Phases summary */
  if ((sign_app_loop_idx == APP) && (masters_info.my_master_rank >= 0) && (sig_ext != NULL)) {
    ullong ms_total = 0;
    timestamp currt;

    /* Remaining time since last change */
    timestamp_getfast(&currt);
    sig_ext->earl_phase[last_earl_phase_classification].elapsed += timestamp_diff(&currt, &init_last_phase, TIME_USECS);

    /* For COMP_BOUND we compute if CPU or MEM (or mix) */
    if (last_earl_phase_classification == APP_COMP_BOUND){
        ullong elapsed_comp = timestamp_diff(&currt, &init_last_comp, TIME_USECS);
        /* policy_cpu_bound and policy_mem_bound are updated only in the APP_COMP_PHASE */
        if (policy_cpu_bound)      sig_ext->earl_phase[APP_COMP_CPU].elapsed += elapsed_comp;
        else if (policy_mem_bound) sig_ext->earl_phase[APP_COMP_MEM].elapsed += elapsed_comp;
        else                            sig_ext->earl_phase[APP_COMP_MIX].elapsed += elapsed_comp;
    }

    for (uint ph = 0; ph < EARL_BASIC_PHASES; ph ++) ms_total += sig_ext->earl_phase[ph].elapsed;
    for (uint ph = 0; ph < EARL_BASIC_PHASES; ph ++){
      if (sig_ext->earl_phase[ph].elapsed){
        verbose_master(PHASES_SUMMARY_VERB, "Phase[%s] percentage %f (%llu msec/%llu msec)", phase_to_str(ph), (ms_total && sig_ext->earl_phase[ph].elapsed)? (float)sig_ext->earl_phase[ph].elapsed / (float)ms_total : 0.0, sig_ext->earl_phase[ph].elapsed, ms_total);
        app_phase_summary.event = PHASE_SUMMARY_BASE + ph;
        app_phase_summary.value = sig_ext->earl_phase[ph].elapsed;
        report_events(&rep_id, &app_phase_summary, 1);
      }
    }
    for (uint ph = EARL_BASIC_PHASES; ph < EARL_MAX_PHASES; ph++){
      if (sig_ext->earl_phase[ph].elapsed){
        verbose_master(PHASES_SUMMARY_VERB, "Phase[%s] percentage %f (%llu msec/%llu msec)", phase_to_str(ph), (ms_total && sig_ext->earl_phase[ph].elapsed)? (float)sig_ext->earl_phase[ph].elapsed / (float)ms_total : 0.0, sig_ext->earl_phase[ph].elapsed, ms_total);
        app_phase_summary.event = PHASE_SUMMARY_BASE + ph;
        app_phase_summary.value = sig_ext->earl_phase[ph].elapsed;
        report_events(&rep_id, &app_phase_summary, 1);
      }
    }
  }

	if (sign_app_loop_idx == APP) {
		sig_shared_region[my_node_id].exited = 1;
	}
}

/**************************** Init function used in ear_init ******************/
int metrics_load(topology_t *topo)
{

    char *cimc_max_khz,*cimc_min_khz;

	pstate_t tmp_def_imc_pstate;
	state_t st;
	state_t s;
	int i;

    read_metrics_options(); // Read env. vars configuring metrics

	metrics_static_init(topo);
	assert(master >= 0); // Debug if master was well initialized at metrics_static_init
	debug("metrics_static_init done");

	//debug("Masters region %p size %lu",&masters_info,sizeof(masters_info));
	//debug("My master rank %d",masters_info.my_master_rank);

	hw_cache_line_size = topo->cache_line_size;
	num_packs = topo->socket_count;
	topology_copy(&mtopo,topo);

	// debug("detected cache line size: %0.2lf bytes", hw_cache_line_size);
	// debug("detected sockets: %d", num_packs);

	if (hw_cache_line_size == 0) {
		return_msg(EAR_ERROR, "error detecting the cache line size");
	}
	if (num_packs == 0) {
		return_msg(EAR_ERROR, "error detecting number of packges");
	}

	st = energy_lib_init(system_conf);
	if (st != EAR_SUCCESS) {
		verbose_warning("Error returned by energy_lib_init: %s", state_msg);
		return_msg(EAR_ERROR, "Loading energy plugin");
	}

	if (state_ok(io_init(&ctx_io, getpid()))) {
        verbose_info2_master("I/O data initialized.");
	} else {
        verbose_warning_master("I/O data not available.");
    }

	// Allocating data for energy node metrics
	energy_datasize(&node_energy_datasize);
	debug("energy data size %lu", node_energy_datasize);
	energy_units(&node_energy_units);
	/* We should create a data_alloc for enerrgy and a set_null */
	energy_data_alloc(&aux_energy);
	energy_data_alloc(&aux_energy_stop);
	energy_data_alloc(&metrics_ipmi[LOO]);
	energy_data_alloc(&metrics_ipmi[APP]);
	acum_ipmi[LOO] = 0;acum_ipmi[APP] = 0;
	total_acum_time = 0;

	if (master) {


		debug("Allocating data for %d processes", lib_shared_region->num_processes);

		/* We will collect MPI statistics */
		metrics_mpi_info[APP]             = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
		metrics_last_mpi_info[APP]        = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
		metrics_mpi_info[LOO]             = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
		metrics_last_mpi_info[LOO]        = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
#if MPI_CALL_TYPES
		metrics_mpi_calls_types[APP]      = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
		metrics_last_mpi_calls_types[APP] = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
		metrics_mpi_calls_types[LOO]      = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
		metrics_last_mpi_calls_types[LOO] = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
#endif

		/* IMC management */
		// if (mgt_imcfreq.ok) {

			imc_devices = mgt_imcfreq.devs_count;

			imc_max_pstate = calloc(mgt_imcfreq.devs_count, sizeof(uint));
			imc_min_pstate = calloc(mgt_imcfreq.devs_count, sizeof(uint));

			if ((imc_max_pstate == NULL) || (imc_min_pstate == NULL)){
				error_lib("Allocating memory for IMC P-states list.");
				return EAR_ERROR;
			}

			if (state_fail(s = mgt_imcfreq_get_available_list(NULL, (const pstate_t **)&imc_pstates, &imc_num_pstates))){
				error_lib("Asking for IMC frequency list.");
			}

#if 0
			mgt_imcfreq_data_tostr(imc_pstates, imc_num_pstates, imc_buffer, sizeof(imc_buffer));
			verbose_master(2, "IMC pstates list:\n%s", imc_buffer);
#endif

			/* Default values */
			def_imc_max_khz = imc_pstates[0].khz;
			def_imc_min_khz = imc_pstates[imc_num_pstates-1].khz;
			for (i=0; i< mgt_imcfreq.devs_count;i++) imc_min_pstate[i] = 0;
			for (i=0; i< mgt_imcfreq.devs_count;i++) imc_max_pstate[i]= imc_num_pstates-1;

			/* This section checks for explicitly requested IMC freq. range */

			cimc_max_khz = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_MAX_IMCFREQ) : NULL; // Max. IMC freq. (kHz)
			cimc_min_khz = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_MIN_IMCFREQ) : NULL; // Min. IMC freq. (kHz)

			imc_max_khz = def_imc_max_khz;
			imc_min_khz = def_imc_min_khz;

			if (cimc_max_khz != (char*)NULL){
				ulong tmp_def_imc = atoi(cimc_max_khz);
				if (state_ok(pstate_freqtops(imc_pstates,imc_num_pstates,tmp_def_imc,&tmp_def_imc_pstate))){
					for (i=0; i< mgt_imcfreq.devs_count;i++) imc_min_pstate[i] = tmp_def_imc_pstate.idx;
					/* We are assuming that all devices work at the same IMC freq. */
					imc_max_khz = imc_pstates[imc_min_pstate[0]].khz;
				}
			}
			if (cimc_min_khz != (char*)NULL){
				ulong tmp_def_imc = atoi(cimc_min_khz);
				if (state_ok(pstate_freqtops(imc_pstates,imc_num_pstates,tmp_def_imc,&tmp_def_imc_pstate))){
					for (i=0; i< mgt_imcfreq.devs_count;i++) imc_max_pstate[i] = tmp_def_imc_pstate.idx;
					/* We are assuming that all devices ork at the same IMC freq. */
					imc_min_khz = imc_pstates[imc_max_pstate[0]].khz;
				}
			}

			/* End sets */
			if (state_fail(s = mgt_imcfreq_set_current_ranged_list(NULL, imc_min_pstate, imc_max_pstate))) {
				error_lib("Setting the IMC frequency.");
			}

			/* We assume all the sockets have the same configuration */
			imc_curr_max_pstate = imc_max_pstate[0];
			imc_curr_min_pstate = imc_min_pstate[0];	

			// Verbose a resume
			if (verb_level >= INFO_METRICS + 1) {
				verbose_master(INFO_METRICS + 1, "\n           --- IMC info ---\n            #devices: %u", mgt_imcfreq.devs_count);

				if (cimc_max_khz != NULL) {
					verbose_master(INFO_METRICS + 1, "              Maximum: %s kHz", cimc_max_khz);
				}

				if (cimc_min_khz != NULL) {
					verbose_master(INFO_METRICS + 1, "             Minimum: %s kHz", cimc_min_khz);
				}

				verbose_master(INFO_METRICS + 1, "Init (max/min) (kHz): %.2f/%.2f\n           ----------------\n",
						(float) imc_pstates[imc_min_pstate[0]].khz, (float) imc_pstates[imc_max_pstate[0]].khz);
			}

		// } else {
		// 	verbose_warning_master("IMCFREQ not supported.");
		// } // IMC
	} // master

#if USE_GPUS
    fflush(stderr);	
#endif

	metrics_global_start();
	metrics_partial_start();

	return EAR_SUCCESS;
}


#define MAX_SYNCHO_TIME 100


void metrics_dispose(signature_t *metrics, ulong procs)
{
	sig_ext_t *se;
	signature_t job_node_sig;
  signature_t last_loop_sig;
	uint num_ready;
	float wait_time = 0;


#if METRICS_OVH
    //verbose(0,"[%d] Total usecs in metrics signature finish %lu", my_node_id, total_metrics_ovh);
    if (masters_info.my_master_rank >= 0) overhead_report(1);
#endif

    // metrics->sig_ext = NULL;
    signature_copy(&last_loop_sig, metrics);
    last_loop_sig.sig_ext = NULL;

    ullong time_st = timestamp_getconvert(TIME_MSECS);
    verbosen(INFO_METRICS + 1, "--- %llu Metrics dispose init (l. rank) %d ---", time_st, my_node_id);

	if (!eards_connected() && master) {
		if (!EARL_LIGHT){ 
			return;
		}
	}

        verbose_info2_master("Calling last partial stop...");

        /* Synchro 1, waiting for last loop data: partial stop: master will wait if. Others will wait for master signature */
        state_t ret = metrics_partial_stop(SIG_BEGIN);
        while (ret == EAR_NOT_READY && wait_time < MAX_SYNCHO_TIME && lib_shared_region->earl_on)
        { 
            usleep(100);

            wait_time += 0.01;
            ret = metrics_partial_stop(SIG_BEGIN);
        }

        if (!lib_shared_region->earl_on){ 
            return;
        }

        //verbose(0, "Computing last signature data");
        metrics_compute_signature_data(LOO, &last_loop_sig, 1, procs);

	/* Waiting for node signatures */
	verbose_master(INFO_METRICS+1, "(l.rank) %d: Waiting for loop node signatures", my_node_id);

	/* Synchro 2: Waiting for last local loops signatures.
     *
     * Slaves wait master to be ready for the global stop.
     *
     * Control variables:
     *  - wait_time
     *  - two_or_more_processes_tobe_exited 
     *  - all_signs_ready
     */
	wait_time = 0;

	uint exited = num_processes_exited(lib_shared_region,sig_shared_region);
    uint two_or_more_processes_tobe_exited = exited < (lib_shared_region->num_processes - 1);

    uint all_signs_ready = are_signatures_ready(lib_shared_region, sig_shared_region, &num_ready);

	while (two_or_more_processes_tobe_exited && !all_signs_ready && (wait_time < MAX_SYNCHO_TIME))
    {
		if (!(master) && lib_shared_region->global_stop_ready) {
			break;
		}

        // Why do we have this condition ? if just one process is pending to be exited, the loop is not entered.
		if (master && (exited == lib_shared_region->num_processes - 1)) {
			break;
		}

		usleep(100);

		exited = num_processes_exited(lib_shared_region, sig_shared_region);
        two_or_more_processes_tobe_exited = exited < (lib_shared_region->num_processes - 1);

        all_signs_ready = are_signatures_ready(lib_shared_region, sig_shared_region, &num_ready);

		wait_time += 0.01;
	}

	// Only master does something in this funcion
	compute_per_process_and_job_metrics(&job_node_sig);

	/* At this point we mark signatures are not ready to be computed again */
	if (master)  {
		free_node_signatures(lib_shared_region,sig_shared_region);
		lib_shared_region->global_stop_ready = 1;
	}

	debug("Global stop");
	metrics_global_stop();
	debug("metrics_compute_signature_data APP");
	metrics_compute_signature_data(APP, metrics, 1, procs);

	/* Waiting for node signatures */
	//verbose(2, "(l. rank) %d: Waiting for app node signatures", my_node_id);
	/* Synchro 3: Waiting for app signature: global stop */
	wait_time = 0;
	exited = num_processes_exited(lib_shared_region,sig_shared_region);
	while ( (exited != lib_shared_region->num_processes) && (are_signatures_ready(lib_shared_region,sig_shared_region, &num_ready) == 0) 
			&& (wait_time < MAX_SYNCHO_TIME) ){ 
		usleep(100);
		wait_time += 0.01;
		exited = num_processes_exited(lib_shared_region,sig_shared_region);
	}

	/* Estimate local metrics, accumulate */
	if (master) {
	  debug("Computing application node signature");

    /* This function estimates GBs based on cache */
    estimate_metrics_not_available_by_processes(metrics);

		se = (sig_ext_t *) metrics->sig_ext;

		if (!exclusive) {
			metrics_app_node_signature(metrics, &job_node_sig);
		} else {
			metrics_job_signature(&lib_shared_region->job_signature, &job_node_sig);
		}

		signature_copy(metrics, &job_node_sig);
		metrics->sig_ext = se;
	}

	metrics_static_dispose();
	debug("Metrics_dispose_end %d", my_node_id);
}


void metrics_compute_signature_begin()
{
	metrics_partial_stop(SIG_BEGIN);
	metrics_partial_start();
}


int time_ready_signature(ulong min_time_us)
{
	llong f_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);
	if (f_time < min_time_us) {
		return 0;
	}
	return 1;
}

static llong last_sig_elapsed = 0;
static uint  last_iterations  = 0;
float metrics_iter_per_second()
{
  float elap_sec = (float)last_sig_elapsed / 1000000.0;
  if (!last_sig_elapsed || !last_iterations) return 1;
  return (float) last_iterations / elap_sec;
}


/**********************************************************************************************
 ******************* This function checks if data is ready to be shared ***********************
 **********************************************************************************************/

state_t metrics_compute_signature_finish(signature_t *metrics, uint iterations,
		ulong min_time_us, ulong procs, llong *passed_time)
{
    #if METRICS_OVH
    overhead_start(id_ovh_partial_stop);
    partial_stop_cnt++;
    #endif


	debug("metrics_compute_signature_finish %u", my_node_id);
  debug("%s :metrics_compute_signature_finish iterations %u ready %u master_ready %u", node_name, iterations, sig_shared_region[my_node_id].ready, lib_shared_region->master_ready);

	if (!sig_shared_region[my_node_id].ready) {

		debug("Signature for proc. %d is not ready", my_node_id);

		// Time requirements
		llong elap_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);

    *passed_time = elap_time;

		uint time_eq = equal_with_th((double) elap_time, (double) min_time_us, 0.1);
		if (elap_time < min_time_us && !time_eq) {
			metrics->time = 0;
			debug("%s : EAR_NOT_READY because of time %lld, iters %u (start %lld end %lld)\n", node_name, elap_time, iterations, metrics_usecs[LOO], metrics_time());
      #if METRICS_OVH
      overhead_stop(id_ovh_partial_stop);
      #endif
			return EAR_NOT_READY;
		}

		// Master: Returns not ready when power is not ready
		// Not master: when master signature not ready
		if (metrics_partial_stop(SIG_END) == EAR_NOT_READY) {
      debug("%s: EAR_NOT_READY 2", node_name);
      #if METRICS_OVH
      overhead_stop(id_ovh_partial_stop);
      #endif
			return EAR_NOT_READY;
		}
    #if METRICS_OVH
    overhead_stop(id_ovh_partial_stop);
    overhead_start(id_ovh_compute_data);
    compute_sig_cnt++;
    #endif

    last_sig_elapsed = elap_time;
    last_iterations  = iterations;

		// Marks the signature as ready
		metrics_compute_signature_data(LOO, metrics, iterations, procs);

    #if METRICS_OVH
    overhead_stop(id_ovh_compute_data);
    #endif

		// Call report mpitrace
		report_id_t report_id;
		report_create_id(&report_id, my_node_id,
                         ear_my_rank, masters_info.my_master_rank);

		if (state_fail(report_misc(&report_id, MPITRACE,
                                   (cchar *) &sig_shared_region[my_node_id], -1)))
        {
			verbose(3, "%sERROR%s Reporting mpitrace for process %d",
                    COL_RED, COL_CLR, my_node_id);
		}

    // Report node metrics
    if (state_fail(report_misc(&report_id, MPI_NODE_METRICS,
                                   (cchar *) metrics->sig_ext,
                                   lib_shared_region->num_processes)))
        {
			verbose_master(3, "%sERROR%s Reporting mpi node metrics.[%d]",
                    COL_RED, COL_CLR, my_node_id);
		}

		metrics_partial_start();
	}
#if SHOW_DEBUGS
	else {
		debug("Signature for proc. %d is ready", my_node_id);
	}
#endif

	uint num_ready;
	if (!are_signatures_ready(lib_shared_region, sig_shared_region, &num_ready)) {
		if ((master) && (sig_shared_region[0].ready)) {
			debug("Setting Master ready");
			lib_shared_region->master_ready = 1;
		}
		return EAR_NOT_READY;
	} else if (master) {
		lib_shared_region->master_ready = 0;
	}

	return EAR_SUCCESS;
}


long long metrics_usecs_diff(long long end, long long init)
{
	long long to_max;

	if (end < init)
	{
		debug( "Timer overflow (end: %lld - init: %lld)\n", end, init);
		to_max = LLONG_MAX - init;
		return (to_max + end);
	}

	return (end - init);
}


void metrics_app_node_signature(signature_t *master,signature_t *ns)
{
	ullong inst, max_inst = 0;
	ullong cycles;
	ullong FLOPS[FLOPS_EVENTS];
	sig_ext_t *se;
	int i;
	double valid_period;
	ullong L1, L2, L3;
  ullong accesses = 0;
  double io_mbs;

	se = (sig_ext_t *)master->sig_ext;
	signature_copy(ns,master);
	if (se != NULL) memcpy(ns->sig_ext,se,sizeof(sig_ext_t));

	debug(" metrics_app_node_signature");

	compute_job_node_instructions(sig_shared_region, lib_shared_region->num_processes, &inst);
	compute_job_node_cycles(sig_shared_region, lib_shared_region->num_processes, &cycles);
	compute_job_node_io_mbs(sig_shared_region, lib_shared_region->num_processes, &io_mbs);

	ns->CPI = (inst ? (double) cycles / (double) inst : 1);
  ns->IO_MBS = io_mbs;

	compute_job_node_flops(sig_shared_region, lib_shared_region->num_processes, FLOPS);

	compute_job_node_L1_misses(sig_shared_region, lib_shared_region->num_processes, &L1);
	compute_job_node_L2_misses(sig_shared_region, lib_shared_region->num_processes, &L2);
	compute_job_node_L3_misses(sig_shared_region, lib_shared_region->num_processes, &L3);

	for (i = 0; i < FLOPS_EVENTS; i++){
		ns->FLOPS[i] = FLOPS[i];
	}    

	if (state_fail(compute_job_node_gflops(sig_shared_region, lib_shared_region->num_processes, &ns->Gflops))) {
		verbose_warning("Error on computing job's node GFLOP/s rate: %s", state_msg);
	}

	ns->L1_misses  = L1;
	ns->L2_misses  = L2;
	ns->L3_misses  = L3;
	ns->cycles     = cycles;
	ns->instructions = inst;


	uint tcpus     = 0;
	ns->DC_power   = 0;
	ns->GBS        = 0;
	ns->avg_f      = 0;
	ns->DRAM_power = 0;
	ns->PCK_power  = 0;
	if (master){
		for (uint lp = 0; lp < lib_shared_region->num_processes; lp ++){
			valid_period    = sig_shared_region[lp].sig.valid_time;
      max_inst        = ear_max(max_inst, sig_shared_region[lp].sig.instructions);
			if (valid_period){
				ns->DC_power   += sig_shared_region[lp].sig.accum_energy / valid_period;
				ns->DRAM_power += sig_shared_region[lp].sig.accum_dram_energy / valid_period;
				ns->PCK_power  += sig_shared_region[lp].sig.accum_pack_energy / valid_period;
				ns->GBS        += sig_shared_region[lp].sig.accum_mem_access / (valid_period * B_TO_GB);
				ns->avg_f      += (sig_shared_region[lp].sig.accum_avg_f * sig_shared_region[lp].num_cpus) / (ulong)valid_period;
			}
			tcpus          += sig_shared_region[lp].num_cpus;
      accesses       += sig_shared_region[lp].sig.accum_mem_access;
			if (valid_period){
			debug(" p[%u]: Power %lf GBs %lf avg_f %lu dram %lf pck %lf  period %lf", lp, (double)(sig_shared_region[lp].sig.accum_energy / valid_period), (double)(sig_shared_region[lp].sig.accum_mem_access/ (valid_period * 1024 * 1024 * 1024)), sig_shared_region[lp].sig.accum_avg_f/(ulong)valid_period, ns->DRAM_power, ns->PCK_power, valid_period);
			}
		}
		ns->avg_f  = (tcpus ? ns->avg_f/ (double)tcpus : ns->avg_f);
    ns->TPI    = (max_inst ? (double)accesses / (double)max_inst: 1);
    /* TPI should be computed based on total instructions */
		verbose_master(TPI_DEBUG,"AVG  %lu TPI %.2lf (accesses %llu, inst %llu) ", ns->avg_f, ns->TPI, accesses, inst);
	}
	signature_copy(&lib_shared_region->node_signature,ns);

}

void metrics_job_signature(const signature_t *master, signature_t *dst)
{
	ullong inst;
	ullong cycles;
	ullong FLOPS[FLOPS_EVENTS];
	ullong L1, L2, L3;

	signature_copy(dst, master);

	/* If some job node computation fails, we use the CPI data we had from \p master */
	if (state_fail(compute_job_node_instructions(sig_shared_region, lib_shared_region->num_processes, &inst))) {
		verbose_warning("Error on computing job's node instructions: %s", state_msg);
	}
	else if (state_fail(compute_job_node_cycles(sig_shared_region, lib_shared_region->num_processes, &cycles))) {
		verbose_warning("Error on computing job's node cycles: %s", state_msg);
	} else {

		dst->cycles       = cycles;
		dst->instructions = inst;

		assert(inst != 0);
		dst->CPI = (double) cycles / (double) inst;

	}

	if (state_fail(compute_job_node_flops(sig_shared_region, lib_shared_region->num_processes, FLOPS))) {
		verbose_warning("Error on computing job's node FLOPs events: %s", state_msg);
	} else {
		for (uint i = 0; i < FLOPS_EVENTS; i++) {
			dst->FLOPS[i] = FLOPS[i];
		}
	}

	if (state_fail(compute_job_node_gflops(sig_shared_region, lib_shared_region->num_processes, &dst->Gflops))) {
		verbose_warning("Error on computing job's node GFLOP/s rate: %s", state_msg);
	}

  compute_job_node_io_mbs(sig_shared_region, lib_shared_region->num_processes, &dst->IO_MBS);

	compute_job_node_L1_misses(sig_shared_region, lib_shared_region->num_processes, &L1);
	compute_job_node_L2_misses(sig_shared_region, lib_shared_region->num_processes, &L2);
	compute_job_node_L3_misses(sig_shared_region, lib_shared_region->num_processes, &L3);

	dst->L1_misses = L1;
	dst->L2_misses = L2;
	dst->L3_misses = L3;

}

extern uint last_earl_phase_classification;
extern gpu_state_t last_gpu_state;

/* This function returns EAR_SUCCESS when metrics are ready to be compared */
state_t metrics_new_iteration(signature_t *sig)
{
    
    if (!(master)) {
        return EAR_ERROR;
    }


	uint last_phase_io_bnd = (last_earl_phase_classification == APP_IO_BOUND);
	uint last_phase_bw     = (last_earl_phase_classification == APP_BUSY_WAITING);
  llong elap_sec = 0;

	if ((last_phase_io_bnd || last_phase_bw) || (last_gpu_state & _GPU_Idle)){
		llong f_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);
		elap_sec = f_time / 1000000;

		if (elap_sec <= last_elap_sec) {
			return EAR_ERROR;
		}
  }

  last_elap_sec = elap_sec;
	if (last_phase_io_bnd || last_phase_bw)
	{
    validation_new_cpu++;

    /* We should check is we are still on a IO phase */
        if (last_phase_io_bnd){
            double iogb;
            metrics_compute_total_io(&metrics_io_end[LOO_NODE]);
            io_diff(&metrics_io_diff[LOO_NODE], &metrics_io_init[LOO_NODE], &metrics_io_end[LOO_NODE]);
            iogb = (double) metrics_io_diff[LOO_NODE].rchar / (double) (1024 * 1024) +
                (double) metrics_io_diff[LOO_NODE].wchar / (double) (1024 * 1024);

            sig->IO_MBS = iogb/elap_sec;

            verbose_info2_master("IO phase and current IO rate %.2lf (IO? %u)",
                                    sig->IO_MBS, sig->IO_MBS > phases_limits.io_th);
            return EAR_SUCCESS;
        }

    /* We should check the CPI */
    if (last_phase_bw){
      cpi_read_diff(no_ctx,   &cpi_read2[LOO],   &cpi_read1[LOO],   &cpi_diff[LOO],   &cpi_avrg[LOO]);
      sig->CPI = cpi_avrg[LOO];
      verbose_master(3, "Busy Waiting phase and current CPI %.2lf (BW? %u)", sig->CPI , sig->CPI < phases_limits.cpi_busy_waiting);
      return EAR_SUCCESS;
    }    
        // TODO: We changed the approach, so this message should change.
		debug("--- Computing flops because of application phase ---\n"
				"%-26s: %d\n%-26s: %d\n"
				"%-26s: %lld\n%-26s: %lld\n"
				"----------------------------------------------------",
				"IO", last_phase_io_bnd, "Busy waiting", last_phase_bw,
				"Time elapsed", elap_sec, "Last elapsed", last_elap_sec);


	    /* FLOPS are used to determine the CPU BUSY waiting
         *  WARNING: Currently below code won't be executed never. */
		flops_read_diff(no_ctx, &flops_read2[LOO],
				&flops_read1[LOO], &flops_diff[LOO], &sig->Gflops);
		flops_help_toold(&flops_diff[LOO], sig->FLOPS);

		sig->time = elap_sec;
		debug("GFlops in metrics for validation: %lf", sig->Gflops);
	}

#if USE_GPUS
    sig->gpu_sig.num_gpus = 0;
    if (last_gpu_state & _GPU_Idle) {
        validation_new_gpu++;

        debug("Computing GPU util because of application gpu phase is IDLE");

        /* GPUs */
        if (state_fail(gpu_read(no_ctx, gpu_metrics_busy))) {
            verbose_warning("Error reading GPU data on new iteration.");
        }
        gpu_data_diff(gpu_metrics_busy, gpu_metrics_read1[LOO], gpu_metrics_busy_diff);

        sig->gpu_sig.num_gpus = met_gpu.devs_count;

        if (!met_gpu.ok) {
            debug("Setting num_gpu to 0 because model is DUMMY");
            sig->gpu_sig.num_gpus = 0;
        }
        for (int i = 0; i < sig->gpu_sig.num_gpus; i++) {
            sig->gpu_sig.gpu_data[i].GPU_power    = gpu_metrics_busy_diff[i].power_w;
            sig->gpu_sig.gpu_data[i].GPU_freq     = gpu_metrics_busy_diff[i].freq_gpu;
            sig->gpu_sig.gpu_data[i].GPU_mem_freq = gpu_metrics_busy_diff[i].freq_mem;
            sig->gpu_sig.gpu_data[i].GPU_util     = gpu_metrics_busy_diff[i].util_gpu;
            sig->gpu_sig.gpu_data[i].GPU_mem_util = gpu_metrics_busy_diff[i].util_mem;
#if WF_SUPPORT
						sig->gpu_sig.gpu_data[i].GPU_temp     = gpu_metrics_busy_diff[i].temp_gpu;
						sig->gpu_sig.gpu_data[i].GPU_temp_mem = gpu_metrics_busy_diff[i].temp_mem;
#endif
        }
    }
#endif // USE_GPUS
    return EAR_SUCCESS;
}

/* This function computes local metrics per process,
 * computes per job and computes the accumulated. */
void compute_per_process_and_job_metrics(signature_t *dest)
{

	if (master) {
    estimate_metrics_not_available_by_processes(dest);
		update_earl_node_mgr_info();
		estimate_power_and_gbs(lib_shared_region, sig_shared_region, node_mgr_job_info);
		accum_estimations(lib_shared_region, sig_shared_region);
		signature_copy(dest, &lib_shared_region->job_signature);
	}
}


#if MPI_OPTIMIZED
static int fd_csv_cpu;
static uint report_power_id = 0;
static timestamp last_cpupower;
void metrics_start_cpupower()
{
  if (master && report_power_trace) {
    char filename[316];
    timestamp_getfast(&last_cpupower);
    energy_cpu_data_alloc(no_ctx, &metrics_rapl_fine_monitor_start, &rapl_size);
    energy_cpu_data_alloc(no_ctx, &metrics_rapl_fine_monitor_end, &rapl_size);
    energy_cpu_data_alloc(no_ctx, &metrics_rapl_fine_monitor_diff, &rapl_size);
    energy_cpu_read(no_ctx, metrics_rapl_fine_monitor_start);
    if (!traces_are_on()){
      snprintf(filename, sizeof(filename), "csv_cpu_power.%lu.%lu.%s.csv", application.job.id, application.job.step_id, application.node_id);
      fd_csv_cpu = open(filename, O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR);
      if (fd_csv_cpu < 0 ){
        verbose_master(0,"Error opening csv_cpu_power.csv file (%s)", strerror(errno));
        fd_csv_cpu = verb_channel;
      }
    }else{
      verbose_master(0,"CPU power metrics will be generated in traces");
    }
  }
}

void metrics_read_cpupower()
{
  if (master && report_power_trace) {

    char buffer[64], csv_str[512];
    double total = 0, curr ;
    timestamp currt;
    ullong elap;
    char check[256];

    timestamp_getfast(&currt);
    elap = timestamp_diff(&currt, &last_cpupower, TIME_MSECS);
    last_cpupower = currt;



    energy_cpu_read(no_ctx, metrics_rapl_fine_monitor_end);
    energy_cpu_data_diff(no_ctx, metrics_rapl_fine_monitor_start, metrics_rapl_fine_monitor_end, metrics_rapl_fine_monitor_diff);
    energy_cpu_data_copy(no_ctx, metrics_rapl_fine_monitor_start, metrics_rapl_fine_monitor_end); 
    energy_cpu_to_str(NULL, check, metrics_rapl_fine_monitor_diff);
    //verbose_master(2, "CPUPOW[%s]: elapsed %llu (%s)", application.node_id, elap, check);

    if (!traces_are_on()){
      snprintf(csv_str, sizeof(csv_str), "%s;%u;%lld;",application.node_id, report_power_id, timestamp_getconvert(TIME_USECS));
    }
    report_power_id++;

    ullong lpck = 0, ldram = 0;
    // num_packs is socket_count and there are 2 events. DRAM0, DRAM1, PCK0, PCK1 is the
    // default use case
    // elap is un MSEC and energy in nanoJoules
    // ldram and lpck is multiplied to be used by Paraver
    for (int p = 0; p < (num_packs*2) -1 ; p++) {
      curr = energy_cpu_compute_power((double)metrics_rapl_fine_monitor_diff[p], (double)elap/1000.0);
      if (p < num_packs){
        ldram += (ullong)curr*1000;
      }else{
        lpck  += (ullong)curr*1000;
      }
      if (!traces_are_on()){
        snprintf(buffer, sizeof(buffer), "%.2lf;", curr);
        strcat( csv_str, buffer);
      }
      total += curr;
    }
    
    curr = energy_cpu_compute_power((double)metrics_rapl_fine_monitor_diff[(num_packs*2) -1], (double)elap/1000.0);
    lpck  += (ullong)curr*1000;
    total += curr;
    if (traces_are_on()){
        traces_generic_event(ear_my_rank, my_node_id, TRA_PCK_POWER_NODE, lpck);
        traces_generic_event(ear_my_rank, my_node_id, TRA_DRAM_POWER_NODE, ldram);
        //verbose_master(2,"Reporting event PCK %llu DRAM %llu", lpck, ldram);
    }else{
      snprintf(buffer, sizeof(buffer), "%.2lf;%.2lf\n", curr, total);
      strcat(csv_str, buffer);
      write(fd_csv_cpu, csv_str, strlen(csv_str));
    }
    max_app_power = ear_max(max_app_power, total);
  }
}
#endif // MPI_OPTIMIZED

/** Compute the total node I/O data. */
static state_t metrics_compute_total_io(io_data_t *total)
{
    if (total != NULL) {

        memset((char *) total, 0, sizeof(io_data_t));

        debug("Reading IO node data - #processes: %d", lib_shared_region->num_processes);

        for (int i = 0; i < lib_shared_region->num_processes; i++) {

            ctx_t local_io_ctx;

            if (state_ok(io_init(&local_io_ctx, sig_shared_region[i].pid))) {

                io_data_t io;
                io_read(&local_io_ctx, &io);

                total->rchar       += io.rchar;
                total->wchar       += io.wchar;
                total->syscr       += io.syscr;
                total->syscw       += io.syscw;
                total->read_bytes  += io.read_bytes;
                total->write_bytes += io.write_bytes;
                total->cancelled   += io.cancelled;

                io_dispose(&local_io_ctx);
            } else {
                debug("IO data process %d (%d) can not be read", i, sig_shared_region[i].pid);
                return_msg(EAR_ERROR, "I/O could not be read for some process.");
            }
        }

        debug("Node IO data ready - master: %d", masters_info.my_master_rank);

        return EAR_SUCCESS;
    } else {
		return_msg(EAR_ERROR, Generr.input_null);
    }
}


/* Update the description of this function (at the top) if you modify it. */
static void read_metrics_options()
{
#if USE_CUPTI
    char *cuse_cupti;
    /* We will get per-process cupti metrics */
    cuse_cupti = ear_getenv(EAR_USE_CUPTI);
    if (cuse_cupti != NULL) read_cupti_metrics = atoi(cuse_cupti);
    verbose_master(VERBOSE_CUPTI, "CUPTI metrics: %s", (read_cupti_metrics ? "Enabled" : "Disabled"));
#endif

#if MPI_OPTIMIZED
    /* this is already read in policy.c but RAPL traces will be generated only if this option is ON */
    char *cear_mpi_opt          =  ear_getenv(FLAG_MPI_OPT);
    if (cear_mpi_opt != NULL){
      uint opt = atoi(cear_mpi_opt);
      if (opt && (getenv(EAR_REPORT_POWER_TRACE) != NULL)){
        report_power_trace = atoi(getenv(EAR_REPORT_POWER_TRACE));
      }
      verbose_master(0,"RAPL power traces %s", (report_power_trace?"Enabled":"Disabled"));
    }
#endif
}

static state_t energy_lib_init(settings_conf_t *conf)
{
	int read_env = (conf->user_type == AUTHORIZED || USER_TEST);
	char *hack_energy_plugin_path =
	    (read_env) ? ear_getenv(HACK_ENERGY_PLUGIN) : NULL;

	char my_plug_path[SZ_PATH];

	state_t ret = EAR_ERROR;

	if (hack_energy_plugin_path) {
		ret = utils_build_valid_plugin_path(my_plug_path, sizeof(my_plug_path), "energy", hack_energy_plugin_path, conf);
	}

	if (state_fail(ret)) {
		ret = utils_build_valid_plugin_path(my_plug_path, sizeof(my_plug_path), "energy", conf->installation.obj_ener, conf);
		if (state_fail(ret)) {
			verbose_error_master("Energy plug-in %s not found.",
							 conf->installation.obj_ener);
			return EAR_NOT_FOUND;
		}
	}

	/* master is defined in metrics_static_init */
	// int eard = master;

	ret = energy_node_load(my_plug_path, master);

	verbose_master(2, "Energy plugin %s loaded with success: %u",
		       my_plug_path, state_ok(ret));

	// TODO: if ret fails, generate dummy path and load that one
	return ret;
}

static void metrics_static_dispose()
{
	energy_node_dispose();
	temp_dispose();
}


/** Use this function to compute the total number of threads if neded  */
#define VERB_NUMTH 3
int ear_get_num_threads()
{
  verbose(VERB_NUMTH, "EARL: Testing new threads");
  folder_t ltasks;
  char miproc[256];
  char *my_th;
  int num_th = 0; 


	#if ESTIMATE_PERF == 0
	return 1;
	#endif
  snprintf(miproc,sizeof(miproc),"/proc/%d/task/",getpid());
  verbose(VERB_NUMTH,"Testing %s folder", miproc);

  /* Open task folder */
  if (state_fail(folder_open(&ltasks, miproc))){
    verbose(VERB_NUMTH,"Task folder open fail");
    return 1;
  }

  /* Iterate */
  while ((my_th = folder_getnextdir(&ltasks,NULL, NULL)))
  {
    // verbose(VERB_NUMTH, "Task[%d] th_id '%s'\n", getpid(), my_th);
    num_th++;
  }
	verbose(VERB_NUMTH,"Task[%d] %d threads detected", getpid(), num_th);
	return num_th;
}
