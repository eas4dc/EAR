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


#include <library/policies/policy.h>

#include <sched.h>
#include <math.h>
#if MPI
#include <mpi.h>
#endif
#include <dlfcn.h>
#include <semaphore.h>

#include <common/includes.h>
#include <common/system/symplug.h>
#include <common/environment.h>
#include <common/types/generic.h>
#include <common/types/pc_app_info.h>
#include <common/hardware/hardware_info.h>
#include <common/sizes.h>
#include <common/utils/string.h>

#include <management/cpufreq/frequency.h>
#include <management/imcfreq/imcfreq.h>
#include <management/cpufreq/cpufreq.h>
#if USE_GPUS
#include <management/gpu/gpu.h>
#endif

#include <report/report.h>
#include <daemon/local_api/eard_api.h>
#include <library/loader/module_mpi.h>
#include <library/loader/module_cuda.h>
#include <library/api/mpi_support.h>
#include <library/api/clasify.h>
#include <library/metrics/metrics.h>
#include <library/policies/policy_ctx.h>
#include <library/policies/common/imc_policy_support.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/common/gpu_support.h>
#include <library/policies/common/cpu_support.h>
#include <library/policies/common/pc_support.h>
#include <library/policies/common/cpuprio_support.h>
#include <library/policies/policy_state.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/common/global_comm.h>
#include <library/common/utils.h>
#include <library/tracer/tracer.h>
#include <library/tracer/tracer_paraver_comp.h>


/* Verbose levels */
#define POLICY_INFO   2
#define POLICY_PHASES 2
#define POLICY_GPU    2


/* Wrap verbosing policy general info.
 * Currently, it is a verbose master with level 2. */
#define verbose_policy_info(msg, ...) \
    verbose_info2_master("Policy: " msg, ##__VA_ARGS__);

/* Must be migrated to config_env */
#define GPU_ENABLE_OPT "EAR_GPU_ENABLE_OPT"

#define is_master (masters_info.my_master_rank >= 0)


static ear_event_t curr_policy_event;
extern uint report_earl_events;
extern report_id_t rep_id;

extern sem_t *lib_shared_lock_sem;


#define REPORT_PHASE(phase) \
  if (report_earl_events){ \
    fill_event(&curr_policy_event, EARL_PHASE, phase); \
    report_events(&rep_id, &curr_policy_event, 1); \
  }


#define REPORT_OPT_ACCURACY(phase) \
  if (report_earl_events){ \
    fill_event(&curr_policy_event, EARL_OPT_ACCURACY, phase); \
    report_events(&rep_id, &curr_policy_event, 1); \
  }


// TODO: Can below macro be defined in a more coherent file?
#define GHZ_TO_KHZ 1000000.0

#define POLICY_MPI_CALL_ENABLED 1 // Enables doing stuff at MPI call level.
																	// Useful for fast overhead testing.

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define DEF_FREQ(f) (!ext_def_freq?f:ext_def_freq)
#else
#define DEF_FREQ(f) f
#endif

extern masters_info_t masters_info;

extern cpu_set_t ear_process_mask;
extern int       ear_affinity_is_set;

extern       uint *imc_max_pstate; // Hardware, not config
extern       uint *imc_min_pstate; // Hardware, not config
extern       uint  imc_devices;
extern const pstate_t *imc_pstates;

extern pc_app_info_t *pc_app_info_data;

/* Node mgr data */
extern ear_njob_t *node_mgr_data;
extern uint node_mgr_index;
extern node_mgr_sh_data_t *node_mgr_job_info;

/** Options */

/* Uncore Frequency Scaling */
uint conf_dyn_unc   = EAR_eUFS;
uint dyn_unc        = EAR_eUFS;
double imc_extra_th = EAR_IMC_TH;

/* Load Balance */
uint enable_load_balance = EAR_USE_LB;
uint use_turbo_for_cp    = EAR_USE_TURBO_CP;

/* Try Turbo */
uint try_turbo_enabled = 0;

/* Energy models */
uint use_energy_models = 1;

/* Exclusive mode */
static uint exclusive_mode = 0;

/* CPU and IMC lower bounds */
static ullong min_cpufreq  = 0; // CPu freq. selection works with khz
static ullong min_imcfreq  = 0;
uint   max_policy_imcfreq_ps = ps_nothing; // IMC freq. selection works with pstates

uint gpu_optimize = 0;

static pstate_t *cpu_pstate_lst; /*!< The list of available CPU pstates in the current system.*/
static uint      cpu_pstate_cnt;

static pstate_t *imc_pstate_lst; /*!< The list of available IMC pstates in the current system.*/
static uint      imc_pstate_cnt;

static uint use_earl_phases = EAR_USE_PHASES;
static ulong init_def_cpufreq_khz;
//  NEW CLASSIFY
extern ear_classify_t phases_limits;
uint policy_cpu_bound = 0;
uint policy_mem_bound = 0;

int my_globalrank;

static signature_t policy_last_local_signature;
static signature_t policy_last_global_signature;

polctx_t my_pol_ctx;

unsigned long long int total_mpi = 0; // This variable only counts the number of MPI calls and it's only used
                                      // at state_comm::verbose_signature. It only is useful when only the master verboses,
                                      // so it's value only shows master info while that signature verbose message tries to
                                      // show per-node info.

node_freqs_t nf;     /*!< Nominal frequency */
node_freqs_t avg_nf;
node_freqs_t last_nf;

static node_freqs_t per_process_node_freq;

static ulong max_freq_per_socket[MAX_SOCKETS_SUPPORTED];
static uint  cpu_must_be_normalized[MAX_CPUS_SUPPORTED];

uint last_earl_phase_classification = APP_COMP_BOUND;
static uint ignore_affinity_mask = 0;

int cpu_ready = EAR_POLICY_NO_STATE;
int gpu_ready = EAR_POLICY_NO_STATE;

gpu_state_t curr_gpu_state;
gpu_state_t last_gpu_state = _GPU_NoGPU;             // Initial GPU state as Computation

static uint fake_io_phase = 0;
static uint fake_bw_phase = 0;

const int   polsyms_n = 18;
const char *polsyms_nam[] = {
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
    "policy_cpu_gpu_settings",
    "policy_busy_wait_settings",
    "policy_restore_settings",
};

static polsym_t polsyms_fun;

static ulong *freq_per_core;

static node_freq_domain_t freqs_domain;

static const ulong **gpuf_pol_list;
static const uint   *gpuf_pol_list_items;

static uint first_policy_try = 1;

static uint total_mpi_optimized =  0;

/* Governor management for exclusive mode */
static uint *governor_list_before_init = NULL;

#if MPI_OPTIMIZED
/* MPI optimization */
extern uint ear_mpi_opt;
#endif

void fill_common_event(ear_event_t *ev);
void fill_event(ear_event_t *ev, uint event, llong value);


/** This function is called automatically by the
 * init_power_policy that loads policy symbols. */
static state_t policy_init();

/** Fills \p ret_pol_name with the policy name that must be loaded.
 * If mgt_cpufreq API is correct, \p default_policy_name is used.
 * Otherwise 'monitoring' is used. */
static void select_policy_name(char *ret_pol_name, size_t pol_name_size, char *default_policy_name);

static void build_plugin_path(char *policy_plugin_path, size_t plug_path_size, settings_conf_t *app_settings);

/** Reads information about some management API and stores the pstate list
 * and devices count information to the output arguments. Pure function.
 * \param api_id[in]      The API id of the mgt API you want to get the information, e.g., MGT_CPUFREQ.
 * \param pstate_lst[out] The list of pstates read from \p api_id API.
 * \param pstate_cnt[out] The number of pstate available in \p pstate_lst. */
static state_t fill_mgt_api_pstate_data(uint api_id, pstate_t **pstate_lst, uint *pstate_cnt);

/** Maps the list of CPU frequencies mapped per process, to a list of CPU frequencies mapped per core.
 *
 * \param[in]  process_cpu_freqs_khz A list of frequencies indexed per local (node-level) rank.
 * \param[in]  process_cnt           The number of elements in \p process_cpu_freqs_khz.
 * \param[out] core_cpu_freqs_khz    A list of frequencies indexed per core.
 * \param[in]  cpu_cnt               The number of cores which is expected to be in core_cpu_freqs_khz.
 * \param[out] cpu_freq_max          If a non-NULL address is passed, the argument is filled with the maximum
 * CPU freq. found in \process_cpu_freqs_khz.
 * \param[out] cpu_freq_min          If a non-NULL address is passed, the argument is filled with the minimum
 * CPU freq. found in \process_cpu_freqs_khz.
 *
 * \return EAR_ERROR if some argument is invalid.
 * \return EAR_SUCCESS otherwise. */
static state_t from_proc_to_core(ulong *process_cpu_freqs_khz,
		int process_cnt,
		ulong **core_cpu_freqs_khz,
		int cpu_cnt,
		ulong *cpu_freq_max,
		ulong *cpu_freq_min,
		int process_id);

/** This function returns whether the CPU frequency selected by the policy itself must be selected.
 * The result depends on current Powercap status. */
static uint must_set_policy_cpufreq(pc_app_info_t *pcdata, settings_conf_t *settings_conf);

/** Calls the different methods for applying the frequency selection for the domains compatible with the
 * current loaded policy. It only works if master process (node-level) called the function. */
static state_t policy_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id);

static state_t policy_master_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id);

/** Returns the mean of the frequency list \p freq_list with size \p n. */
static ulong compute_avg_freq(ulong *freq_list, int n);

/** Sets the CPU frequency according to what was selected by the policy plugin.
 * \param[in] c The current policy context.
 * \param[in] my_sig The signature used by the policy plugin. Not used.
 * \param[in] freqs The set of frequencies for the different system domains (e.g., CPU, IMC, GPU).
 * \param[in] dom The domain of the current policy plugin.
 * \param[out] avg_freq After the function, this adress will contain the average CPU freq. selected. */
static state_t policy_cpu_freq_selection(polctx_t *c, signature_t *my_sig,
                                         node_freqs_t *freqs, node_freq_domain_t *dom,
                                         node_freqs_t *avg_freq, int process_id);

/** Verboses the list of frequencies \p core_cpu_freqs_khz, assuming that is given as the per CPU frequency
 * list, for a total number of \p cpu_cnt CPU/processors. Use \p verb_lvl to specify the verbosity level
 * required by the job to print the information. */
static void verbose_freq_per_core(int verb_lvl, ulong **core_cpu_freqs_khz, int cpu_cnt);

/** Verboses the elapsed time on each phase encountered
 * in the extended signature of the passed signature \p sig. */
static void verbose_time_in_phases(signature_t *sig);

/** Sets governor `userspace` to  CPUs that are
 * in my node mask or all if exclusive_mode is set */
static state_t policy_set_cpus_governor(uint exclusive);

/** Sets the governor list stored at governor_list_before_init. */
static state_t policy_restore_cpus_governor();

static void fill_cpus_out_of_cgroup(uint exc);

static void policy_new_phase(uint phase, signature_t *sig);

#define DEBUG_CPUFREQ_COST 0

#if DEBUG_CPUFREQ_COST
static ulong total_cpufreq_cost_us = 0;
static ulong calls_cpufreq = 0;
#endif

timestamp init_last_phase, init_last_comp;



/** This function loads specific CPU and GPU policy functions and calls the initialization functions. */
state_t init_power_policy(settings_conf_t *app_settings, resched_t *res)
{
	char policy_plugin_path[SZ_PATH]; // the complete policy plug-in path.
	memset(policy_plugin_path, 0, sizeof(policy_plugin_path));

	build_plugin_path(policy_plugin_path, sizeof(policy_plugin_path), app_settings);

	// Load policy plugin
	verbose_policy_info("Loading policy: %s", policy_plugin_path);

	if (state_fail(symplug_open(policy_plugin_path, (void **) &polsyms_fun, polsyms_nam, polsyms_n)))
	{
		verbose_error_master("Loading %s: %s", policy_plugin_path, state_msg);
	}

	ear_frequency                 = DEF_FREQ(app_settings->def_freq);
	init_def_cpufreq_khz          = ear_frequency;
	my_globalrank                 = masters_info.my_master_rank;

	// Init the policy context
	my_pol_ctx.affinity           = ear_affinity_is_set;
	my_pol_ctx.pc_limit	          = app_settings->pc_opt.current_pc;
	my_pol_ctx.mpi_enabled        = module_mpi_is_enabled();
	my_pol_ctx.num_pstates	      = frequency_get_num_pstates();
	my_pol_ctx.use_turbo          = ear_use_turbo;
	my_pol_ctx.user_selected_freq = DEF_FREQ(app_settings->def_freq);
	my_pol_ctx.reset_freq_opt     = get_ear_reset_freq();
	my_pol_ctx.ear_frequency      = &ear_frequency;
	my_pol_ctx.app	              = app_settings;
	my_pol_ctx.reconfigure        = res;
	// TODO: Missing gpu_mgt_ctx
	// TODO: Missing mpi
	my_pol_ctx.num_gpus = 0;

#if USE_GPUS
	if (metrics_gpus_get(MET_GPU)->ok)
	{
		my_pol_ctx.num_gpus = metrics_gpus_get(MET_GPU)->devs_count;
	}
#endif

	// Here it is a good place to print policy context:
	verbose_policy_ctx(POLICY_INFO, &my_pol_ctx);

#if MPI
	if (PMPI_Comm_dup(masters_info.new_world_comm, &my_pol_ctx.mpi.comm) != MPI_SUCCESS) {
		verbose_error_master("Duplicating COMM_WORLD in policy.");
	}

	if (masters_info.my_master_rank >= 0)
	{
		if (PMPI_Comm_dup(masters_info.masters_comm, &my_pol_ctx.mpi.master_comm) != MPI_SUCCESS) {
			verbose_error_master("Duplicating master_comm in policy.");
		}
	}
#endif

	// We fill available pstates info for both CPU and IMC APIs. this is a must in order to use
	// functions for converting a pstate value to a khz frequency, and viceversa.
	// The called function is defined at the end of this file.
	
	state_t st = fill_mgt_api_pstate_data(MGT_CPUFREQ, &cpu_pstate_lst, &cpu_pstate_cnt);
	if (state_fail(st))
	{
		verbose_error_master("Policy init could not read mgt_cpufreq: %s", state_msg);
	}

	st = fill_mgt_api_pstate_data(MGT_IMCFREQ, &imc_pstate_lst, &imc_pstate_cnt);
	if (state_fail(st))
	{
		verbose_error_master("Policy init could not read mgt_imcfreq: %s", state_msg);
	}

	// Set the minimum CPU and IMC freqs. to the max P-States set by the Admin in ear.conf

	debug("max cpu pstate: %u / cpu pstate cnt %u", (uint) app_settings->cpu_max_pstate, cpu_pstate_cnt);

#if 1
	uint elem_count;
	char **policy_path = strtoa(policy_plugin_path, '/', NULL, &elem_count);

	uint max_cpu_ps = cpu_pstate_cnt - 1;

	if (policy_path)
	{
		if (strcmp(policy_path[elem_count - 1], "monitoring.so") != 0)
		{
			max_cpu_ps = ear_min(max_cpu_ps, (uint) app_settings->cpu_max_pstate);
		}

		strtoa_free(policy_path);
	}
#else
	uint max_cpu_ps = cpu_pstate_cnt - 1;
	if (strcmp(app_settings->policy_name, "monitoring") != 0)
	{
			max_cpu_ps = ear_min(max_cpu_ps, (uint) app_settings->cpu_max_pstate);
	}
#endif


	if (state_fail(pstate_pstofreq(cpu_pstate_lst, cpu_pstate_cnt, max_cpu_ps, &min_cpufreq)))
	{
		verbose_error_master("Policy init could not read cpu_max_pstate: %s", state_msg);
	}

	verbose_policy_info("System Min. CPU freq. limited: %llu kHz ", min_cpufreq);

	debug("max imc pstate: %u / imc pstate cnt %u", (uint) app_settings->imc_max_pstate, imc_pstate_cnt);

	max_policy_imcfreq_ps = ear_min(imc_pstate_cnt - 1, (uint) app_settings->imc_max_pstate);
	if (state_fail(pstate_pstofreq(imc_pstate_lst, imc_pstate_cnt, max_policy_imcfreq_ps, &min_imcfreq)))
	{
		verbose_error_master("Policy init could not read imc_max_pstate: %s", state_msg);
	}

	verbose_policy_info("System Min. IMC freq. limited: %llu kHz", min_imcfreq);

	return policy_init();
}


/* This is the main function for frequency selection at application level */
/* The signatures are used first the last signatur computed is a global variable, second signatures are in the shared memory */
/* freq_set and ready are output values */
/* There is not a GPU part here, still pending */
/* The CPU freq and GPu freq are set in this function, so freq_set is just a reference for the CPU freq selected */
state_t policy_app_apply(ulong *freq_set, int *ready)
{
	state_t st = EAR_SUCCESS;

	if (polsyms_fun.app_policy_apply == NULL)
	{
		*ready = EAR_POLICY_LOCAL_EV;
		return st;
	}

	if (!eards_connected() || (masters_info.my_master_rank < 0))
	{
		*ready = EAR_POLICY_CONTINUE;
		return st;
	}

	node_freqs_t *freqs = &nf;
	if (freqs_domain.cpu == POL_GRAIN_CORE)
	{
		memset(freqs->cpu_freq, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);
	}

	*ready = EAR_POLICY_READY;
	st = polsyms_fun.app_policy_apply(&my_pol_ctx, &policy_last_global_signature, freqs, ready);

	if (*ready == EAR_POLICY_READY)
	{
		debug("Average frequency after app_policy is %lu",*freq_set );
		st = policy_master_freq_selection(&my_pol_ctx ,&policy_last_global_signature,freqs,&freqs_domain,&avg_nf, ALL_PROCESSES);
		*freq_set = avg_nf.cpu_freq[0];
	}
	return st;
}


/* This function confgures freqs for apps consuming lot GPU. */
state_t policy_cpu_gpu_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    REPORT_PHASE(APP_CPU_GPU);
    if (polsyms_fun.cpu_gpu_settings != NULL){
        polsyms_fun.cpu_gpu_settings(c,my_sig,freqs);
    }
    return EAR_SUCCESS;
}


/* This function confgures freqs based on IO criteria. */
state_t policy_io_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    REPORT_PHASE(APP_IO_BOUND);
    if (polsyms_fun.io_settings != NULL)
    {
        polsyms_fun.io_settings(c, my_sig, freqs);
    }
    return EAR_SUCCESS;
}


/* This function sets again default frequencies after a setting modification */
state_t policy_restore_comp_configuration(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    REPORT_PHASE(APP_COMP_BOUND);
    if (polsyms_fun.restore_settings != NULL) {
        polsyms_fun.restore_settings(c,my_sig,freqs);
        return policy_master_freq_selection(c,my_sig,freqs,dom,avg_freq, ALL_PROCESSES);
    } else {
        return EAR_SUCCESS;
    }
}


state_t policy_busy_waiting_configuration(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    REPORT_PHASE(APP_BUSY_WAITING);    
    if (polsyms_fun.busy_wait_settings != NULL){
        polsyms_fun.busy_wait_settings(c, my_sig, freqs);
    }
    return EAR_SUCCESS;
}


static state_t policy_init()
{
    polctx_t *c = &my_pol_ctx;

    char *cdyn_unc              = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_SET_IMCFREQ)       : NULL; // eUFS
    char *cimc_extra_th         = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_IMC_TH)            : NULL; // eUFS threshold
    char *cexclusive_mode       = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_EXCLUSIVE_MODE)    : NULL; // Job has the node exclusive
    char *cuse_earl_phases      = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_EARL_PHASES)       : NULL; // Detect application phases
    char *cenable_load_balance  = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_LOAD_BALANCE)      : NULL; // Load balance efficiency
    char *cuse_turbo_for_cp     = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_TURBO_CP)          : NULL; // Boost critical path processes
    char *ctry_turbo_enabled    = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_TRY_TURBO)         : NULL; // Boost turbo if policy sets max cpufreq
    char *cuse_energy_models    = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_USE_ENERGY_MODELS) : NULL; // Use energy models
    char *cmin_cpufreq          = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_MIN_CPUFREQ)       : NULL; // Minimum CPU freq.
    char *cfake_io_phase        = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_IO_FAKE_PHASE)     : NULL;
    char *cfake_bw_phase        = (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_BW_FAKE_PHASE)     : NULL;
    char *cignore_affinity_mask = ear_getenv(FLAG_NO_AFFINITY_MASK);

    char *cgpu_optimize = ear_getenv(GPU_ENABLE_OPT);
    if (cgpu_optimize != NULL) gpu_optimize = atoi(cgpu_optimize);

    verbose_policy_info("GPU optimization during GPU comp: %s", gpu_optimize ? "Enabled" : "Disabled");

#if MPI_OPTIMIZED
		char *cear_mpi_opt		= (system_conf->user_type == AUTHORIZED) ? ear_getenv(FLAG_MPI_OPT)           : NULL; // MPI optimizattion

		if (cear_mpi_opt != NULL) ear_mpi_opt = atoi(cear_mpi_opt);

		if (ear_mpi_opt)
		{
			verbose_policy_info("MPI optimization enabled.");
		}
#endif

    state_t retg = EAR_SUCCESS, ret = EAR_SUCCESS;

    /* To compute time consumed in each phase */
    timestamp_getfast(&init_last_phase);
    memcpy(&init_last_comp, &init_last_phase, sizeof(timestamp));

    /* We read FLAG_USE_ENERGY_MODELS before calling to polsyms_fun.init() because this function
     * may needs `use_energy_models` to configure itself. */
    if (cuse_energy_models != NULL) {
        use_energy_models = atoi(cuse_energy_models);
    }
    verbose_policy_info("Using energy models: %u", use_energy_models);

    if (cfake_io_phase != NULL) fake_io_phase = atoi(cfake_io_phase);
    if (cfake_bw_phase != NULL) fake_bw_phase = atoi(cfake_bw_phase);

    if (fake_io_phase || fake_bw_phase) {
        verbose_master(0, "%sWARNING%s Using FAKE I/O phase: %u FAKE BW phase: %u",
                       COL_RED, COL_CLR, fake_io_phase, fake_bw_phase);
    }

    if (cdyn_unc != NULL && metrics_get(MGT_IMCFREQ)->ok) {
        conf_dyn_unc = atoi(cdyn_unc);
    }
    dyn_unc = (metrics_get(MGT_IMCFREQ)->ok) ? conf_dyn_unc : 0;

    if (cimc_extra_th != NULL) {
        imc_extra_th = atof(cimc_extra_th);
    }

    if (cmin_cpufreq != NULL) {
        min_cpufreq = (ulong) atoi(cmin_cpufreq);
    }

    verbose_policy_info("Min. CPU freq. limited: %llu kHz / Min. IMC freq. limited: %llu kHz", min_cpufreq, min_imcfreq);

    // Calling policy plug-in init
    if (polsyms_fun.init != NULL) {

        ret = polsyms_fun.init(c);

        if (state_fail(ret)) {
            verbose_error_master("Policy init: %s", state_msg);
        }
    }

    // Read policy configuration
    if (cexclusive_mode != NULL) exclusive_mode = atoi(cexclusive_mode);
    verbose_policy_info("Running in exclusive mode: %u", exclusive_mode);

    if (is_master)
    {
        if (state_fail(policy_set_cpus_governor(exclusive_mode)))
        {
            verbose_warning("Governor configuration error: %s", state_msg);
        }
    }

    if (cuse_earl_phases != NULL) use_earl_phases = atoi(cuse_earl_phases);
    verbose_policy_info("Phases detection: %u", use_earl_phases);


    /* Even though the policy could be at the Node granularity, we will use a list */
#if USE_GPUS
    gpuf_pol_list       = (const ulong **) metrics_gpus_get(MGT_GPU)->avail_list;
    gpuf_pol_list_items = (const uint *)   metrics_gpus_get(MGT_GPU)->avail_count;
#endif // USE_GPUS
    node_freqs_alloc(&nf);
    node_freqs_alloc(&avg_nf);
    node_freqs_alloc(&last_nf);

		if (POLICY_MPI_CALL_ENABLED)
		{
			/* This is to be used in mpi_calls */
			node_freqs_alloc(&per_process_node_freq);
		}

    freq_per_core = calloc(MAX_CPUS_SUPPORTED, sizeof(ulong));
    signature_init(&policy_last_local_signature);
    signature_init(&policy_last_global_signature);
#if USE_GPUS
    policy_last_local_signature.gpu_sig.num_gpus  = metrics_gpus_get(MET_GPU)->devs_count;
    policy_last_global_signature.gpu_sig.num_gpus = policy_last_local_signature.gpu_sig.num_gpus;
#endif
    #if 0
    if (arch_desc.top.vendor == VENDOR_AMD) {
        enable_load_balance = 0;
    }
    #endif

    // TODO: This check is for the transition to the new environment variables.
    // It will be removed when SCHED_ENABLE_LOAD_BALANCE will be removed, in the next release.
    if (cenable_load_balance == NULL) {
        cenable_load_balance = (system_conf->user_type == AUTHORIZED) ? ear_getenv(SCHED_ENABLE_LOAD_BALANCE) : NULL;
        if (cenable_load_balance != NULL) {
            verbose_error("Warning: %s will be removed on the next EAR release. "
                    "Please, change it by %s in your submission scripts.",
                    SCHED_ENABLE_LOAD_BALANCE, FLAG_LOAD_BALANCE);
        }
    }
    if (cenable_load_balance != NULL) enable_load_balance = atoi(cenable_load_balance);
    /* EAR relies on the affinity mask, some special use cases (Seissol) modifies at runtime
     * the mask. Since EAR reads the mask during the initializacion, we offer this option
     * to switch off the utilization of the mask
     */
    if (cignore_affinity_mask != NULL)
    {
      enable_load_balance = 0;
      ignore_affinity_mask = 1;

      verbose_policy_info("Affinity mask explicitly ignored. Load balance off.");
    }

    verbose_policy_info("Load balance: %u", enable_load_balance);

    if (cuse_turbo_for_cp != NULL) use_turbo_for_cp = atoi(cuse_turbo_for_cp);
    verbose_policy_info("Turbo for CP when app in the node is unbalanced: %u", use_turbo_for_cp);

    try_turbo_enabled = c->use_turbo;

    // TODO: This check is for the transition to the new environment variables.
    // It will be removed when SCHED_TRY_TURBO will be removed, in the next release.

    if (ctry_turbo_enabled == NULL) {

        ctry_turbo_enabled = (system_conf->user_type == AUTHORIZED) ? ear_getenv(SCHED_TRY_TURBO) : NULL;

        if (ctry_turbo_enabled != NULL) {
            verbose_error("Warning: %s will be removed on the next EAR release. "
                    "Please, change it by %s in your submission scripts.",
                    SCHED_TRY_TURBO, FLAG_TRY_TURBO);
        }
    }

    if (ctry_turbo_enabled != NULL) try_turbo_enabled = atoi(ctry_turbo_enabled);
    verbose_policy_info("Turbo for cbound app in the node: %u", try_turbo_enabled);

    // Read the domain of the loaded policy plugin
    if (polsyms_fun.domain != NULL) {
        ret = polsyms_fun.domain(c, &freqs_domain);
    } else {
        freqs_domain.cpu = POL_GRAIN_NODE;
        freqs_domain.mem = POL_NOT_SUPPORTED;
        freqs_domain.gpu = POL_NOT_SUPPORTED;
        freqs_domain.gpumem = POL_NOT_SUPPORTED;
    }

#if 0
    // TODO
    if (earl_default_domains.mem == POL_NOT_SUPPORTED){
    }
#endif

    policy_show_domain(&freqs_domain);

    if (is_master)
		{
        // Powercap (?)
        pc_support_init(c);
    }

    fill_common_event(&curr_policy_event);
    REPORT_OPT_ACCURACY(OPT_NOT_READY);

    /* We have migrated frequency set from eard to earl */
    policy_get_default_freq(&nf);

    if (is_master) {
      for (uint lp = 0 ; lp < lib_shared_region->num_processes; lp++) {
        sig_shared_region[lp].unbalanced = 0;
        sig_shared_region[lp].new_freq = nf.cpu_freq[0];
      }
    }

    /* What do we need at this point in the signature ? */
    policy_master_freq_selection(c, &policy_last_local_signature, &nf,
                                 &freqs_domain, &avg_nf, ALL_PROCESSES);

    if (is_master && !ignore_affinity_mask)
    {
        policy_setup_priority();
    }

    if ((ret == EAR_SUCCESS) && (retg == EAR_SUCCESS)) return EAR_SUCCESS;
    else return EAR_ERROR;
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
/* The CPU and GPU freqs are set in this function, so freq_set is just a reference for the CPU freq selected */
state_t policy_node_apply(signature_t *my_sig, ulong *freq_set, int *ready)
{
    uint busy, iobound, mpibound, earl_phase_classification;
    uint jobs_in_node;

    polctx_t *c = &my_pol_ctx;

    node_freqs_t *freqs = &nf;

    state_t st = EAR_ERROR;

    verbose_policy_info("EAR policy...");

    cpu_ready = EAR_POLICY_NO_STATE;
    gpu_ready = EAR_POLICY_NO_STATE;

    if (!eards_connected() || (masters_info.my_master_rank < 0)) {
#if EARL_LIGHT
        if (is_master) {
            clean_signatures(lib_shared_region, sig_shared_region);
        }
				*ready = EAR_POLICY_READY;
#else
        *ready = EAR_POLICY_CONTINUE;
#endif
				verbose_policy_info("EAR policy... ready");
        return EAR_SUCCESS;
    }

    if (!is_monitor_mpi_calls_enabled()) enable_load_balance = 0;

    verbose_master(POLICY_INFO + 1, "%sPOLICY%s Policy node apply", COL_BLU, COL_CLR);

		if (!lib_shared_region->estimate_node_metrics){
			verbose_master(POLICY_INFO,"%sPOLICY%s Policy not applied in EARL[%d] because is master in master/worker use case", COL_BLU, COL_CLR, getpid());
			*ready = EAR_POLICY_READY;
			return EAR_SUCCESS;
		}

    //nodemgr_get_num_jobs_attached(&jobs_in_node);
    jobs_in_node = 1;
    if (jobs_in_node > 1) {
      verbose_jobs_in_node(2, node_mgr_data, node_mgr_job_info);
    }

    if (jobs_in_node > 1) dyn_unc = 0;
    else                  dyn_unc = conf_dyn_unc;

    /* End update */

    //  NEW CLASSIFY
    is_network_bound(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &mpibound);
    is_io_bound(     &lib_shared_region->job_signature, lib_shared_region->num_cpus, &iobound);
    classify(iobound, mpibound, &earl_phase_classification);
    is_cpu_busy_waiting(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &busy);

    gpu_activity_state(&lib_shared_region->job_signature, &curr_gpu_state); // GPU phase classification
    if (curr_gpu_state & _GPU_Idle)
    {
        verbose_master(POLICY_GPU, "%sGPU idle phase%s", COL_BLU, COL_CLR);
    }

    /* Applications gpu bound cannot be applied as CPU-GPU */
    if ((curr_gpu_state & _GPU_Bound) && (busy != BUSY_WAITING))
    {
      earl_phase_classification = APP_CPU_GPU;
    }

    //  NEW CLASSIFY
    verbose_master(POLICY_PHASES, "EARL phase %s: Busy Waiting %u / I/O bound %u / MPI bound %u / Using phases %u",
        phase_to_str(earl_phase_classification), busy, iobound, mpibound, use_earl_phases);

    if (use_earl_phases)
    {
      REPORT_OPT_ACCURACY(OPT_OK);
      /* CPU-GPU Code */
      if (earl_phase_classification == APP_CPU_GPU)
      {
            // verbose_master(POLICY_PHASES,"%sCPU-GPU application%s", COL_BLU,COL_CLR);
            verbose_master(1, "%sCPU-GPU application%s (%s)", COL_BLU, COL_CLR, node_name);

            // TODO: The below function does not modify avg_nf, but it's used just after the function call.
            st = policy_cpu_gpu_freq_selection(c, my_sig, freqs, &freqs_domain, &avg_nf);
            *freq_set = avg_nf.cpu_freq[0];

            policy_new_phase(last_earl_phase_classification, my_sig);

            last_earl_phase_classification = earl_phase_classification;
            last_gpu_state = curr_gpu_state;

            policy_master_freq_selection(c,my_sig,freqs,&freqs_domain,&avg_nf, ALL_PROCESSES);

            clean_signatures(lib_shared_region, sig_shared_region);

            if (is_all_ready())
            {
              *ready = EAR_POLICY_READY;
            } else {
              *ready = EAR_POLICY_CONTINUE;
            }
            return EAR_SUCCESS;
      }

      /* IO Use case */
      if (earl_phase_classification == APP_IO_BOUND)
      {
          verbose_master(POLICY_PHASES,"%sPOLICY%s I/O configuration", COL_BLU, COL_CLR);

          // TODO Below function always returns EAR_SUCCESS
          st = policy_io_freq_selection(c, my_sig, freqs, &freqs_domain, &avg_nf);

          // TODO: GPU ?
          // TODO: avg_nf is not set on the above function
          *freq_set = avg_nf.cpu_freq[0];

          policy_new_phase(last_earl_phase_classification, my_sig);

          last_earl_phase_classification = earl_phase_classification;
          last_gpu_state = curr_gpu_state;

          policy_master_freq_selection(c, my_sig, freqs, &freqs_domain, &avg_nf, ALL_PROCESSES);

          clean_signatures(lib_shared_region, sig_shared_region);

          if (is_all_ready())
          {
              *ready = EAR_POLICY_READY;
          } else
          {
              *ready = EAR_POLICY_CONTINUE;
          }
          return EAR_SUCCESS;
      }

      /* Restore configuration to COMP or MPI if needed */
      if ((earl_phase_classification == APP_COMP_BOUND) ||
          (earl_phase_classification == APP_MPI_BOUND))
      {
          /* Are we in a busy waiting phase ? */
          if (busy == BUSY_WAITING)
          {
              verbose_master(POLICY_PHASES, "%sPOLICY%s CPU Busy waiting configuration (%s)",
                             COL_BLU, COL_CLR, node_name);

              st = policy_busy_waiting_configuration(c, my_sig, freqs, &freqs_domain, &avg_nf);

              // TODO: avg_nf is not set on the above function
              *freq_set = avg_nf.cpu_freq[0];

              policy_master_freq_selection(c, my_sig, freqs, &freqs_domain, &avg_nf, ALL_PROCESSES);

              last_gpu_state = curr_gpu_state;
              policy_new_phase(last_earl_phase_classification, my_sig);

              last_earl_phase_classification = APP_BUSY_WAITING;
              clean_signatures(lib_shared_region, sig_shared_region);

              if (is_all_ready())
              {
                  *ready = EAR_POLICY_READY;
              } else {
                  *ready = EAR_POLICY_CONTINUE;
              }

              return EAR_SUCCESS;
          }

          /* If not, we should restore to default settings */
          if ((last_earl_phase_classification == APP_BUSY_WAITING) ||
              (last_earl_phase_classification == APP_IO_BOUND) ||
              !(last_gpu_state & curr_gpu_state))
          {
              verbose_master(POLICY_PHASES, "%sPOLICY%s COMP-MPI configuration", COL_BLU, COL_CLR);

              st = policy_restore_comp_configuration(c, my_sig, freqs,
                                                     &freqs_domain, &avg_nf);

              *freq_set = avg_nf.cpu_freq[0];

              policy_new_phase(last_earl_phase_classification, my_sig);

              last_earl_phase_classification = earl_phase_classification;
              last_gpu_state = curr_gpu_state;

              if (earl_phase_classification == APP_COMP_BOUND) timestamp_getfast(&init_last_comp);

              clean_signatures(lib_shared_region, sig_shared_region);

              *ready = EAR_POLICY_CONTINUE;

              return st;
          }
      } // COMP_BOUND OR MPI_BOUND
    } // EAR phase decisions

    /* Here we are COMP or MPI bound AND not busy waiting. */
    verbose_master(POLICY_PHASES,
            "%sPOLICY%s COMP or MPI bound and not busy waiting: Applying node policy...",
            COL_BLU, COL_CLR);
    policy_new_phase(last_earl_phase_classification, my_sig);

    last_earl_phase_classification     = earl_phase_classification;

    if (earl_phase_classification == APP_COMP_BOUND) timestamp_getfast(&init_last_comp);
    is_cpu_bound(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &policy_cpu_bound);
    is_mem_bound(&lib_shared_region->job_signature, lib_shared_region->num_cpus, &policy_mem_bound);

    if (fake_io_phase)                 last_earl_phase_classification = APP_IO_BOUND;
    if (fake_bw_phase)                 last_earl_phase_classification = APP_BUSY_WAITING;

    last_gpu_state = curr_gpu_state;

    if (polsyms_fun.node_policy_apply)
    {
        /* Initializations */
        signature_t node_sig;
        signature_copy(&node_sig, my_sig);

        // We remove here the GPU power, as models are not trained with it.
        node_sig.DC_power = sig_node_power(my_sig);
        signature_copy(&policy_last_local_signature, &node_sig);

        if ((cpu_ready != EAR_POLICY_READY) &&
            (freqs_domain.cpu == POL_GRAIN_CORE))
        {
            memset(freqs->cpu_freq, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);
        }

        /* CPU specific function is applied here */
        st = polsyms_fun.node_policy_apply(c, &node_sig, freqs, &cpu_ready);

        // if (cpu_ready) compute_policy_savings(&node_sig, freqs, &freqs_domain);

        /* Depending on the policy status and granularity we adapt the CPU freq selection */
        if (is_all_ready() || is_try_again())
        {
            /* This function checks the powercap and the processor affinity,
             * finally it sets the CPU freq. asking the eard to do it. */
            st = policy_master_freq_selection(c, &node_sig, freqs, &freqs_domain, &avg_nf, ALL_PROCESSES);
            *freq_set = avg_nf.cpu_freq[0];
        } /* Stop*/
    } else if (polsyms_fun.app_policy_apply != NULL)
    {
        *ready = EAR_POLICY_GLOBAL_EV;
    } else
    {
        if (c != NULL)
        {
            *freq_set = DEF_FREQ(c->app->def_freq);
            cpu_ready = EAR_POLICY_READY;
        }
    }

    if (is_all_ready())
    {
        *ready = EAR_POLICY_READY;
    } else
    {
        *ready = EAR_POLICY_CONTINUE;
    }

    if (VERB_ON(POLICY_INFO))
    {
        if (*ready == EAR_POLICY_READY)
        {
            verbose_policy_info("%sNode policy ready.%s", COL_GRE, COL_CLR);
        }
        verbose_master(POLICY_INFO + 1, "%sPOLICY%s CPU freq. policy ready: %d\n"
                "GPU policy ready: %d\nGlobal node readiness: %d", COL_BLU, COL_CLR,
                cpu_ready, gpu_ready, *ready);
    }

    clean_signatures(lib_shared_region, sig_shared_region);

    return EAR_SUCCESS;
}


state_t policy_get_default_freq(node_freqs_t *freq_set)
{
    if (masters_info.my_master_rank < 0)
    {
        return EAR_SUCCESS;
    }

    if (polsyms_fun.get_default_freq != NULL) {
        return polsyms_fun.get_default_freq(&my_pol_ctx, freq_set, NULL);
    }

    verbose_warning("The policy plug-in has no symbol for getting the default frequency. Default frequency: %lu",
                    DEF_FREQ(my_pol_ctx.app->def_freq));

    // Below code overwrites the approach where only the index 0 of the destination array was filled.
    // A conflict appeared for a plug-in with fine-grain cpu domain but not get_default_freq implementation.

    // TODO: As the logic of this function is always the same,
    // it would be preferably if we avoid iterating all times we enter into this function.
    for (int i = 0; i < MAX_CPUS_SUPPORTED; i++)
    {
        freq_set->cpu_freq[i] = DEF_FREQ(my_pol_ctx.app->def_freq);
    }

		sig_ext_t *sigext = (sig_ext_t *)application.signature.sig_ext;
		/* Copy avg selected mem freq */
		compute_avg_sel_freq(&sigext->sel_mem_freq_khz, freq_set->imc_freq);

    return EAR_SUCCESS;
}


/* This function sets the default freq for both (CPU and GPU policies). */
state_t policy_set_default_freq(signature_t *sig)
{
    signature_t s;

    /* WARNING: sig can be NULL */

    if (masters_info.my_master_rank < 0 ) return EAR_SUCCESS;
    verbose_master(2,"policy_set_default_freq");

#if USE_GPUS
    s.gpu_sig.num_gpus = my_pol_ctx.num_gpus;
#endif

    if (polsyms_fun.restore_settings != NULL) {

        state_t st = polsyms_fun.restore_settings(&my_pol_ctx, sig, &nf);
        if (state_ok(st)) {
            return policy_master_freq_selection(&my_pol_ctx, &s, &nf, &freqs_domain, &avg_nf, ALL_PROCESSES);
        } else {
            return st;
        }
    } else {
        return EAR_ERROR;
    }
}


/* Uncomment this option to use energy savings estimations to decide if policy is ok */
//#define USE_ENERGY_SAVING
/* This function checks the CPU freq selection accuracy (only CPU), ok is an output */
state_t policy_ok(signature_t *curr, signature_t *prev, int *ok)
{
    /* Below definition wraps verbose messages triggered from this function.
     * It is a verbose master with level 2. */
#define verbose_policy_ok(msg, ...) \
    verbose_policy_info("%sPOLICY OK%s " msg, COL_BLU, COL_CLR, ##__VA_ARGS__)

    if (masters_info.my_master_rank < 0) {
        *ok = 1;
        return EAR_SUCCESS;
    }

    if (curr == NULL || prev == NULL) {
        *ok = 1;
        // TODO: Should we report OPT_OK event value?
        return_msg(EAR_ERROR, Generr.input_null);
    }

    polctx_t *c = &my_pol_ctx;
    state_t ret, retg = EAR_SUCCESS;
    uint num_ready;
    uint jobs_in_node;

    sig_ext_t *sig_ext = curr->sig_ext;
    sig_ext->saving = 0;

    /* Update node info */
    /* update_earl_node_mgr_info(); */
    nodemgr_get_num_jobs_attached(&jobs_in_node);

    // By now, only the master will reach this instruction.
    verbose_policy_info("There are %u jobs in the node", jobs_in_node);

    if (jobs_in_node > 1) {
      verbose_jobs_in_node(2, node_mgr_data, node_mgr_job_info);
    }
    /* End update */

    /* estimate_power_and_gbs(curr,lib_shared_region,sig_shared_region, node_mgr_job_info, &new_sig, PER_JOB , -1); */
    verbose(3, "Job's GB/s: %.2lf - Job's Power: %.2lf", curr->GBS, curr->DC_power);

    if (polsyms_fun.ok != NULL)
    {
        /* We wait for node signature computation */
        if (!are_signatures_ready(lib_shared_region, sig_shared_region, &num_ready)) {
            *ok = 1;

            REPORT_OPT_ACCURACY(OPT_OK);
            
            verbose_policy_ok("OK: not all signatures are ready.");

            return EAR_SUCCESS;
        }

        pc_support_compute_next_state(c, &my_pol_ctx.app->pc_opt, curr);

        ret = polsyms_fun.ok(c, curr, prev, ok);

        // char *ok_color = COL_GRE;

        if (*ok == 0) {
            // REPORT_OPT_ACCURACY(OPT_NOT_OK);

            policy_new_phase(last_earl_phase_classification, curr);

            last_earl_phase_classification = APP_COMP_BOUND;
            last_gpu_state = _GPU_Comp;

            timestamp_getfast(&init_last_comp);

            // ok_color = COL_RED;
        }
        verbose_policy_ok("%s: Decided by the policy plug-in.", *ok ? "OK" : "Not OK");

#if 0
        /* Migrated to states */
        /* And then we mark as not ready */
        verbose(3, "%sPOLICY OK%s Cleaning signatures...", COL_BLU, COL_CLR);
        clean_signatures(lib_shared_region,sig_shared_region);
#endif

        // TODO By now, retg is not used since its definition
        if ((ret != EAR_SUCCESS) || (retg != EAR_SUCCESS)) {

            uint opt_acc_val = (*ok) ? OPT_OK : OPT_NOT_OK;
            REPORT_OPT_ACCURACY(opt_acc_val);

            return EAR_ERROR;
        }
    } else {

        *ok = 1;

        verbose_policy_ok("OK: The policy plug-in does not implement this method.");
    }

    // We finally report the OPT_ACCURACY
    uint opt_acc_val = (*ok) ? OPT_OK : OPT_NOT_OK;
    REPORT_OPT_ACCURACY(opt_acc_val);

    return EAR_SUCCESS;
}


/* This function returns the maximum mumber of attempts to select the optimal CPU frequency. */
state_t policy_max_tries(int *intents)
{
    polctx_t *c=&my_pol_ctx;
    if (masters_info.my_master_rank < 0){*intents = 100000; return EAR_SUCCESS;}
    if (polsyms_fun.max_tries!=NULL)
    {
        return	polsyms_fun.max_tries( c,intents);
    } else
    {
        *intents=1;
        return EAR_SUCCESS;
    }
}


/* This function is executed at application end */
state_t policy_end()
{
    polctx_t *c = &my_pol_ctx;

    verbose_master(2, "Total number of MPI calls %llu", total_mpi);
	
#if MPI_OPTIMIZED
	if (ear_mpi_opt)
	{
		verbose(1, "[%d] Total number of MPI calls with optimization: %u", my_node_id, total_mpi_optimized);
	}
#endif

#if DEBUG_CPUFREQ_COST
    verbose_master(1, "Overhead of cpufreq setting %lu us: Total calls %lu: Cost per call %f",
                   total_cpufreq_cost_us, calls_cpufreq, (float)total_cpufreq_cost_us/(float)calls_cpufreq);
#endif

    policy_reset_priority();
    if (is_master && exclusive_mode)
    {
        policy_restore_cpus_governor();
    }

    preturn(polsyms_fun.end, c);
}


/** LOOPS */
/* This function is executed  at loop init or period init */
state_t policy_loop_init(loop_id_t *loop_id)
{
    preturn(polsyms_fun.loop_init, &my_pol_ctx, loop_id);
}


/* This function is executed at each loop end. */
state_t policy_loop_end(loop_id_t *loop_id)
{
	polctx_t *c = &my_pol_ctx;
	preturn(polsyms_fun.loop_end, c, loop_id);
}

/* This function is executed at each loop iteration or beginning of a period */
state_t policy_new_iteration(signature_t *sig)
{
    // TODO: Here we are doing things useless in monitoring mode.
    /* This validation is common to all policies */
    /* Check cpu idle or gpu idle */
    uint last_phase_io_bnd = (last_earl_phase_classification == APP_IO_BOUND);
    uint last_phase_bw     = (last_earl_phase_classification == APP_BUSY_WAITING);


    if ((last_phase_io_bnd || last_phase_bw) && metrics_CPU_phase_ok()){
			verbose_policy_info("Validating CPU phase: (GFlop/s = %.2lf, IO = %.2lf CPI %.2lf)", sig->Gflops, sig->IO_MBS, sig->CPI);

        uint restore = 0;

        if (last_phase_io_bnd && (sig->IO_MBS < phases_limits.io_th))         restore = 1; 
        if (last_phase_bw     && (sig->CPI > phases_limits.cpi_busy_waiting)) restore = 1;
        /* WARNING: Below condition won't be accomplished never. */
        if ((sig->Gflops/lib_shared_region->num_cpus) > phases_limits.gflops_busy_waiting)                  restore = 1;

        if (restore)
        {
            verbose_policy_info("Phase has changed, restoring...");

            /* We must reset the configuration */
            policy_new_phase(last_earl_phase_classification, sig);
            last_earl_phase_classification = APP_COMP_BOUND;
            last_gpu_state = _GPU_Comp;
            timestamp_getfast(&init_last_comp);
            REPORT_OPT_ACCURACY(OPT_TRY_AGAIN);
            return EAR_ERROR;
        }
    }

#if USE_GPUS
    if (last_gpu_state & _GPU_Idle) {

        verbose_master(POLICY_GPU, "%sPOLICY%s Validating GPU phase: (#GPUs) %u",
                COL_BLU, COL_CLR, sig->gpu_sig.num_gpus);

        for (int i = 0; i < sig->gpu_sig.num_gpus; i++){
            if (sig->gpu_sig.gpu_data[i].GPU_util > 0){
                last_gpu_state = _GPU_Comp;
                REPORT_OPT_ACCURACY(OPT_TRY_AGAIN);
                return EAR_ERROR;
            }
        }
    }
#endif

    state_t st = EAR_SUCCESS;
    if (polsyms_fun.new_iter != NULL) {
        st = polsyms_fun.new_iter(&my_pol_ctx, sig);
    }
    return st;
}

/** MPI CALLS */

/* This function is executed at the beginning of each mpi call: Warning! it could introduce a lot of overhead*/
static signature_t aux_per_process;
static node_freq_domain_t per_process_dom = {
  .cpu = POL_GRAIN_CORE,
  .mem = POL_NOT_SUPPORTED,
  .gpu = POL_NOT_SUPPORTED,
  .gpumem = POL_NOT_SUPPORTED};



state_t policy_mpi_init(mpi_call call_type)
{
	if (POLICY_MPI_CALL_ENABLED)
	{
		int process_id = NO_PROCESS;

		polctx_t *c = &my_pol_ctx;

		total_mpi++;

		if (polsyms_fun.mpi_init != NULL) {

			if (state_ok(polsyms_fun.mpi_init(c, call_type, &per_process_node_freq, &process_id))) {

				if (process_id != NO_PROCESS) {

					total_mpi_optimized++;

					if (state_fail(policy_freq_selection(c, &aux_per_process,
									&per_process_node_freq,  &per_process_dom, NULL, process_id))) {

						verbose_error("CPU freq. selection failure at MPI call init.");

						return EAR_ERROR;

					}
				}
			} else
			{
				verbose_error("At MPI call init: %s", state_msg);
				return EAR_ERROR;
			}
		} else {
			return EAR_SUCCESS;
		}
	}

	return EAR_SUCCESS;
}


/* This function is executed after each mpi call:
 * Warning! it could introduce a lot of overhead. */
state_t policy_mpi_end(mpi_call call_type)
{
	if (POLICY_MPI_CALL_ENABLED)
	{
		int process_id = NO_PROCESS;

		state_t st = EAR_SUCCESS;

		polctx_t *c = &my_pol_ctx;
		if (polsyms_fun.mpi_end != NULL)
		{
			st = polsyms_fun.mpi_end(c, call_type, &per_process_node_freq, &process_id);
			if (state_fail(st))
			{
				verbose_error("At policy MPI call end: %s", state_msg);
				return st;
			}

			if (process_id != NO_PROCESS)
			{
				st = policy_freq_selection(c, &aux_per_process, &per_process_node_freq, &per_process_dom, NULL, process_id);
				if (state_fail(st))
				{
					verbose_error("CPU freq selection failure at policy_mpi_end.");
					return st;
				}
			}

			return st;

		} else return EAR_SUCCESS;
	} else
	{
		return EAR_SUCCESS;
	}
}


static void select_policy_name(char *ret_pol_name, size_t pol_name_size, char *default_policy_name)
{
	char *monitoring = "monitoring";

	char *policy_name = default_policy_name;

	// Check APIs correctness
	const metrics_t *mgt_cpufreq = metrics_get(MGT_CPUFREQ);
	const metrics_t *met_cpi = metrics_get(MET_CPI);
	const metrics_t *met_bwidth = metrics_get(MET_BWIDTH);

	int cpu_ok = (mgt_cpufreq && mgt_cpufreq->ok);
	int cpi_ok = (met_cpi && met_cpi->ok);
	int bwidth_ok = (met_bwidth && met_bwidth->ok);

	if (!cpu_ok || !cpi_ok || !bwidth_ok)
	{
		policy_name = monitoring;
		verbose_policy_info("Forcing policy: %s", policy_name);
	}

	strncpy(ret_pol_name, policy_name, pol_name_size);
}

/** GLOBAL EVENTS */

/* This function is executed when a reconfiguration needs to be done*/
state_t policy_configure()
{
    polctx_t *c=&my_pol_ctx;
    preturn(polsyms_fun.configure, c);
}


state_t policy_get_current_freq(node_freqs_t *freq_set)
{
    if (freq_set != NULL) node_freqs_copy(freq_set,&nf);
    return EAR_SUCCESS;
}


static state_t fill_mgt_api_pstate_data(uint api_id, pstate_t **pstate_lst, uint *pstate_cnt)
{
    debug("Requesting MGT_CPUFREQ (%u) vs. MGT_IMCFREQ (%u): %u",
            MGT_CPUFREQ, MGT_IMCFREQ, api_id);

    if (pstate_lst && pstate_cnt)
    {
        if (api_id != MGT_CPUFREQ && api_id != MGT_IMCFREQ) {
            return_msg(EAR_ERROR, Generr.api_incompatible);
        }

        // Get mgt API from metrics
        const metrics_t *mgt_api = metrics_get(api_id);

        if (mgt_api == NULL) {
            return_msg(EAR_ERROR, Generr.api_undefined);
        } else if (mgt_api->avail_list != NULL) {
            *pstate_lst = (pstate_t *) mgt_api->avail_list;
            *pstate_cnt = mgt_api->avail_count;

            return EAR_SUCCESS;
        }

        return_msg(EAR_ERROR, Generr.api_initialized);
    } else
    {
        return_msg(EAR_ERROR, Generr.input_null)
    }
}


static state_t from_proc_to_core(ulong *process_cpu_freqs_khz, int process_cnt, ulong **core_cpu_freqs_khz,
        int cpu_cnt, ulong *cpu_freq_max, ulong *cpu_freq_min, int proc_id)
{
    int init_rank, end_rank;
#if 0
    uint cpus_affected = 0;
#endif
    if (proc_id == ALL_PROCESSES){
      init_rank = 0;
      end_rank = process_cnt;
    }else{
      init_rank = proc_id;
      end_rank  = proc_id + 1;
    }
    if (proc_id >= 0){
      verbose(3, "from_proc_to_core  [%d] Freq %.2f", proc_id, (float)process_cpu_freqs_khz[proc_id]/(float)1000000.0);
    }
    if (proc_id == ALL_PROCESSES && (masters_info.my_master_rank < 0)){
        verbose(2, "Not master setting CPUfreq for ALL (%.2f)", (float)process_cpu_freqs_khz[0]/(float)1000000.0);
    }
    if (process_cpu_freqs_khz == NULL || process_cnt < 0 || core_cpu_freqs_khz == NULL
            || *core_cpu_freqs_khz == NULL || cpu_cnt < 0) {
        debug("ERROR: invalid arguments");
        return EAR_ERROR;
    }

    int verb_lvl = POLICY_INFO;

    static uint first_print = 1;
    if (first_print) {
        verb_lvl = 1;
        first_print = 0;
    }

    if (proc_id == ALL_PROCESSES){
      verbose_master(verb_lvl, "Mapping frequencies from %d processes to %d CPUs...", process_cnt, cpu_cnt);
    }

    memset(cpu_must_be_normalized, 0, MAX_CPUS_SUPPORTED * sizeof(uint));

    /* For AMD we compute the max per socket */
    if (arch_desc.top.vendor == VENDOR_AMD) {
        memset(max_freq_per_socket, 0, MAX_SOCKETS_SUPPORTED * sizeof(ulong));
    }
    /* This is a special use case, ignoring the mask. Using freq from proc 0 to all the CPUs */
		if (ignore_affinity_mask){
          // The frequency is filtered by the minimum permitted in the EAR configuration
          ulong cpu_freq_khz = ear_max(process_cpu_freqs_khz[0], (ulong) min_cpufreq);
          debug("Warning: Ignoring the affinity mask. Using %lu in all the cores ", cpu_freq_khz);
			    for (int cpu_idx = 0; cpu_idx < cpu_cnt; cpu_idx++) (*core_cpu_freqs_khz)[cpu_idx] = cpu_freq_khz;
          if (cpu_freq_max != NULL) *cpu_freq_max = cpu_freq_khz;
          if (cpu_freq_min != NULL) *cpu_freq_min = cpu_freq_khz;
			    return EAR_SUCCESS;
		}

    /* This is the default use case */

    for (int proc_lrank = init_rank; proc_lrank < end_rank; proc_lrank++)
    {
#if SHOW_DEBUGS
        if (process_cpu_freqs_khz[proc_lrank] < (ulong) min_cpufreq) {
            debug("The selected frequency (%lu) for process %d is lower than the minimum"
                    " permitted (%lu). Filtering...", process_cpu_freqs_khz[proc_lrank], proc_lrank, (ulong) min_cpufreq);
        }
#endif // SHOW_DEBUGS

        // The frequency is filtered by the minimum permitted in the EAR configuration
        ulong cpu_freq_khz = ear_max(process_cpu_freqs_khz[proc_lrank], (ulong) min_cpufreq);
		process_cpu_freqs_khz[proc_lrank] = cpu_freq_khz;

        /* Compute the maximum and minimum if required */
        if (cpu_freq_max != NULL) {
            *cpu_freq_max = ear_max(cpu_freq_khz, *cpu_freq_max);
        }
        if (cpu_freq_min != NULL) {
            *cpu_freq_min = ear_min(cpu_freq_khz, *cpu_freq_min);
        }

        uint verb = (cpu_freq_khz != init_def_cpufreq_khz); // Verbose only when the frequency to set is
                                                            // different from the initial
        if (verb && (proc_id == ALL_PROCESSES)) {
            verbosen_master(verb_lvl, "Process %d: %lu kHz", proc_lrank, cpu_freq_khz);
            verbosen_master(verb_lvl + 1, " -> CPUs:")
        }


        /* This is the default use case. EAR uses the CPU affinity mask */
        int sem_ret_val = sem_wait(lib_shared_lock_sem);
        if (sem_ret_val < 0)
        {
            verbose_warning("Locking semaphor for cpu mask read failed: %d", errno);
        }

        cpu_set_t process_mask = sig_shared_region[proc_lrank].cpu_mask;

        // We only want to increase the semaphor number if the previous lock was successful.
        if (!sem_ret_val)
        {
            if (sem_post(lib_shared_lock_sem) < 0)
            {
                verbose_warning("Unlocking semaphor for cpu mask read failed: %d", errno);
            }
        }

        for (int cpu_idx = 0; cpu_idx < cpu_cnt; cpu_idx++) {
            if (CPU_ISSET(cpu_idx, &process_mask)) {
                if (verb) {
                    verbosen_master(verb_lvl + 1, " %d", cpu_idx);
                }
                (*core_cpu_freqs_khz)[cpu_idx] = cpu_freq_khz;

                max_freq_per_socket[arch_desc.top.cpus[cpu_idx].socket_id] =
                    ear_max(max_freq_per_socket[arch_desc.top.cpus[cpu_idx].socket_id],
                            cpu_freq_khz);

                cpu_must_be_normalized[cpu_idx] = 1;
#if 0
                cpus_affected++;
#endif
            }
        }
        if (verb) {
            verbosen_master(verb_lvl,"\n");
        }
    }
    if (arch_desc.top.vendor == VENDOR_AMD) {

        for (int cpu_idx = 0; cpu_idx < cpu_cnt; cpu_idx++) {

          // The maximum CPU freq. for the socket which the CPU belongs to.
          ulong max_cpufreq = max_freq_per_socket[arch_desc.top.cpus[cpu_idx].socket_id];

          if (cpu_must_be_normalized[cpu_idx] &&
              (*core_cpu_freqs_khz)[cpu_idx] != max_cpufreq) {

            (*core_cpu_freqs_khz)[cpu_idx] = max_cpufreq;

            verbose_master(verb_lvl,
                "Increasing the CPU %d freq. to %lu because it's on AMD node at socket %d.",
                cpu_idx, max_cpufreq, arch_desc.top.cpus[cpu_idx].socket_id);
          }
        }
    }
#if 0
    if (proc_id >= 0){
        verbose(2,"[%d] Changing CPU freq for %d cpus", proc_id, cpus_affected);
    }
#endif

    return EAR_SUCCESS;
}

static uint must_set_policy_cpufreq(pc_app_info_t *pc_data, settings_conf_t *settings_conf)
{
    debug("must set policy cpufreq (first policy try %u)", first_policy_try);
    if (first_policy_try) {
        debug("Letting the first try to the power policy cpufreq");
        first_policy_try = 0;
        return 1;
    }

    if (pc_data == NULL)
        return 1;
    if ((pc_data->cpu_mode == PC_DVFS)
            && (settings_conf->pc_opt.current_pc <= POWER_CAP_UNLIMITED))
        return 1;
    if (pc_data->cpu_mode == PC_POWER)
        return 1;

    return 0;
}

static ulong compute_avg_freq(ulong *freq_list, int n)
{
    ulong total = 0;

    for (int i = 0; i < n; i++)
    {
        total += freq_list[i];
    }

    return total / n;
}

static void verbose_freq_per_core(int verb_lvl, ulong **core_cpu_freqs_khz, int cpu_cnt)
{
    if (verb_level >= verb_lvl) {

        verbose_master(verb_lvl, "Selected frequency per Processor/CPU:");

        for (int i = 0; i < cpu_cnt; i++)
        {
            if ((*core_cpu_freqs_khz)[i] > 0) {
                verbosen_master(verb_lvl, "CPU[%d]=%.3f ", i, (float) (*core_cpu_freqs_khz)[i] / GHZ_TO_KHZ);
            } else {
                verbosen_master(verb_lvl, "CPU[%d]= --", i);
            }
        }
        verbose_master(verb_lvl, " ");
    }
}

static state_t policy_cpu_freq_selection(polctx_t *c, signature_t *my_sig,
        node_freqs_t *freqs, node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id)
{
    ulong *freq_set = freqs->cpu_freq;

    int i;
    char pol_grain_str[16];

    verbose_master(POLICY_INFO + 1, "%sPOLICY%s CPU freq. selection.", COL_BLU, COL_CLR);

#if DEBUG_CPUFREQ_COST
    timestamp init_cpufreq_cost, end_cpufreq_cost;
    ulong elapsed_cpufreq_cost_us;

    timestamp_getfast(&init_cpufreq_cost);
#endif

    if (dom->cpu == POL_GRAIN_CORE) sprintf(pol_grain_str, "GRAIN_CORE");
    else                            sprintf(pol_grain_str, "GRAIN_NODE");

    verbose_master(3, "%-12s: %s\n%-12s: %d", "Policy level",
            pol_grain_str, "Affinity", ear_affinity_is_set);

    ulong max_cpufreq_sel_khz = 0, min_cpufreq_sel_khz = 1000000000;
    memset(freq_per_core, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);

    // Assumption: If affinity is set for master, it
    // is set for all, we could check individually
    if ((dom->cpu == POL_GRAIN_CORE) && (ear_affinity_is_set)) {
        from_proc_to_core(freq_set, lib_shared_region->num_processes, &freq_per_core,
                arch_desc.top.cpu_count, &max_cpufreq_sel_khz, &min_cpufreq_sel_khz, process_id);
    } else {
        if (process_id == ALL_PROCESSES) {
            debug("Setting same freq in all node %lu", freq_set[0]);

            for (i = 1; i < MAX_CPUS_SUPPORTED; i++) {
                freq_set[i] = freq_set[0];
            }

            if (ear_affinity_is_set) {
                from_proc_to_core(freq_set, lib_shared_region->num_processes, &freq_per_core,
                        arch_desc.top.cpu_count, &max_cpufreq_sel_khz, &min_cpufreq_sel_khz, process_id);
            } else {
                for (i = 0; i < MAX_CPUS_SUPPORTED; i++) {
                    freq_per_core[i] = freq_set[0];
                }
            }
        }
    }

    if  (process_id == ALL_PROCESSES) {
        fill_cpus_out_of_cgroup(exclusive_mode);
    }

    verbose_freq_per_core(POLICY_INFO , &freq_per_core, arch_desc.top.cpu_count);

    // We decided arbitrarely to put show the minimum CPU freq. selected.
    *(c->ear_frequency) = max_cpufreq_sel_khz;

#if SHOW_DEBUGS
    debug("pc_app_info_data %p", pc_app_info_data);
    if (pc_app_info_data != NULL) {
        debug("pc_app_info_data->cpu_mode (%d) == PC_DVFS ? %d",pc_app_info_data->cpu_mode, pc_app_info_data->cpu_mode == PC_DVFS);
    }
    debug("system_conf->pc_opt.current_pc (%d) <= POWER_CAP_UNLIMITED (%d) ? %d", system_conf->pc_opt.current_pc, POWER_CAP_UNLIMITED, system_conf->pc_opt.current_pc <= POWER_CAP_UNLIMITED);
#endif

	// POWER_CAP_UNLIMITED is equal to 1, so the CPU freq is set if powercap is 0 (disabled) or unlimited (1)
	  
    if (must_set_policy_cpufreq(pc_app_info_data, system_conf))
    {
       if ((masters_info.my_master_rank >=0 ) && (process_id == ALL_PROCESSES) )
       {
        verbose_policy_info("Setting a CPU frequency (kHz) range of [%lu, %lu] GHz...",
                            min_cpufreq_sel_khz, max_cpufreq_sel_khz);
       }
			 uint do_change = 1;
			 #if 0
			 if ((process_id != ALL_PROCESSES) && !node_freqs_are_diff(DOM_CPU, freqs, &last_nf)){
					//verbose(0,"not changing freq because they are the same");
					do_change = 0;
				}
       #endif
        

        if (do_change){
          frequency_set_with_list(0, freq_per_core); // Setting the selected CPU frequency
        }
				#if 0
				if ((process_id != ALL_PROCESSES) && do_change) node_freqs_copy(&last_nf, freqs);
				#endif

    } else {
        verbose_policy_info("Requesting a CPU frequency (kHz) range of [%lu, %lu] GHz...",
                            min_cpufreq_sel_khz, max_cpufreq_sel_khz);
    }
    if (process_id == ALL_PROCESSES){
      if ((pc_app_info_data != NULL) && (pc_app_info_data->cpu_mode == PC_DVFS)) {
        /* Warning: POL_GRAIN_CORE not supported */
        pcapp_info_set_req_f(pc_app_info_data, freq_per_core, MAX_CPUS_SUPPORTED);
      }

      int freq_cnt = (dom->cpu == POL_GRAIN_CORE) ? lib_shared_region->num_processes : 1;
      if (avg_freq != NULL){
				avg_freq->cpu_freq[0] = compute_avg_freq(freq_set, freq_cnt);
			}

#if DEBUG_CPUFREQ_COST
      /* For debugging purposes */
      timestamp_getfast(&end_cpufreq_cost);
      elapsed_cpufreq_cost_us = timestamp_diff(&end_cpufreq_cost, &init_cpufreq_cost, TIME_USECS);
      total_cpufreq_cost_us += elapsed_cpufreq_cost_us;
      calls_cpufreq++;
#endif
    }

    return EAR_SUCCESS;
}

static state_t policy_mem_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    if ((freqs == NULL) || (freqs->imc_freq == NULL))
    {
        return_msg(EAR_ERROR, Generr.input_null); // Warning: The caller does not check the returned state value.

        // verbose_master(POLICY_INFO, "%sERROR%s policy_mem_freq_selection NULL", COL_RED, COL_CLR);
        // return EAR_SUCCESS;
    }
    verbose_policy_info("(Lower/Upper) requested IMC pstate (%lu/%lu)",
                        freqs->imc_freq[IMC_MAX],freqs->imc_freq[IMC_MIN]);

    uint *imc_up, *imc_low;

    if (dyn_unc && metrics_get(MGT_IMCFREQ)->ok) {
        imc_up = calloc(imc_devices, sizeof(uint));
        imc_low = calloc(imc_devices, sizeof(uint));

        /* We are using the same imc pstates for all the devices for now */
#if SHOW_DEBUGS
            // Below code assumes the same IMC freq. was selected for all devices!
            if (max_policy_imcfreq_ps < freqs->imc_freq[0 * IMC_VAL + IMC_MAX]) {
                debug("The selected upper IMC P-State (%lu) is greather than the maximum"
                        " permitted (%u). Fitering...",
                        freqs->imc_freq[0 * IMC_VAL + IMC_MAX], max_policy_imcfreq_ps);
            }
#endif
        for (int sid = 0; sid < imc_devices; sid++) {
            imc_low[sid] = ear_min((uint) freqs->imc_freq[sid * IMC_VAL + IMC_MAX], max_policy_imcfreq_ps);
            imc_up[sid]  = freqs->imc_freq[sid * IMC_VAL + IMC_MIN];	

            // Check whether selected IMC upper bound is correct
            if (imc_up[sid] < imc_min_pstate[sid]) { 
                verbose_error_master("IMC upper bound is greather than max (%u)", imc_up[sid]);
                return EAR_SUCCESS;
            }

            // Check whether selected IMC lower bound is correct
            if (imc_low[sid] > imc_max_pstate[sid]) {
                verbose_error_master("IMC lower bound is lower than minimum (%u)", imc_low[sid]);
                return EAR_SUCCESS;
            }

            if (imc_up[sid] != ps_nothing) {
                verbose_policy_info("Setting IMC freq (Upper/Lower) [SID=%d] to (%llu/%llu)",
                                    sid, imc_pstates[imc_low[sid]].khz, imc_pstates[imc_up[sid]].khz);

            } else {
                verbose_policy_info("Setting IMC freq (Upper) [SID=%d] (%llu) and min",
                                    sid, imc_pstates[imc_low[sid]].khz);
            }
        }

        if (state_fail(mgt_imcfreq_set_current_ranged_list(NULL, imc_low, imc_up)))
        {
            uint sid = 0;
            if (imc_up[sid] != ps_nothing)
            {
                verbose_error_master("Error setting IMC freq to %llu and %llu",
                                     imc_pstates[imc_low[0]].khz, imc_pstates[imc_up[0]].khz);
            } else
            {
                verbose_error_master("Error setting IMC freq to %llu and min",
                                     imc_pstates[imc_low[0]].khz);
            }
        }

        free(imc_up);
        free(imc_low);
    } else {
        verbose_master(2, "IMC not compatible");
    }

    return EAR_SUCCESS;
}


static state_t policy_gpu_mem_freq_selection(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs,node_freq_domain_t *dom,node_freqs_t *avg_freq)
{
    return EAR_SUCCESS;
}


static state_t policy_gpu_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq)
{
    ulong *gpu_f = freqs->gpu_freq;
    int i;
    if (c->num_gpus == 0) return EAR_SUCCESS;

#if USE_GPUS
    if (verb_level >= POLICY_GPU) {
        for (i=0; i < c->num_gpus; i++) {
            verbosen_master(POLICY_GPU," GPU freq[%d] = %.3f ",
                    i, (float) gpu_f[i] / GHZ_TO_KHZ);
        }
        verbose_master(POLICY_GPU," ");
    }

    // TODO: Powercap limits are checked here but this code could be moved to a function.
    pc_app_info_data->num_gpus_used = my_sig->gpu_sig.num_gpus;
    memcpy(pc_app_info_data->req_gpu_f, gpu_f, c->num_gpus * sizeof(ulong));

    // TODO We must filter the GPUs I'm using.
   if (state_fail(mgt_gpu_freq_limit_set(no_ctx, gpu_f))) {
        verbose_error_master("Setting GPU frequency (%d: %s)", intern_error_num, intern_error_str);
    }
		if (avg_freq != NULL)
        {
    	avg_freq->gpu_freq[0] = compute_avg_freq(gpu_f, my_sig->gpu_sig.num_gpus);
		}
#endif
    return EAR_SUCCESS;
}


static state_t policy_master_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id)
{
  if (masters_info.my_master_rank < 0) return EAR_SUCCESS;
  return policy_freq_selection(c, my_sig, freqs, dom, avg_freq, process_id);
}


static state_t policy_freq_selection(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,
        node_freq_domain_t *dom, node_freqs_t *avg_freq, int process_id)
{
#if SINGLE_CONNECTION
    if (masters_info.my_master_rank < 0 ) {
        return EAR_SUCCESS;
    }
#endif

    if (dom->cpu != POL_NOT_SUPPORTED) {
        policy_cpu_freq_selection(c, my_sig, freqs, dom, avg_freq, process_id);
    }
    if (process_id == ALL_PROCESSES){
        if (dom->mem != POL_NOT_SUPPORTED) {
            policy_mem_freq_selection(c, my_sig, freqs, dom, avg_freq);
        }
        if (dom->gpu != POL_NOT_SUPPORTED) {
            policy_gpu_freq_selection(c, my_sig, freqs, dom, avg_freq);
        }
        if (dom->gpumem != POL_NOT_SUPPORTED) {
            policy_gpu_mem_freq_selection(c, my_sig, freqs, dom, avg_freq);
        }
    }

    return EAR_SUCCESS;
}


static void verbose_time_in_phases(signature_t *sig)
{
  sig_ext_t *se = (sig_ext_t *)sig->sig_ext;
  if (se != NULL){
    verbose_master(2, "............Phases");
    for (uint ph = 0; ph < EARL_MAX_PHASES; ph++){
      if (se->earl_phase[ph].elapsed){ 
        verbose_master(2, "Phase[%s] elapsed %llu", phase_to_str(ph), se->earl_phase[ph].elapsed);
      }
    }
    verbose_master(2, "............END-Phases");
  }
}


static state_t policy_set_cpus_governor(uint exclusive)
{
#define free_return_err(msg) \
        free(governor_list_before_init);\
        governor_list_before_init = NULL;\
        return_msg(EAR_ERROR, msg);

#define check(st, msg) \
    if (state_fail(st)) {free_return_err(msg);}

    if (mgt_cpufreq_governor_is_available(no_ctx, Governor.userspace))
    {
        // Allocate the governor list
        mgt_cpufreq_data_alloc(NULL, &governor_list_before_init);

        // Retrieve the current governor list
        check(mgt_cpufreq_governor_get_list(no_ctx, governor_list_before_init),
              "Retrieving the governor list.");

        // Verbose governor list
        if (VERB_ON(POLICY_INFO))
        {
            verbosen(POLICY_INFO , "List of governors of %u CPUs: ", lib_shared_region->num_cpus);
            for (int i = 0; i < metrics_get(MGT_CPUFREQ)->devs_count; i++)
            {
                verbosen(POLICY_INFO , " %u", governor_list_before_init[i]);;
            }
            verbose(POLICY_INFO , " ");
        }

        // Set userspace governor on CPUs not used by the job

        cpu_set_t cpus_to_be_set;
        if (exclusive){
          cpumask_all_cpus(&cpus_to_be_set);
        }else{
          memcpy(&cpus_to_be_set, &lib_shared_region->node_mask, sizeof(cpu_set_t));
        }

        verbosen(POLICY_INFO , "Node used CPUs: ");
        verbose_affinity_mask(POLICY_INFO , (const cpu_set_t *) &lib_shared_region->node_mask, MAX_CPUS_SUPPORTED);

        verbosen(POLICY_INFO , "CPUs to be set: ");
        verbose_affinity_mask(POLICY_INFO , (const cpu_set_t *) &cpus_to_be_set, MAX_CPUS_SUPPORTED);

        // Getting a valid ID for 'userspace' governor
        char *governor_str = "userspace";
        uint governor_int;
        check(governor_toint(governor_str, &governor_int),
              "Converting governor from str to int.");

        // Setting the governor
#if VERSION_MAJOR > 4 || (VERSION_MAJOR == 4 && VERSION_MINOR >= 3)
// #warning mgt_cpufreq_governor_set_mask compiled
        check(mgt_cpufreq_governor_set_mask(no_ctx, governor_int, cpus_to_be_set),
              "Setting governor.");
#endif
    }

    return EAR_SUCCESS;
}


static state_t policy_restore_cpus_governor()
{
    if (governor_list_before_init)
    {
        mgt_cpufreq_governor_set_list(no_ctx, governor_list_before_init);

        free(governor_list_before_init);
        governor_list_before_init = NULL;
    }

    return EAR_SUCCESS;
}


void compute_avg_sel_freq(ulong * avg_sel_mem_f, ulong *imcfreqs)
{
	if (!avg_sel_mem_f || !imcfreqs || arch_desc.top.socket_count <= 0)
	{
		return;
	}

	ulong avg = 0, low, f;
	for (uint sock_id = 0; sock_id < arch_desc.top.socket_count; sock_id++)
	{
		low = imcfreqs[sock_id * IMC_VAL + IMC_MAX];
		f = imc_pstates[low].khz;

		verbose_master(2,"Computing avg mem freq : Socket %u max freq %lu", sock_id, f);

		avg += f;
	}

	avg = avg / arch_desc.top.socket_count;
	*avg_sel_mem_f = avg;
}


#if MPI_OPTIMIZED
state_t policy_restore_to_mpi_pstate(uint index)
{
  polctx_t *c = &my_pol_ctx;
  ulong f = sig_shared_region[my_node_id].mpi_freq;

  if (index != process_get_target_pstate())
	{
    pstate_t *sel_pstate, *pstate_list;
    pstate_list = metrics_get(MGT_CPUFREQ)->avail_list;
    sel_pstate = &pstate_list[index];
    f = sel_pstate->khz;
  }

  if (traces_are_on())
	{
    traces_generic_event(ear_my_rank, my_node_id , TRA_CPUF_RAMP_UP, f);
  }

  verbose(2, "restore CPU freq in process %d to %lu", my_node_id, f);

	state_t st = EAR_ERROR;

	if (POLICY_MPI_CALL_ENABLED)
	{
		per_process_node_freq.cpu_freq[my_node_id] = f;

		st = policy_freq_selection(c, &aux_per_process, &per_process_node_freq,  &per_process_dom, NULL, my_node_id);
	}

  if (state_fail(st))
	{
      verbose_error("CPU freq. restore in MPI opt.");
      return EAR_ERROR;
  }
  return EAR_SUCCESS;
}
#endif


static void fill_cpus_out_of_cgroup(uint exc)
{
  if (exc == 0) return;

  verbosen_master(VEARL_DEBUG, "IDLE freq (%lu) cpus --> ",
                  frequency_pstate_to_freq(frequency_get_num_pstates()-1));

  int cpu_cnt = arch_desc.top.cpu_count;
  for (int cpu_idx = 0; cpu_idx < cpu_cnt; cpu_idx++)
  {
    if (freq_per_core[cpu_idx] == 0)
    {
      freq_per_core[cpu_idx] = frequency_pstate_to_freq(frequency_get_num_pstates()-1);
      verbosen_master(VEARL_DEBUG, "%d ", cpu_idx);
    }
  }
  verbosen_master(VEARL_DEBUG, "\n");
}


static void policy_new_phase(uint phase, signature_t *sig)
{
  verbose_master(3, "policy_new_phase summary computation %s (%u/%u)",
                 phase_to_str(phase), policy_cpu_bound, policy_mem_bound);

  sig_ext_t *se = (sig_ext_t *) sig->sig_ext;
  if (se != NULL)
  {
    for (uint ph = 0; ph < EARL_MAX_PHASES; ph++) se->earl_phase[ph].elapsed = 0;

    timestamp currt;
    timestamp_getfast(&currt);
    se->earl_phase[phase].elapsed = timestamp_diff(&currt, &init_last_phase, TIME_USECS);

    /* For COMP_BOUND we compute if CPU or MEM (or mix) */
    if (phase == APP_COMP_BOUND)
    {
      ullong elapsed_comp = timestamp_diff(&currt, &init_last_comp, TIME_USECS);

      /* policy_cpu_bound and policy_mem_bound are updated only in the APP_COMP_PHASE */
      verbose_master(3, "COMP_PHASE: cpu %u mem %u mix %u", policy_cpu_bound, policy_mem_bound , !policy_cpu_bound && !policy_mem_bound);

      if (policy_cpu_bound)      se->earl_phase[APP_COMP_CPU].elapsed = elapsed_comp;
      else if (policy_mem_bound) se->earl_phase[APP_COMP_MEM].elapsed = elapsed_comp;
      else                            se->earl_phase[APP_COMP_MIX].elapsed = elapsed_comp;  
    }
    memcpy(&init_last_phase, &currt, sizeof(timestamp));
    verbose_time_in_phases(sig);
  }
}


static void build_plugin_path(char *policy_plugin_path, size_t plug_path_size, settings_conf_t *app_settings)
{
	int read_env = (app_settings->user_type == AUTHORIZED || USER_TEST);
	// Hack env var overwrites the entire path
	char *hack_so	= (read_env) ? ear_getenv(HACK_POWER_POLICY) : NULL;

	// No hack set
	if (!hack_so)
	{
		// Policy file selection
		char my_policy_name[SZ_FILENAME];
		select_policy_name(my_policy_name, sizeof(my_policy_name), app_settings->policy_name);

		// Plug-in path selection

		char *hack_path = ear_getenv(HACK_EARL_INSTALL_PATH);

		char plugin_path[MAX_PATH_SIZE];
		memset(plugin_path, 0, sizeof(plugin_path));

		conf_install_t *install_data = &app_settings->installation;
		utils_create_plugin_path(plugin_path, install_data->dir_plug, hack_path, app_settings->user_type);

		// App mgr
		char *app_mgr_policy = ear_getenv(FLAG_APP_MGR_POLICIES);

		int app_mgr = 0;
		if (app_mgr_policy != NULL)
		{
			app_mgr = atoi(app_mgr_policy);
		}

		// Final path build

		if (!app_mgr)
		{
			snprintf(policy_plugin_path, plug_path_size,
							 "%s/policies/%s.so", plugin_path, my_policy_name);
		} else
		{
			snprintf(policy_plugin_path, plug_path_size,
							 "%s/policies/app_%s.so", plugin_path, my_policy_name);
		}
	} else
	{
		// Hack env var overwrites the entire path
		snprintf(policy_plugin_path, plug_path_size, "%s", hack_so);
	}
}
