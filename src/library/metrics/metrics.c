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

// #define SHOW_DEBUGS            1
#define INFO_METRICS           2
#define IO_VERB                3
#define SET_DYN_UNC_BY_DEFAULT 1
#define MPI_VERB               3

#if !SHOW_DEBUGS
#define NDEBUG // Disable asserts
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/mman.h>
#include <assert.h>

#include <common/config.h>
#include <common/states.h>
#include <common/system/time.h>
#include <common/output/verbose.h>
#include <common/types/signature.h>
#include <common/math_operations.h>
#include <common/hardware/hardware_info.h>

#include <library/api/clasify.h>
#include <library/loader/module_cuda.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/common/library_shared_data.h>
#include <library/metrics/metrics.h>
#include <library/metrics/energy_node_lib.h>
#include <library/policies/common/mpi_stats_support.h> /* Should we move this file out out policies? */
#if USE_GPUS
#include <library/metrics/gpu.h>
#endif

#include <metrics/io/io.h>
#include <metrics/cpi/cpi.h>
#include <metrics/cache/cache.h>
#include <metrics/flops/flops.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/bandwidth/bandwidth.h>
#include <management/cpufreq/cpufreq.h>
#include <management/imcfreq/imcfreq.h>
#include <daemon/local_api/eard_api.h>

#include <report/report.h>

// Registers
#define LOO         0 // loop
#define APP         1 // application
#define LOO_NODE    2 // We will share the same pos than for accumulated values
#define APP_NODE    3
#define APP_ACUM    4 // Used to store the accumulated values for metrics which also use
                      // LOO_NODE position
#define SIG_BEGIN 	0
#define SIG_END		1

// #define MASTER(master_rank) (master_rank >= 0)

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
static ull        *aux_rapl;         // nJ (vec)
static ull        *last_rapl;        // nJ (vec)
static ulong       rapl_size;
static llong       aux_time;
static edata_t     aux_energy;
static edata_t     metrics_ipmi[2];  // mJ
static ulong       acum_ipmi[2];
static edata_t     aux_energy_stop;
static llong       metrics_usecs[2]; // uS

/* In the case of IO we will collect per process, per loop,
 * per node, and per-loop node accumulated. */
static io_data_t   metrics_io_init[4];
static io_data_t   metrics_io_end[4];
static io_data_t   metrics_io_diff[5]; // Another position was added to store per-loop node
                                       // accumulated, to be used at the end if some read fails
static ctx_t       ctx_io;

/* MPI statistics */
mpi_information_t *metrics_mpi_info[2];             // Contains the data of the period
mpi_information_t *metrics_last_mpi_info[2];
mpi_calls_types_t *metrics_mpi_calls_types[2];      // Contains the data of the period
mpi_calls_types_t *metrics_last_mpi_calls_types[2];

#if USE_GPUS
/* These vectors have position APP for application granularity and LOO for loop granularity. */
static gpu_t      *gpu_metrics_init[2];
static gpu_t      *gpu_metrics_end[2];
static gpu_t      *gpu_metrics_diff[2];
static gpu_t      *gpud_busy;
static gpu_t      *gpuddiff_busy;
static ctx_t       gpu_lib_ctx;
       uint        gpu_initialized      = 0;
static uint        gpu_loop_stopped     = 0;
static uint        earl_gpu_model       = MODEL_UNDEFINED;

char               gpu_str[256];
uint               ear_num_gpus_in_node = 0;
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
static double      cache_avrg[2];
static cpi_t       cpi_read1[2];
static cpi_t       cpi_read2[2];
static cpi_t       cpi_diff[2];
static double      cpi_avrg[2];

/* New metrics */
static metrics_t   mgt_cpufreq;
static metrics_t   mgt_imcfreq;
static metrics_t   met_cpufreq;
static metrics_t   met_imcfreq;
static metrics_t   met_bwidth;
static metrics_t   met_flops;
static metrics_t   met_cache;
static metrics_t   met_cpi;

/* New status */
static int master = -1;
static uint eard;

/* Energy saving app event */
static ear_event_t app_energy_saving;
extern report_id_t rep_id;

/** Compute the total node I/O data. */
static state_t metrics_compute_total_io(io_data_t *total);

node_freq_domain_t earl_default_domains = {
	.cpu 	= POL_DEFAULT,
	.mem 	= POL_DEFAULT,
	.gpu 	= POL_DEFAULT,
	.gpumem = POL_DEFAULT,
};

const metrics_t *metrics_get(uint api)
{
    switch(api) {
        case MGT_CPUFREQ: return &mgt_cpufreq;
        case MGT_IMCFREQ: return &mgt_imcfreq;
        case MET_CPUFREQ: return &met_cpufreq;
        case MET_IMCFREQ: return &met_imcfreq;
        case MET_BWIDTH:  return &met_bwidth;
        case MET_FLOPS:   return &met_flops;
        case MET_CACHE:   return &met_cache;
        case MET_CPI:     return &met_cpi;
        default: return NULL;
    }
}

static void metrics_configuration(topology_t *tp)
{
	char topo_str[1024];
	ulong freq_base;
	if (!EARL_LIGHT) return;
	if (eards_connected()) {
		verbose_master(2, "EARD connected");
	}else {
		verbose_master(2, "EARD not connected. Switch to monitoring?");
	}
	if (!mgt_cpufreq.ok){
		verbose_master(2, "Mgt cpufreq is not ok, switching to monitoring");
		earl_default_domains.cpu = POL_NOT_SUPPORTED;
		topology_tostr(tp, topo_str, sizeof(topo_str));
		verbose_master(2,"%s", topo_str);
		if (topology_freq_getbase(0, &freq_base) == EAR_SUCCESS){
			verbose_master(2, "Frequency detected %lu", freq_base);
		}
	}
	if (!mgt_imcfreq.ok){
		verbose_master(2, "Mgt imcfreq is not ok, swtching to monitoring for IMC");
		earl_default_domains.mem = POL_NOT_SUPPORTED;
	}
	/* This must be migrated to mgt_gpu.ok */
	#if USE_GPUS
	if (earl_gpu_model == MODEL_DUMMY) {
		verbose_master(2, "GPU dummy, swtching to monitoring for GPU");
		earl_default_domains.gpu = POL_NOT_SUPPORTED;
	}
	#endif
	if (!met_cpufreq.ok && mgt_cpufreq.ok){
		verbose_master(2, "Avg cpufreq is not ok. Using mgt_cpufreq");
	}
	if (!met_imcfreq.ok && mgt_imcfreq.ok){
		verbose_master(2, "Avg imcfreq is not ok. Using mgt_imcfreq");
	}
  if (! met_bwidth.ok){
		verbose_master(2, "Mem bandwith not ok. swtching to monitoring");
		earl_default_domains.cpu = POL_NOT_SUPPORTED;
	}
  if(!met_flops.ok){
		verbose_master(2, "FLOPS not available");
	}
  if (!met_cache.ok){
		verbose_master(2, "Cache not available");
	}
  if (!met_cpi.ok){
		verbose_master(2, "CPI not available. swtching to monitoring");
		earl_default_domains.cpu = POL_NOT_SUPPORTED;
	}
}

static void metrics_static_init(topology_t *tp)
{
    char buffer[SZ_BUFFER_EXTRA];

    #define sa(function) \
        function;

    master = (masters_info.my_master_rank >= 0);
    eard   = (master);
    // Load
#if 0
    sa(mgt_cpufreq_load(tp, 1));
#else
    sa(mgt_cpufreq_load(tp, eard));
#endif
    sa(mgt_imcfreq_load(tp, eard, NULL));
    sa(    cpufreq_load(tp, eard));
    sa(    imcfreq_load(tp, eard));
    sa(     bwidth_load(tp, eard));
    sa(      flops_load(tp, eard));
    sa(      cache_load(tp, eard));
    sa(        cpi_load(tp, eard));

    debug("metics_load ready");
    // This extinguishable
    frequency_init(eard);
    // Get API
    sa(mgt_cpufreq_get_api(&mgt_cpufreq.api));
    sa(mgt_imcfreq_get_api(&mgt_imcfreq.api));
    sa(    cpufreq_get_api(&met_cpufreq.api));
    sa(    imcfreq_get_api(&met_imcfreq.api));
    sa(     bwidth_get_api(&met_bwidth.api ));
    sa(      flops_get_api(&met_flops.api  ));
    sa(      cache_get_api(&met_cache.api  ));
    sa(        cpi_get_api(&met_cpi.api    ));
    // Working
    mgt_cpufreq.ok = (mgt_cpufreq.api > API_DUMMY);
    mgt_imcfreq.ok = (mgt_imcfreq.api > API_DUMMY);
    met_cpufreq.ok = (met_cpufreq.api > API_DUMMY);
    met_imcfreq.ok = (met_imcfreq.api > API_DUMMY);
    met_bwidth.ok  = (met_bwidth.api  > API_DUMMY);
    met_flops.ok   = (met_flops.api   > API_DUMMY);
    met_cache.ok   = (met_cache.api   > API_DUMMY);
    met_cpi.ok     = (met_cpi.api     > API_DUMMY);

	debug("metrics_api ready");
    // Init
    sa(mgt_cpufreq_init(no_ctx));
    sa(mgt_imcfreq_init(no_ctx));
    sa(    cpufreq_init(no_ctx));
    sa(    imcfreq_init(no_ctx));
    sa(     bwidth_init(no_ctx));
    sa(      flops_init(no_ctx));
    sa(      cache_init(no_ctx));
    sa(        cpi_init(no_ctx));
	debug("metrics_init ready");

    // Count
    sa(mgt_cpufreq_count_devices(no_ctx, &mgt_cpufreq.devs_count));
    sa(mgt_imcfreq_count_devices(no_ctx, &mgt_imcfreq.devs_count));
    sa(    cpufreq_count_devices(no_ctx, &met_cpufreq.devs_count));
    sa(    imcfreq_count_devices(no_ctx, &met_imcfreq.devs_count));
    sa(     bwidth_count_devices(no_ctx, &met_bwidth.devs_count ));

    debug("metrics_count_device ready");
    debug("mgt cpufreq %u", mgt_cpufreq.devs_count);
    debug("mgt imcfreq %u", mgt_imcfreq.devs_count);
    debug("    cpufreq %u", met_cpufreq.devs_count);
    debug("    imcfreq %u", met_imcfreq.devs_count);
    debug("     bwidth %u", met_bwidth.devs_count);

    // Available lists
    sa(mgt_cpufreq_get_available_list(no_ctx, (const pstate_t **) &mgt_cpufreq.avail_list,
                &mgt_cpufreq.avail_count));
    sa(mgt_imcfreq_get_available_list(no_ctx, (const pstate_t **) &mgt_imcfreq.avail_list,
                &mgt_imcfreq.avail_count));

    pstate_tostr(mgt_cpufreq.avail_list, mgt_cpufreq.avail_count, buffer, SZ_BUFFER_EXTRA);
    verbose_master(2, "CPUFREQ available list of frequencies:\n%s", buffer);
    pstate_tostr(mgt_imcfreq.avail_list, mgt_imcfreq.avail_count, buffer, SZ_BUFFER_EXTRA);
    verbose_master(2, "IMCFREQ available list of frequencies:\n%s", buffer);

    if (VERB_ON(2) && master) {
        apis_print(mgt_cpufreq.api, "MGT_CPUFREQ: ");
        apis_print(mgt_imcfreq.api, "MGT_IMCFREQ: ");
        apis_print(met_cpufreq.api, "MET_CPUFREQ: ");
        apis_print(met_imcfreq.api, "MET_IMCFREQ: ");
        apis_print(met_bwidth.api , "MET_BWIDTH : ");
        apis_print(met_flops.api  , "MET_FLOPS  : ");
        apis_print(met_cache.api  , "MET_CACHE  : ");
        apis_print(met_cpi.api    , "MET_CPI    : ");
    }

    cpufreq_data_alloc(&cpufreq_read1[0], empty);
    cpufreq_data_alloc(&cpufreq_read1[1], empty);
    cpufreq_data_alloc(&cpufreq_read2[0], empty);
    cpufreq_data_alloc(&cpufreq_read2[1], &cpufreq_diff);

    imcfreq_data_alloc(&imcfreq_read1[0], empty);
    imcfreq_data_alloc(&imcfreq_read1[1], empty);
    imcfreq_data_alloc(&imcfreq_read2[0], &imcfreq_diff[0]);
    imcfreq_data_alloc(&imcfreq_read2[1], &imcfreq_diff[1]);

    bwidth_data_alloc(&bwidth_read1[0]);
    bwidth_data_alloc(&bwidth_read1[1]);
    bwidth_data_alloc(&bwidth_read2[0]);
    bwidth_data_alloc(&bwidth_read2[1]);
    bwidth_data_alloc(&bwidth_diff[0]);
    bwidth_data_alloc(&bwidth_diff[1]);

    debug("metrics_allocation ready");

    metrics_configuration(tp);
}

void metrics_static_dispose()
{
}

// Legacy code
static void cpufreq_my_avgcpufreq(ulong *nodecpuf, ulong *avgf)
{
	cpu_set_t m;
	ulong total = 0, total_cpus = 0;
	ulong lavg, lcpus;
	int p;
    for (p = 0; p < lib_shared_region->num_processes ; p++)
    {
        m = sig_shared_region[p].cpu_mask;
		pstate_freqtoavg(m, nodecpuf, met_cpufreq.devs_count, &lavg, &lcpus);
		total += lavg*lcpus;
		total_cpus += lcpus;
	}
	*avgf = total/total_cpus;
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

	app_energy_saving.jid 		= application.job.id;
	app_energy_saving.step_id = application.job.step_id;
	strcpy(app_energy_saving.node_id, application.node_id);

	if (master)
	{
		/* Avg CPU freq */
        if (state_fail(s = cpufreq_read(no_ctx, cpufreq_read1[APP]))) {
            debug("CPUFreq data read in global_start");
        }
		/* Reads IMC data */
		if (mgt_imcfreq.ok){
		    debug("imcfreq_read");
		    if (state_fail(s = imcfreq_read(no_ctx, imcfreq_read1[APP]))){
			    debug("IMCFreq data read in global_start");
		    }
		}
		/* Energy */
		if (eards_connected()){
			debug("eards_node_dc_energy");
			ret = eards_node_dc_energy(aux_energy,node_energy_datasize);
			if ((ret == EAR_ERROR) || energy_lib_data_is_null(aux_energy)){
				debug("MR[%d] Error reading Node energy at application start",masters_info.my_master_rank);
			}
			/* RAPL energy metrics */
			eards_read_rapl(aux_rapl);
		}else{
			memset(aux_energy, 0, node_energy_datasize);
			memset(aux_rapl, 0 ,rapl_size);
		}

		#if USE_GPUS
		/* GPu metrics */
		if (gpu_initialized){
			gpu_lib_data_null(gpu_metrics_init[APP]);
			if (gpu_lib_read(&gpu_lib_ctx,gpu_metrics_init[APP]) != EAR_SUCCESS){
				debug("Error in gpu_read in application start");
			}
			debug("gpu_lib_read in global start");
			gpu_lib_data_copy(gpu_metrics_end[LOO],gpu_metrics_init[APP]);
			debug("gpu_lib_data_copy in global start");
		}
		#endif

		/* Only the master computes per node I/O metrics */
		if (state_fail(metrics_compute_total_io(&metrics_io_init[APP_NODE]))) {

            verbose_master(2, "%sWarning%s Node total I/O could not be"
                    " read (%s)." , COL_RED, COL_CLR, state_msg);
        }

	} else {
		set_null_dc_energy(aux_energy);
		set_null_rapl(aux_rapl);
	}

	/* Per process IO data */
	if (io_read(&ctx_io, &metrics_io_init[APP]) != EAR_SUCCESS){
		verbose_master(2,"Warning: IO data not available");
	}

    // New metrics
    bwidth_read(no_ctx, bwidth_read1[APP]);
		debug("bwidth_read ready");
     flops_read(no_ctx, &flops_read1[APP]);
		debug("flops_read ready");
     cache_read(no_ctx, &cache_read1[APP]);
	  debug("cache_read ready");
       cpi_read(no_ctx,   &cpi_read1[APP]);
		debug("cpi_read ready");
}

static void metrics_global_stop()
{
	char io_info[256];
	char *verb_path=getenv(FLAG_EARL_VERBOSE_PATH);
    state_t s;
	int i;

    debug("%s--- (l. rank) %d  METRICS GLOBAL STOP ---%s", COL_YLW, my_node_id, COL_CLR);

    if (!(master)) {
        /* Synchro 4: Waiting for master signature */
        while(!lib_shared_region->global_stop_ready) {
            if (msync(lib_shared_region, sizeof(lib_shared_data_t), MS_SYNC) < 0) {
                verbose_master(2, "%sError%s Memory sync. for lib shared region failed: %s",
                        COL_RED, COL_CLR, strerror(errno));
            }
            usleep(10000);
        }
    }

	imcfreq_avrg[APP] = 0;
	cpufreq_avrg[APP] = 0;

	/* Only the master will collect these metrics */
	if (master) {

        /* MPI statistics */
        if (is_mpi_enabled()) {
          read_diff_node_mpi_info(lib_shared_region, sig_shared_region,
                  metrics_mpi_info[APP], metrics_last_mpi_info[APP]);
          read_diff_node_mpi_types_info(lib_shared_region, sig_shared_region,
                  metrics_mpi_calls_types[APP], metrics_last_mpi_calls_types[APP]);
        }

        /* Avg CPU frequency */
        if (state_fail(s = cpufreq_read(no_ctx, cpufreq_read2[APP]))) {
            error("CPUFreq data read in global_stop");
        }
        if (state_fail(s = cpufreq_data_diff(cpufreq_read2[APP], cpufreq_read1[APP],
                        cpufreq_diff, &cpufreq_avrg[APP]))){
            error("CPUFreq data diff");
        } else {
            debug("CPUFreq average for the application is %.2fGHz",
                    (float) cpufreq_avrg[APP] / 1000000.0);
        }

        cpufreq_my_avgcpufreq(cpufreq_diff, &cpufreq_avrg[APP]);

        // Copy the average CPU freq. in the shared signature
        memcpy(lib_shared_region->avg_cpufreq, cpufreq_diff,
                met_cpufreq.devs_count * sizeof(cpufreq_diff[0]));

        /* AVG IMC frequency */
        if (mgt_imcfreq.ok) {
            if (state_fail(s = imcfreq_read(no_ctx, imcfreq_read2[APP]))) {
                error("IMCFreq data read in global_stop");
            }
            if (state_fail(s = imcfreq_data_diff(imcfreq_read2[APP], imcfreq_read1[APP],
                            imcfreq_diff[APP], &imcfreq_avrg[APP]))){
                error("IMC data diff fails");
            }
            verbose_master(2, "AVG IMC frequency %.2f", (float)imcfreq_avrg[APP]/1000000.0);
            for (i=0; i< mgt_imcfreq.devs_count;i++) {
                imc_max_pstate[i]= imc_num_pstates-1;
                imc_min_pstate[i] = 0;
            }
            if (state_fail(s = mgt_imcfreq_set_current_ranged_list(no_ctx,
                            imc_min_pstate, imc_max_pstate))){
                error("Setting the IMC freq in global stop");
            }

            verbose_master(2, "IMC freq restored to %.2f-%.2f",
                    (float) imc_pstates[imc_min_pstate[0]].khz / 1000000.0,
                    (float) imc_pstates[imc_max_pstate[0]].khz / 1000000.0);
        }

#if USE_GPUS
        /* GPU */
        if (gpu_initialized) {
            if (state_fail(gpu_lib_read(&gpu_lib_ctx, gpu_metrics_end[APP]))) {
                debug("gpu_lib_read error");
            }
            debug("gpu_read in global_stop");

            gpu_lib_data_diff(gpu_metrics_end[APP],
                    gpu_metrics_init[APP], gpu_metrics_diff[APP]);

            gpu_lib_data_tostr(gpu_metrics_diff[APP], gpu_str, sizeof(gpu_str));

            debug("gpu_data in global_stop %s", gpu_str);
        }
#endif
		/* IO node data */
        if (state_fail(metrics_compute_total_io(&metrics_io_end[APP_NODE]))) {

            // As we could not read the global node IO, we
            // put as global IO the total accumulated until now.
            metrics_io_diff[APP_NODE] = metrics_io_diff[APP_ACUM];

            verbose_master(2, "%sWarning%s Node total I/O could not be read (%s), working with"
                    " the acumulated data read until now.", COL_RED, COL_CLR, state_msg);

        } else {
            io_diff(&metrics_io_diff[APP_NODE], &metrics_io_init[APP_NODE],
                    &metrics_io_end[APP_NODE]);
        }
	} // master

    /* Per-process IO */
    if (state_fail(io_read(&ctx_io, &metrics_io_end[APP]))) {
        verbose_master(2, "%sWarning%s I/O data not available.", COL_RED, COL_CLR);
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
		write(fd, io_buffer, strlen(io_buffer));

        free(io_buffer);

		if (verb_path != NULL) close(fd);
	}

    // New metrics
    bwidth_read_diff(no_ctx, bwidth_read2[APP], bwidth_read1[APP], NULL, &bwidth_cas[APP], NULL);
     flops_read_diff(no_ctx, &flops_read2[APP], &flops_read1[APP], NULL, NULL);
     cache_read_diff(no_ctx, &cache_read2[APP], &cache_read1[APP], &cache_diff[APP], &cache_avrg[APP]);
       cpi_read_diff(no_ctx,   &cpi_read2[APP],   &cpi_read1[APP],   &cpi_diff[APP],   &cpi_avrg[APP]);
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
    if (state_fail(io_read(&ctx_io, &metrics_io_init[LOO]))){
        verbose_master(2, "%sWarning%s I/O data not available.", COL_RED, COL_CLR);
    }
    debug("IO read done");

    /* These data is measured only by the master */
    if (master) {
        /* Avg CPU freq */
        if (state_fail(cpufreq_read(no_ctx, cpufreq_read1[LOO]))) {
            verbose_master(2, "%sError%s Reading loop's CPUFreq data.", COL_RED, COL_CLR);
        }
        debug("cpufreq_read ");

        /* Avg IMC freq */
        if (mgt_imcfreq.ok){
            /* Reads the IMC */
            if (state_fail(imcfreq_read(no_ctx,imcfreq_read1[LOO]))){
			    verbose_master(2, "%sError%s Reading loop's IMCFreq data.", COL_RED, COL_CLR);
            }
        }

        /* Per NODE IO data */
		if (state_fail(metrics_compute_total_io(&metrics_io_init[LOO_NODE]))) {

            verbose_master(2, "%sWarning%s Node total I/O could not be"
                    " read (%s)." , COL_RED, COL_CLR, state_msg);
        }

#if USE_GPUS
        /* GPUS */
        if (gpu_initialized){
            if (gpu_loop_stopped) {
                gpu_lib_data_copy(gpu_metrics_init[LOO],gpu_metrics_end[LOO]);
            } else{
                gpu_lib_data_copy(gpu_metrics_init[LOO],gpu_metrics_init[APP]);
            }
            debug("gpu_copy in partial start");
        }
#endif
    } //master_rank

    /* Energy RAPL */
    for (int i = 0; i < rapl_elements; i++) {
        last_rapl[i] = aux_rapl[i];
    }
    debug("RAPL copied");

    // New metrics
    bwidth_read(no_ctx, bwidth_read1[LOO]);
    flops_read(no_ctx, &flops_read1[LOO]);
    cache_read(no_ctx, &cache_read1[LOO]);
    cpi_read(no_ctx,   &cpi_read1[LOO]);

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
	char stop_energy_str[256], start_energy_str[256];
    state_t s;

    debug("metrics_partial_stop %u", my_node_id);

    if (!eards_connected() && (master)){
        verbose_master(2, "Disconnecting library because EARD not connected.");
        if (!EARL_LIGHT) lib_shared_region->earl_on = 0;
    }

    /* If the signature of the master is not ready, we cannot compute our signature */
    //debug("My rank is %d",masters_info.my_master_rank);
    if ((masters_info.my_master_rank<0) && (!sig_shared_region[0].ready)){
        debug("Iterations %u ear_guided %u", ear_iterations, ear_guided);
        return EAR_NOT_READY;
    }

#if SHOW_DEBUGS
    float elapsed = (metrics_time() - app_start) / 1000000;
    debug("Master signature ready at time %f", elapsed);
#endif

	// Manual IPMI accumulation
	if (master) {
		/* Energy node */
		if (eards_connected()){
			int ret = eards_node_dc_energy(aux_energy_stop, node_energy_datasize);

			/* If EARD is present, we wait until the power is computed */
			if (energy_lib_data_is_null(aux_energy_stop) || (ret == EAR_ERROR)) {
				if (eards_connected()) return EAR_NOT_READY;
				else 							     return EAR_SUCCESS;
			}
		}else{
			memset(aux_energy_stop , 0, node_energy_datasize);
		}
		energy_lib_data_accumulated(&c_energy,metrics_ipmi[LOO],aux_energy_stop);
		energy_lib_data_to_str(start_energy_str,metrics_ipmi[LOO]);
		energy_lib_data_to_str(stop_energy_str,aux_energy_stop);

		/* If EARD is present, we wait until the power is computed */
		if (eards_connected()){
			if ((where==SIG_END) && (c_energy==0) && (master)) {
				debug("EAR_NOT_READY because of accumulated energy %lu\n",c_energy);
				if (dispose) fprintf(stderr,"partial stop and EAR_NOT_READY\n");
				if (!EARL_LIGHT) return EAR_NOT_READY;
			}
		}
	} // master

	/* Time */
	aux_time_stop = metrics_time();

	/* Sometimes energy is not zero but power is not correct */
	c_time=metrics_usecs_diff(aux_time_stop, metrics_usecs[LOO]);

	/* energy is computed in node_energy_units and time in usecs */
	c_power=(float)(c_energy*(1000000.0/(double)node_energy_units))/(float)c_time;

    if (master) {
        if (dispose && ((c_power<0) || (c_power>system_conf->max_sig_power))){
            debug("dispose and c_power %lf\n",c_power);
            debug("power %f energy %lu time %llu\n",c_power,c_energy,c_time);
        }
    }

	/* If we are not the node master, we will continue */
	if ((where==SIG_END) && (c_power<system_conf->min_sig_power) && (master)) {
		debug("EAR_NOT_READY because of power %f\n",c_power);
		if (!EARL_LIGHT) return EAR_NOT_READY;
	}

	/* This is new to avoid cases where uncore gets frozen */
    if (master) {
#if USE_GPUS
        /* GPUs */
        if (gpu_initialized){
            if (gpu_lib_read(&gpu_lib_ctx, gpu_metrics_end[LOO])!= EAR_SUCCESS){
                debug("gpu_lib_read error");
            }
            debug("gpu_read in partial stop");
            gpu_loop_stopped = 1;
            gpu_lib_data_diff(gpu_metrics_end[LOO], gpu_metrics_init[LOO], gpu_metrics_diff[LOO]);
            gpu_lib_data_tostr(gpu_metrics_diff[LOO], gpu_str, sizeof(gpu_str));
            debug("gpu_diff in partial_stop: %s", gpu_str);
        }
#endif
    }

	/* End new section to check frozen uncore counters */
	energy_lib_data_copy(aux_energy,aux_energy_stop);
	aux_time=aux_time_stop;

	if (master) {
		if (c_power < system_conf->max_sig_power) {
			acum_ipmi[LOO] = c_energy;
		}else{
			verbose_master(2, "Computed power was not correct (%lf) reducing it to %lf\n",c_power,system_conf->min_sig_power);
			acum_ipmi[LOO] = system_conf->min_sig_power*c_time;
		}
		acum_ipmi[APP] += acum_ipmi[LOO];
	}
	/* Per process IO data */
    if (io_read(&ctx_io, &metrics_io_end[LOO]) != EAR_SUCCESS){
        verbose_master(2,"%sWarning%s I/O data not available", COL_RED, COL_CLR);
    }
	io_diff(&metrics_io_diff[LOO],&metrics_io_init[LOO],&metrics_io_end[LOO]);

	//  Manual time accumulation
	metrics_usecs[LOO] = c_time;
	metrics_usecs[APP] += metrics_usecs[LOO];
	
    // Daemon metrics
    if (master) {
        /* MPI statistics */
        if (is_mpi_enabled()) {
         read_diff_node_mpi_info(lib_shared_region, sig_shared_region,
                 metrics_mpi_info[LOO], metrics_last_mpi_info[LOO]);
         read_diff_node_mpi_types_info(lib_shared_region, sig_shared_region,
                 metrics_mpi_calls_types[LOO], metrics_last_mpi_calls_types[LOO]);
        }

        /* Avg CPU freq */
        if (state_fail(s = cpufreq_read(no_ctx, cpufreq_read2[LOO]))) {
            verbose_master(2, "%sCPUFreq data read in partial stop failed%s", COL_RED, COL_CLR);
        }
        if (xtate_fail(s, cpufreq_data_diff(cpufreq_read2[LOO], cpufreq_read1[LOO], cpufreq_diff, &cpufreq_avrg[LOO]))){
            verbose_master(2, "%sCPUFreq data diff failed%s", COL_RED, COL_CLR);
        } else {
            verbose_master(2, "%sPARTIAL STOP%s CPUFreq average is %.2f GHz", COL_BLU, COL_CLR, (float) cpufreq_avrg[LOO] / 1000000.0);
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
            if (state_fail(s = imcfreq_read(no_ctx,imcfreq_read2[LOO]))){
                verbose_master(2, "%sError%s IMC freq. data read in partial stop", COL_RED, COL_CLR);
            }
            if (state_fail(s = imcfreq_data_diff(imcfreq_read2[LOO],imcfreq_read1[LOO], imcfreq_diff[LOO], &imcfreq_avrg[LOO]))){
                verbose_master(2, "IMC data diff fails");
            }
            debug("AVG IMC frequency %.2f",(float)imcfreq_avrg[LOO]/1000000.0);
		}

		/* Per node IO data */
        if (state_ok(metrics_compute_total_io(&metrics_io_end[LOO_NODE]))) {

            // Diff
            io_diff(&metrics_io_diff[LOO_NODE],
                    &metrics_io_init[LOO_NODE], &metrics_io_end[LOO_NODE]);

            // Accumulate
            io_accum(no_ctx, &metrics_io_diff[APP_ACUM], &metrics_io_diff[LOO_NODE]);
        } else {
            memset(&metrics_io_diff[LOO_NODE], 0, sizeof(io_data_t));

            verbose_master(2, "%sWarning%s Node total I/O could not be read (%s). Assuming"
                    " no I/O data for this loop.", COL_RED, COL_CLR, state_msg);
        }

        /* This code needs to be adapted to read , compute the difference, and copy begin=end 
         * diff_uncores(values,values_end,values_begin,num_counters);
         * copy_uncores(values_begin,values_end,num_counters);
         */

	/* RAPL energy */
		if (eards_connected()) eards_read_rapl(aux_rapl);
		else memset(aux_rapl, 0, rapl_size);

		// We read acuumulated energy
		for (int i = 0; i < rapl_elements; i++) {
			if (aux_rapl[i] < last_rapl[i]) {
				metrics_rapl[LOO][i] = ullong_diff_overflow(last_rapl[i], aux_rapl[i]);
			}
			else {
				metrics_rapl[LOO][i]=aux_rapl[i]-last_rapl[i];		
			}
		}
	
		// Manual RAPL accumulation
		for (int i = 0; i < rapl_elements; i++) {
				metrics_rapl[APP][i] += metrics_rapl[LOO][i];
		}
    } //master_rank (metrics collected by node_master)

	// New metrics
    bwidth_read_diff(no_ctx, bwidth_read2[LOO], bwidth_read1[LOO], bwidth_diff[LOO], &bwidth_cas[LOO], NULL);
     flops_read_diff(no_ctx, &flops_read2[LOO], &flops_read1[LOO], &flops_diff[LOO], NULL);
     cache_read_diff(no_ctx, &cache_read2[LOO], &cache_read1[LOO], &cache_diff[LOO], &cache_avrg[LOO]);
       cpi_read_diff(no_ctx,   &cpi_read2[LOO],   &cpi_read1[LOO],   &cpi_diff[LOO],   &cpi_avrg[LOO]);

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

void copy_node_data(signature_t *dest,signature_t *src)
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

/******************* This function computes the signature : per loop (global = LOO) or application ( global = APP)  **************/
/******************* The node master has computed the per-node metrics and the rest of processes uses this information ***********/
static void metrics_compute_signature_data(uint sign_app_loop_idx, signature_t *metrics,
        uint iterations, ulong procs)
{
				// If sign_app_loop_idx is 1, it means that the global application metrics are required
				// instead the small time metrics for loops. 'sign_app_loop_idx' is just a
                // signature index.
				sig_ext_t *sig_ext;
				
                debug("process %d - %sapp/loop(1/0) %u%s",
                        my_node_id, COL_YLW, sign_app_loop_idx, COL_CLR);

				/* Total time */
				double time_s = (double) metrics_usecs[sign_app_loop_idx] / 1000000.0;

                if (VERB_ON(2) && (sign_app_loop_idx == APP)) {
                    verbose_master(2, "Process %d ends in exclusive mode: %u",
                            my_node_id, exclusive);
                }

				/* Avg CPU frequency */
				metrics->avg_f = cpufreq_avrg[sign_app_loop_idx];

				/* Avg IMC frequency */
				metrics->avg_imc_f = imcfreq_avrg[sign_app_loop_idx];

				/* Time per iteration */
				metrics->time = time_s / (double) iterations;

				/* Cache misses */
				metrics->L1_misses = cache_diff[sign_app_loop_idx].l1d_misses;
				metrics->L2_misses = cache_diff[sign_app_loop_idx].l2_misses;
				metrics->L3_misses = cache_diff[sign_app_loop_idx].l3_misses;
 
                if (metrics->sig_ext == NULL) {
                    metrics->sig_ext = (void *) calloc(1, sizeof(sig_ext_t));
                }
				sig_ext = metrics->sig_ext;
				sig_ext->elapsed = time_s;

				/* FLOPS */
                flops_data_accum(&flops_diff[APP], NULL, &metrics->Gflops);
                flops_help_toold(&flops_diff[APP], metrics->FLOPS);

                /* CPI */
                metrics->cycles       = cpi_diff[sign_app_loop_idx].cycles;
                metrics->instructions = cpi_diff[sign_app_loop_idx].instructions;
                metrics->CPI          = cpi_avrg[sign_app_loop_idx];

				/* TPI and Memory bandwith */
                if (master) {
                    bwidth_data_accum(bwidth_diff[APP], bwidth_diff[LOO], NULL, NULL);
                    verbose_master(INFO_METRICS + 1, "%sCOMPUTE SIGNATURE DATA%s GBS[%d]: %0.2lf", COL_BLU, COL_CLR, sign_app_loop_idx, metrics->GBS);
					metrics->TPI = bwidth_help_castotpi(bwidth_cas[sign_app_loop_idx],
										metrics->instructions);
				 	metrics->GBS = bwidth_help_castogbs(bwidth_cas[sign_app_loop_idx], time_s);
                } else {
                    bwidth_cas[sign_app_loop_idx] = (ullong) lib_shared_region->cas_counters;
                    metrics->GBS = bwidth_help_castogbs(bwidth_cas[sign_app_loop_idx], time_s);
                }

                metrics->TPI = bwidth_help_castotpi(bwidth_cas[sign_app_loop_idx],
                        metrics->instructions);

				/* Per process IO data */
                io_copy(&(sig_ext->iod), &metrics_io_diff[sign_app_loop_idx]);
                io_copy(&sig_shared_region[my_node_id].sig.iod, &metrics_io_diff[sign_app_loop_idx]);

				/* Per Node IO data */
				int io_app_loop_idx = sign_app_loop_idx;

                /* Each process will compute its own IO_MBS, except
                 * the master, that computes per node IO_MBS */
                if (master) {
                    if (io_app_loop_idx == APP) {
                        io_app_loop_idx = APP_NODE;
                    } else if (io_app_loop_idx == LOO) {
                        io_app_loop_idx = LOO_NODE;
                    }
                }

                double iogb = (double) metrics_io_diff[io_app_loop_idx].rchar / (double) (1024*1024) + (double) metrics_io_diff[io_app_loop_idx].wchar/(double)(1024*1024);

				metrics->IO_MBS = iogb/time_s;

                if (master) {
                    sig_ext->max_mpi = 0;
                    sig_ext->min_mpi = 100;

                    /* MPI statistics */
                    if (is_mpi_enabled()) {
                        double mpi_time_perc = 0.0;
                        for (int i = 0; i < lib_shared_region->num_processes; i++) {
                            ullong mpi_time_usecs = ((mpi_information_t *) metrics_mpi_info[sign_app_loop_idx])[i].mpi_time;
                            ullong exec_time_usecs = ((mpi_information_t *) metrics_mpi_info[sign_app_loop_idx])[i].exec_time;

#if 0
                            double local_mpi_time_perc;
                            double local_mpi_time_perc_test = ((mpi_information_t *) metrics_mpi_info[sign_app_loop_idx])[i].perc_mpi;
                            if ((metrics_mpi_info[sign_app_loop_idx])[i].mpi_time > 0) {
                                local_mpi_time_perc = (double) (mpi_time_usecs * 100.0 / exec_time_usecs);
                            }
                            else {
                                local_mpi_time_perc = 0;
                            }
#endif
                            // The percentage of time spent in MPI calls is computed on
                            // metrics_[partial|global]_stop, so we can avoid the above code.
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
                    sig_ext->mpi_types = metrics_mpi_calls_types[sign_app_loop_idx];

                    /* Power: Node, DRAM, PCK */
                    metrics->DC_power = (double) acum_ipmi[sign_app_loop_idx] / (time_s * node_energy_units);

                    /* DRAM and PCK PENDING for Njobs */
                    metrics->PCK_power=0;
                    metrics->DRAM_power=0;

                    for (int p = 0; p < num_packs; p++) {
                        double rapl_dram = (double) metrics_rapl[sign_app_loop_idx][p];
                        metrics->DRAM_power = metrics->DRAM_power + rapl_dram;

                        double rapl_pck = (double) metrics_rapl[sign_app_loop_idx][num_packs+p];
                        metrics->PCK_power = metrics->PCK_power + rapl_pck;
                    }

                    metrics->PCK_power   = (metrics->PCK_power / 1000000000.0) / time_s;
                    metrics->DRAM_power  = (metrics->DRAM_power / 1000000000.0) / time_s;
                    metrics->EDP = time_s * time_s * metrics->DC_power;

#if USE_GPUS
                    /* GPUS */
                    if (gpu_initialized) {
                        metrics->gpu_sig.num_gpus = ear_num_gpus_in_node;
                        if (earl_gpu_model == MODEL_DUMMY) { 
                            debug("Setting num_gpu to 0 because model is DUMMY");
                            metrics->gpu_sig.num_gpus = 0;
                        }
                        verbose_master(2, "We have %d GPUS ", metrics->gpu_sig.num_gpus);
                        for (int p=0;p<metrics->gpu_sig.num_gpus;p++){
                            metrics->gpu_sig.gpu_data[p].GPU_power    = gpu_metrics_diff[sign_app_loop_idx][p].power_w;
                            metrics->gpu_sig.gpu_data[p].GPU_freq     = gpu_metrics_diff[sign_app_loop_idx][p].freq_gpu;
                            metrics->gpu_sig.gpu_data[p].GPU_mem_freq = gpu_metrics_diff[sign_app_loop_idx][p].freq_mem;
                            metrics->gpu_sig.gpu_data[p].GPU_util     = gpu_metrics_diff[sign_app_loop_idx][p].util_gpu;
                            metrics->gpu_sig.gpu_data[p].GPU_mem_util = gpu_metrics_diff[sign_app_loop_idx][p].util_mem;
                        }	
                    }
#endif
                } else {
                    copy_node_data(metrics,&lib_shared_region->node_signature);
                }

				/* Copying my info in the shared signature */
				from_sig_to_mini(&sig_shared_region[my_node_id].sig,metrics);

				/* If I'm the master, I have to copy in the special section */
                if (master){
                    signature_copy(&lib_shared_region->node_signature,metrics);
                    signature_copy(&lib_shared_region->job_signature,metrics);

                    if (VERB_ON(2)) {
                        signature_print_simple_fd(2, metrics);
                        verbose_master(2," ");
                    }
                }

                sig_shared_region[my_node_id].period = time_s;


				update_earl_node_mgr_info();

				debug("Signature ready - local rank %d - time %lld", my_node_id, (metrics_time() - app_start)/1000000);
				signature_ready(&sig_shared_region[my_node_id],0);

				verbose_master(INFO_METRICS + 1,
                        "End compute signature for (l. rank) %d. Period %lf secs",
                        my_node_id, sig_shared_region[my_node_id].period);

				if (msync(sig_shared_region, sizeof(shsignature_t) * lib_shared_region->num_processes,MS_SYNC) < 0) {
                    verbose_master(2, "Memory sync fails: %s", strerror(errno));
                }

				/* Compute and print the total savings */
				if ((sign_app_loop_idx == APP) && (masters_info.my_master_rank >= 0) && sig_ext->telapsed){
					float avg_esaving = sig_ext->saving / sig_ext->telapsed;
					float avg_psaving = sig_ext->psaving / sig_ext->telapsed;
					float avg_tpenalty = sig_ext->tpenalty / sig_ext->telapsed;
					verbose_master(2, "MR[%d] Average estimated energy savings %.2f. Average Power reduction %.2f. Time penalty %.2f. Elapsed %.2f", masters_info.my_master_rank, avg_esaving, avg_psaving, avg_tpenalty, sig_ext->telapsed);
					app_energy_saving.event = ENERGY_SAVING;
					/* We will use freq for the energy saving */
					app_energy_saving.freq = (ulong)(avg_esaving * 10);
					report_events(&rep_id, &app_energy_saving, 1);

					#if 0
					/* Not yet reported */
					app_energy_saving.event = POWER_SAVING;
					app_energy_saving.event = PERF_PENALTY;
					#endif
				}

				if (sign_app_loop_idx == APP) {
                    sig_shared_region[my_node_id].exited = 1;
                }
}

/**************************** Init function used in ear_init ******************/
int metrics_init(topology_t *topo)
{
    char *cimc_max_khz,*cimc_min_khz;
#if 0
    char imc_buffer[1024];
#endif
    pstate_t tmp_def_imc_pstate;
    state_t st;
    state_t s;
    int i;

    metrics_static_init(topo);
    assert(master >= 0); // Debug if master was well initialized at metrics_static_init
    debug("metrics_static_init done");

    //debug("Masters region %p size %lu",&masters_info,sizeof(masters_info));
    //debug("My master rank %d",masters_info.my_master_rank);

    hw_cache_line_size = topo->cache_line_size;
    num_packs = topo->socket_count;
    topology_copy(&mtopo,topo);

    //debug("detected cache line size: %0.2lf bytes", hw_cache_line_size);
    //debug("detected sockets: %d", num_packs);

    if (hw_cache_line_size == 0) {
        return_msg(EAR_ERROR, "error detecting the cache line size");
    }
    if (num_packs == 0) {
        return_msg(EAR_ERROR, "error detecting number of packges");
    }

    st = energy_lib_init(system_conf);
    if (st != EAR_SUCCESS) {
        verbose(2, "%sError%s returned by energy_lib_init: %s", COL_RED, COL_CLR, state_msg);
        return_msg(EAR_ERROR, "Loading energy plugin");
    }

    if (io_init(&ctx_io,getpid()) != EAR_SUCCESS){
        verbose_master(2,"Warning: IO data not available");
    } else {
        verbose_master(INFO_METRICS,"IO data initialized");
    }

    // Daemon metrics allocation
    if (master && eards_connected()){
        rapl_size = eards_get_data_size_rapl();
    }else{
        rapl_size=sizeof(unsigned long long);
    }

    debug("RAPL size %lu",rapl_size);
    rapl_elements = rapl_size / sizeof(unsigned long long);

    // Allocating data for energy node metrics
    // node_energy_datasize=eards_node_energy_data_size();
    energy_lib_datasize(&node_energy_datasize);
    debug("energy data size %lu", node_energy_datasize);
    energy_lib_units(&node_energy_units);
    /* We should create a data_alloc for enerrgy and a set_null */
    aux_energy=(edata_t)malloc(node_energy_datasize);
    aux_energy_stop=(edata_t)malloc(node_energy_datasize);
    metrics_ipmi[0]=(edata_t)malloc(node_energy_datasize);
    metrics_ipmi[1]=(edata_t)malloc(node_energy_datasize);
    memset(metrics_ipmi[0],0,node_energy_datasize);
    memset(metrics_ipmi[1],0,node_energy_datasize);
    acum_ipmi[0]=0;acum_ipmi[1]=0;

    metrics_rapl[LOO] = malloc(rapl_size);
    metrics_rapl[APP] = malloc(rapl_size);
    aux_rapl = malloc(rapl_size);
    last_rapl = malloc(rapl_size);

    memset(metrics_rapl[LOO], 0, rapl_size);
    memset(metrics_rapl[APP], 0, rapl_size);
    memset(aux_rapl, 0, rapl_size);
    memset(last_rapl, 0, rapl_size);
    if (master) {

        debug("Allocating data for %d processes", lib_shared_region->num_processes);

        /* We will collect MPI statistics */
        metrics_mpi_info[APP]             = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
        metrics_last_mpi_info[APP]        = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
        metrics_mpi_info[LOO]             = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
        metrics_last_mpi_info[LOO]        = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
        metrics_mpi_calls_types[APP]      = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
        metrics_last_mpi_calls_types[APP] = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
        metrics_mpi_calls_types[LOO]      = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
        metrics_last_mpi_calls_types[LOO] = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));

        /* IMC management */
        if (mgt_imcfreq.ok) {

            imc_devices = mgt_imcfreq.devs_count;

            imc_max_pstate = calloc(mgt_imcfreq.devs_count, sizeof(uint));
            imc_min_pstate = calloc(mgt_imcfreq.devs_count, sizeof(uint));

            if ((imc_max_pstate == NULL) || (imc_min_pstate == NULL)){
                error("Asking for memory");
                return EAR_ERROR;
            }

            if (state_fail(s = mgt_imcfreq_get_available_list(NULL, (const pstate_t **)&imc_pstates, &imc_num_pstates))){
                error("Asking for IMC frequency list");
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

            cimc_max_khz = (system_conf->user_type == AUTHORIZED) ? getenv(FLAG_MAX_IMCFREQ) : NULL; // Max. IMC freq. (kHz)
            cimc_min_khz = (system_conf->user_type == AUTHORIZED) ? getenv(FLAG_MIN_IMCFREQ) : NULL; // Min. IMC freq. (kHz)

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
                error("Initialzing the IMC freq");
            }

            /* We assume all the sockets have the same configuration */
            imc_curr_max_pstate = imc_max_pstate[0];
            imc_curr_min_pstate = imc_min_pstate[0];	

            // Verbose a resume
            if (verb_level >= INFO_METRICS) {
                verbose_master(INFO_METRICS, "\n           --- IMC info ---\n            #devices: %u", mgt_imcfreq.devs_count);

                if (cimc_max_khz != NULL) {
                    verbose_master(INFO_METRICS, "              Maximum: %s kHz", cimc_max_khz);
                }

                if (cimc_min_khz != NULL) {
                    verbose_master(INFO_METRICS, "             Minimum: %s kHz", cimc_min_khz);
                }

                verbose_master(INFO_METRICS, "Init (max/min) (kHz): %.2f/%.2f\n           ----------------\n",
                        (float) imc_pstates[imc_min_pstate[0]].khz, (float) imc_pstates[imc_max_pstate[0]].khz);
            }

        } else {
            verbose_master(2, "%sMETRICS%s IMCFREQ not supported.", COL_BLU, COL_CLR);
        } // IMC
    }

#if USE_GPUS
    if (master){
        if (gpu_lib_load(system_conf) !=EAR_SUCCESS){
            gpu_initialized=0;
            debug("Error in gpu_lib_load");
        }else gpu_initialized=1;
        if (gpu_initialized){
            debug("Initializing GPU");
            if (gpu_lib_init(&gpu_lib_ctx) != EAR_SUCCESS){
                error("Error in GPU initiaization");
                gpu_initialized=0;
            }else{
                debug("GPU initialization successfully");
                gpu_lib_model(&gpu_lib_ctx,&earl_gpu_model);
                debug("GPU model %u",earl_gpu_model);
                gpu_lib_data_alloc(&gpu_metrics_init[LOO]);gpu_lib_data_alloc(&gpu_metrics_init[APP]);
                gpu_lib_data_alloc(&gpu_metrics_end[LOO]);gpu_lib_data_alloc(&gpu_metrics_end[APP]);
                gpu_lib_data_alloc(&gpu_metrics_diff[LOO]);gpu_lib_data_alloc(&gpu_metrics_diff[APP]);
                gpu_lib_count(&ear_num_gpus_in_node);
                gpu_lib_data_alloc(&gpud_busy);
                gpu_lib_data_alloc(&gpuddiff_busy);
                debug("GPU library data initialized");
            }
        }
        debug("GPU initialization successfully");
    }
    fflush(stderr);	
#endif

    //debug( "detected %d RAPL counters for %d packages: %d events por package", rapl_elements,num_packs,rapl_elements/num_packs);

    metrics_global_start();
    metrics_partial_start();

    return EAR_SUCCESS;
}

#define MAX_SYNCHO_TIME 100

void metrics_dispose(signature_t *metrics, ulong procs)
{
    sig_ext_t *se;
    signature_t nodeapp;
    uint num_ready;
    uint exited;
    float wait_time = 0;

    verbosen_master(INFO_METRICS, "--- Metrics dispose init (l. rank) %d ---", my_node_id);
    verbose_master(INFO_METRICS + 1, "End local signature");

    if (!eards_connected() && master){ 
        if (!EARL_LIGHT){ 
            return;
        }
    }

		verbose_master(2, "Partial stop");

    /* Synchro 1, waiting for last loop data: partial stop: master will wait if. Others will wait for master signature */
    while ((metrics_partial_stop(SIG_BEGIN) == EAR_NOT_READY) && (wait_time < MAX_SYNCHO_TIME) && lib_shared_region->earl_on){ 
		usleep(10000);
		wait_time += 0.01;
	}
	if (!lib_shared_region->earl_on){ 
		return;
	}
	metrics_compute_signature_data(LOO, metrics, 1, procs);

    /* Waiting for node signatures */
    verbose_master(INFO_METRICS+1, "(l.rank) %d: Waiting for loop node signatures", my_node_id);

    /* Synchro 2: Waiting for local signatures to compute loops.
     * Master will wait for the others and Other will wait for the master */
    wait_time = 0;
    exited = num_processes_exited(lib_shared_region,sig_shared_region);
    while ((exited < lib_shared_region->num_processes - 1) 
				&& are_signatures_ready(lib_shared_region,sig_shared_region, &num_ready) == 0 && (wait_time < MAX_SYNCHO_TIME)) {
        if (!(master) && lib_shared_region->global_stop_ready) {
            break;
        }
        if (master && (exited == lib_shared_region->num_processes - 1)) {
            break;
        }
        usleep(10000);
        wait_time += 0.01;
        exited = num_processes_exited(lib_shared_region, sig_shared_region);
    }

    // Only master does something in this funcion
	compute_per_process_and_job_metrics(&nodeapp);

	/* At this point we mark signatures are not ready to be computed again */
    if (master)  {
        free_node_signatures(lib_shared_region,sig_shared_region);
        lib_shared_region->global_stop_ready = 1;
    }
	verbose_master(2, "Global stop");
	metrics_global_stop();
	verbose_master(2, "metrics_compute_signature_data APP");
	metrics_compute_signature_data(APP, metrics, 1, procs);

    /* Waiting for node signatures */
    //verbose(2, "(l. rank) %d: Waiting for app node signatures", my_node_id);
    /* Synchro 3: Waiting for app signature: global stop */
    wait_time = 0;
    exited = num_processes_exited(lib_shared_region,sig_shared_region);
    while ( (exited != lib_shared_region->num_processes) && (are_signatures_ready(lib_shared_region,sig_shared_region, &num_ready) == 0) 
            && (wait_time < MAX_SYNCHO_TIME) ){ 
        usleep(100000);
        wait_time += 0.01;
        exited = num_processes_exited(lib_shared_region,sig_shared_region);
    }

    /* Estimate local metrics, accumulate */
    if (master) {
        verbose_master(2, "Computing application node signature");

        se = (sig_ext_t *) metrics->sig_ext;

        if (!exclusive) {
            metrics_app_node_signature(metrics, &nodeapp);
        } else {
            metrics_node_signature(&lib_shared_region->job_signature, &nodeapp);
        }

        signature_copy(metrics, &nodeapp);
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

/** Compute the total node I/O data. */
static state_t metrics_compute_total_io(io_data_t *total)
{
	if (total == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    }

	memset((char *) total, 0, sizeof(io_data_t));

	debug("Reading IO node data - #processes: %d", lib_shared_region->num_processes);

    for (int i = 0; i < lib_shared_region->num_processes; i++) {

        ctx_t local_io_ctx;

        if (state_fail(io_init(&local_io_ctx, sig_shared_region[i].pid))) {

            debug("IO data process %d (%d) can not be read", i, sig_shared_region[i].pid);

            return_msg(EAR_ERROR, "I/O could not be read for some process.");

        } else {
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
        }
    }

	debug("Node IO data ready - master: %d", masters_info.my_master_rank);

    return EAR_SUCCESS;
}

int time_ready_signature(ulong min_time_us)
{
    long long f_time;
    f_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);
    if (f_time<min_time_us) return 0;
    else return 1;
}

/****************************************************************************************************************************************************************/
/******************* This function checks if data is ready to be shared **************************************/
/****************************************************************************************************************************************************************/


int metrics_compute_signature_finish(signature_t *metrics, uint iterations, ulong min_time_us, ulong procs)
{
				long long aux_time;
                report_id_t report_id;
				uint num_ready;

				debug("metrics_compute_signature_finish %u", my_node_id);

                if (!sig_shared_region[my_node_id].ready){
                    
                    debug("Signature for proc. %d is not ready", my_node_id);

                    // Time requirements
                    aux_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);

                    //
                    if ((aux_time < min_time_us) && !equal_with_th((double)aux_time, (double)min_time_us,0.1)) {
                        metrics->time=0;
                        debug("EAR_NOT_READY because of time %llu\n",aux_time);
                        return EAR_NOT_READY;
                    }

                    //Master: Returns not ready when power is not ready
                    //Not master: when master signature not ready
                    if (metrics_partial_stop(SIG_END)==EAR_NOT_READY) return EAR_NOT_READY;

                    //Marks the signature as ready
                    metrics_compute_signature_data(LOO, metrics, iterations, procs);

                    // Call report mpitrace
                    report_create_id(&report_id, my_node_id, ear_my_rank, masters_info.my_master_rank);
                    if (state_fail(report_misc(&report_id, MPITRACE, (cchar *) &sig_shared_region[my_node_id], -1))) {
                        verbose(3, "%sERROR%s Reporting mpitrace for process %d", COL_RED, COL_CLR, my_node_id);
                    }

                    //
                    metrics_partial_start();
                } else {
                    debug("Signature for proc. %d is ready", my_node_id);
                }

                if (!are_signatures_ready(lib_shared_region, sig_shared_region, &num_ready)) {
                    if ((master) && (sig_shared_region[0].ready)){
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

  se = (sig_ext_t *)master->sig_ext;
  signature_copy(ns,master);
  if (se != NULL) memcpy(ns->sig_ext,se,sizeof(sig_ext_t));

	verbose_master(2, " metrics_app_node_signature");

  compute_total_node_instructions(lib_shared_region, sig_shared_region, &inst);
  compute_total_node_flops(lib_shared_region, sig_shared_region, FLOPS);
  compute_total_node_cycles(lib_shared_region, sig_shared_region, &cycles);
  compute_total_node_L1_misses(lib_shared_region, sig_shared_region, &L1);
  compute_total_node_L2_misses(lib_shared_region, sig_shared_region, &L2);
  compute_total_node_L3_misses(lib_shared_region, sig_shared_region, &L3);

  ns->CPI = (double)cycles/(double)inst;
	#if 0
  verbose_master(2,"CPI %lf CPI2 %lf",ns->CPI, CPI/(double)lib_shared_region->num_processes); */
  if (ns->CPI > 5.0){
    for (i=0; i< lib_shared_region->num_processes;i ++){
      verbosen_master(2," CPI[%d]=%lf",i,sig_shared_region[i].sig.CPI);
    } 
    verbose_master(2," ");
  }
	#endif
  for (i = 0; i < FLOPS_EVENTS; i++){
  	ns->FLOPS[i] = FLOPS[i];
  }    
  ns->Gflops = compute_node_flops(lib_shared_region, sig_shared_region);
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
			ns->DC_power   += sig_shared_region[lp].sig.accum_energy / valid_period;
			ns->DRAM_power += sig_shared_region[lp].sig.accum_dram_energy / valid_period;
			ns->PCK_power  += sig_shared_region[lp].sig.accum_pack_energy / valid_period;
			ns->GBS        += sig_shared_region[lp].sig.accum_mem_access / (valid_period * B_TO_GB);
			ns->avg_f      += (sig_shared_region[lp].sig.accum_avg_f * sig_shared_region[lp].num_cpus) / (ulong)valid_period;
			tcpus          += sig_shared_region[lp].num_cpus;
			accesses       += sig_shared_region[lp].sig.accum_mem_access;
			verbose_master(2," p[%u]: Power %lf GBs %lf avg_f %lu dram %lf pck %lf  period %lf", lp, (double)(sig_shared_region[lp].sig.accum_energy / valid_period), (double)(sig_shared_region[lp].sig.accum_mem_access/ (valid_period * 1024 * 1024 * 1024)), sig_shared_region[lp].sig.accum_avg_f/(ulong)valid_period, ns->DRAM_power, ns->PCK_power, valid_period);
		}
		ns->avg_f /= (double)tcpus;
		ns->TPI = (double)accesses / (double)max_inst;
		verbose_master(2,"AVG  %lu", ns->avg_f);
  }
  signature_copy(&lib_shared_region->node_signature,ns);

}

void metrics_node_signature(signature_t *master,signature_t *ns)
{
	ullong inst;
	ullong cycles;
	ullong FLOPS[FLOPS_EVENTS];
	sig_ext_t *se;
	int i;
	ullong L1, L2, L3;


	se = (sig_ext_t *)master->sig_ext;	
	signature_copy(ns,master);
	if ((se != NULL) && (ns->sig_ext != NULL))  memcpy(ns->sig_ext,se,sizeof(sig_ext_t));
	
	compute_total_node_instructions(lib_shared_region, sig_shared_region, &inst);
	compute_total_node_flops(lib_shared_region, sig_shared_region, FLOPS);
	compute_total_node_cycles(lib_shared_region, sig_shared_region, &cycles);	
	compute_total_node_L3_misses(lib_shared_region, sig_shared_region, &L3);
  compute_total_node_L2_misses(lib_shared_region, sig_shared_region, &L2);
  compute_total_node_L1_misses(lib_shared_region, sig_shared_region, &L1);

	ns->CPI = (double)cycles/(double)inst;
	/* verbose_master(2,"CPI %lf CPI2 %lf",ns->CPI, CPI/(double)lib_shared_region->num_processes); */
	if (ns->CPI > 2.0){
		for (i=0; i< lib_shared_region->num_processes;i ++){
			verbosen_master(2," CPI[%d]=%lf",i,sig_shared_region[i].sig.CPI);
		}	
		verbose_master(2," ");
	}
	ns->Gflops = compute_node_flops(lib_shared_region, sig_shared_region);
  ns->L1_misses  = L1;
  ns->L2_misses  = L2;
  ns->L3_misses  = L3;
  ns->cycles     = cycles;
  ns->instructions = inst;
  for (uint i = 0; i < FLOPS_EVENTS; i++){
  	ns->FLOPS[i] = FLOPS[i];
  }    

	verbose_master(2,"Node metrics: CPI %.2lf Gflops %.2lf", ns->CPI, ns->Gflops);


	#if 0
	signature_copy(&lib_shared_region->node_signature,ns);

	if (master){
		/* Updating shared info */
  	update_earl_node_mgr_info();
  	estimate_power_and_gbs(master,lib_shared_region,sig_shared_region, node_mgr_job_info, ns, PER_JOB, -1);
	}
	#endif
}

extern uint last_earl_phase_classification;
extern gpu_state_t last_gpu_state;

state_t metrics_new_iteration(signature_t *sig)
{
#if USE_GPUS
    int p;
#endif

    if (!(master)) {
        return EAR_ERROR;
    }

    /* FLOPS are used to determine the CPU BUSY waiting */
    uint last_phase_io_bnd = (last_earl_phase_classification == APP_IO_BOUND);
    uint last_phase_bw     = (last_earl_phase_classification == APP_BUSY_WAITING);
    if (last_phase_io_bnd || last_phase_bw)
    {
        llong f_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);
        llong elap_sec = f_time / 1000000;

        debug("--- Computing flops because of application phase ---\n"
                "%-26s: %d\n%-26s: %d\n"
                "%-26s: %lld\n%-26s: %lld\n"
                "----------------------------------------------------",
                "IO", last_phase_io_bnd, "Busy waiting", last_phase_bw,
                "Time elapsed", elap_sec, "Last elapsed", last_elap_sec);

        if (elap_sec <= last_elap_sec) {
            return EAR_ERROR;
        }

        last_elap_sec = elap_sec;

        // FLOPs
        flops_read_diff(no_ctx, &flops_read2[LOO],
                &flops_read1[LOO], &flops_diff[LOO], &sig->Gflops);
        flops_help_toold(&flops_diff[LOO], sig->FLOPS);

        sig->time = elap_sec;
        debug("GFlops in metrics for validation: %lf", sig->Gflops);
    }

#if USE_GPUS
    sig->gpu_sig.num_gpus = 0;
    if (last_gpu_state & _GPU_Idle) {
        debug("Computing GPU util because of application gpu phase is IDLE");
        /* GPUs */
        if (gpu_initialized){
            if (gpu_lib_read(&gpu_lib_ctx,gpud_busy) != EAR_SUCCESS){
                debug("gpu_lib_read error");
            }
            gpu_lib_data_diff(gpud_busy, gpu_metrics_init[LOO], gpuddiff_busy);
            sig->gpu_sig.num_gpus = ear_num_gpus_in_node;
            if (earl_gpu_model == MODEL_DUMMY){
                debug("Setting num_gpu to 0 because model is DUMMY");
                sig->gpu_sig.num_gpus = 0;
            }
            for (p=0; p < sig->gpu_sig.num_gpus;p++){
                sig->gpu_sig.gpu_data[p].GPU_power    = gpuddiff_busy[p].power_w;
                sig->gpu_sig.gpu_data[p].GPU_freq     = gpuddiff_busy[p].freq_gpu;
                sig->gpu_sig.gpu_data[p].GPU_mem_freq = gpuddiff_busy[p].freq_mem;
                sig->gpu_sig.gpu_data[p].GPU_util     = gpuddiff_busy[p].util_gpu;
                sig->gpu_sig.gpu_data[p].GPU_mem_util = gpuddiff_busy[p].util_mem;
            }
        }
    }
#endif
				return EAR_SUCCESS;
}

/* This function computes local metrics per process, computes per job and computes the accumulated */
void compute_per_process_and_job_metrics(signature_t *dest)
{
	if (master) {
		update_earl_node_mgr_info();
		estimate_power_and_gbs(lib_shared_region, sig_shared_region, node_mgr_job_info)	;
		accum_estimations(lib_shared_region, sig_shared_region);
		signature_copy(dest, &lib_shared_region->job_signature);	
	}
}

