/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_CONFIG_ENV_H
#define COMMON_CONFIG_ENV_H

#include <common/environment_common.h>
#include <common/config/config_install.h>
// Because in fact all environment variables depends on ear_getenv().

// These variables are SLURM named variables. In case we want to use it from
// different queue managers, we can:
//
// 1) Replace a manager exclusive variable by our SLURM named variable inside
//    the specific plugin for PBS, OAR... In example:
//          SLURM_STEP_NUM_NODES --- to ---> EAR_NUM_NOES
//
// 2) Surround these variable definitions by the different #ifs and set the name
//    depending on the queue manager are you compiling for. This strategy limits
//    an EAR component compatibility to a specific queue manager. But in this
//    case slurm_sched.h would be better place to keep them.
//
// Commented variables are unused and can be removed.
#define NULL_STEPID             (0xfffffffe)
#define NULL_ACCOUNT            "NO_SLURM_ACCOUNT"
#define FLAG_TASK_PID           "TASK_PID"
#define SCHED_JOB_ID            "JOB_ID"
#define SCHED_STEP_ID           "STEP_ID"
#define SCHED_STEPID            "STEPID"                 // Deprecated by SLURM, maintained for backwards compatibility.
#define SCHED_JOB_ACCOUNT       "JOB_ACCOUNT"
#define SCHED_NUM_TASKS         "STEP_TASKS_PER_NODE"
#define SCHED_STEP_NUM_NODES    "STEP_NUM_NODES"
#define SCHED_IS_ERUN           "IS_ERUN"

/**
 * @defgroup PLUGIN_VARIABLES Plugin Variables
 * These variables are used by the plugin to configure the EAR environment.
 * @{
 */
#define ENV_PATH_TMP            "EAR_TMP"
#define ENV_PATH_ETC            "EAR_ETC"
#define ENV_PATH_EAR            "EAR_INSTALL_PATH"
#define ENV_APP_NAME            "EAR_APP_NAME"
#define ENV_APP_NUM_NODES       "EAR_NUM_NODES"
#define ENV_FLAG_PATH_USERDB    "EAR_USER_DB_PATHNAME"
#define ENV_FLAG_PATH_COEFFS    "EAR_COEFF_DB_PATHNAME"
#define ENV_FLAG_PATH_TRACE     "EAR_TRACE_PATH"
#define ENV_FLAG_IS_LEARNING    "EAR_LEARNING_PHASE"
#define ENV_FLAG_POLICY         "EAR_POWER_POLICY"
#define ENV_FLAG_POLICY_PENALTY "EAR_PERFORMANCE_PENALTY"
#define ENV_FLAG_POLICY_GAIN    "EAR_MIN_PERFORMANCE_EFFICIENCY_GAIN"
#define ENV_FLAG_POLICY_TH      "EAR_POWER_POLICY_TH"
#define ENV_FLAG_PSTATE         "EAR_P_STATE"
#define ENV_FLAG_FREQUENCY      "EAR_FREQUENCY"
#define ENV_FLAG_VERBOSITY      "EAR_VERBOSE"
#define ENV_FLAG_ENERGY_TAG     "EAR_ENERGY_TAG"

// Not controlled:
//   EAR_PERFORMANCE_ACCURACY
//   EAR_MIN_P_STATE
//   EAR_DYNAIS_LEVELS
//   EAR_DYNAIS_WINDOW_SIZE
//   EAR_POWERCAP_POLICY_NODE
//   EAR_AFFINITY
//   EAR_MAX_DYNAIS_OVERHEAD
/**@}*/

/**
 * @defgroup HACK Hack variables.
 * They modify the default configuration.
 * Some variables are deprecated but still supported for compatibility. See at the end.
 * @{
 */
#define HACK_LIBRARY_FILE            "HACK_LIBRARY_FILE"       // Sets a specific library file.
#define HACK_LOADER_FILE             "HACK_LOADER_FILE"        // Sets a specific loader file.
#define HACK_EAR_TMP                 "HACK_EAR_TMP"            // Re-defines EAR_TMP folder
#define HACK_EAR_ETC                 "HACK_EAR_ETC"            // Re-defines EAR_ETC folder
#define HACK_NVML_FILE               "HACK_NVML_FILE"          // The GPU API loads a specific NVML library file.
#define HACK_MPI_VERSION             "HACK_MPI_VERSION"        // Adds a suffix to libear.so (i.e: libear.hello.so).
#define HACK_ENERGY_PLUGIN           "HACK_ENERGY_PLUGIN"       // This var forces to load a specific energy plugin.
#define HACK_POWER_POLICY            "HACK_POWER_POLICY"       // This var forces to load a specific power policy. Replaces (SCHED_PREFIX)_EAR_POWER_POLICY.
#define HACK_GPU_POWER_POLICY        "HACK_GPU_POWER_POLICY"   // This var forces to load a specific gpu power policy. Replaces (SCHED_PREFIX)_EAR_GPU_POWER_POLICY.
#define HACK_GPU_POLICY_PENALTY      "HACK_GPU_POLICY_PENALTY" // Sets the threshold penalty tolered by Min-GPU-Ebnergy policy.. Replaces (SCHED_PREFIX)_EAR_GPU_POLICY_PENALTY.
#define HACK_POWER_MODEL             "HACK_POWER_MODEL"        // This var forces to load a specific power model plugin. Replaces (SCHED_PREFIX)_EAR_POWER_MODEL.
#define HACK_GPU_ENERGY_MODEL        "HACK_GPU_ENERGY_MODEL"   // This var forces to load a specific GPU energy model plugin. Replaces (SCHED_PREFIX)_EAR_GPU_POWER_MODEL.
#define HACK_CPU_POWER_MODEL         "HACK_CPU_POWER_MODEL"    // This var forces to load a specific cpu power model plugin.
#define HACK_EARL_COEFF_FILE         "HACK_EARL_COEFF_FILE"    // This var forces to load a specific coefficient file
#define HACK_EARL_GPU_COEFF_FILE     "HACK_EARL_GPU_COEFF_FILE"    // This var forces to load a specific coefficient file for GPU models
#define HACK_DYNAIS_WINDOW_SIZE      "HACK_DYNAIS_WINDOW_SIZE" // This var forces a specific dynais windows size. Replaces (SCHED_PREFIX)_EAR_DYNAIS_WINDOW_SIZE.
#define HACK_DEF_FREQ                "HACK_DEF_FREQ"           // This var forces a different default frequency than the predefined for a policy. Replaces (SCHED_PREFIX)_EAR_DEF_FREQ.
#define HACK_EARL_VERBOSE            "HACK_EARL_VERBOSE"       // This var forces an EARL verbosity.
#define HACK_EARL_INSTALL_PATH       "HACK_EARL_INSTALL_PATH"  // This variable HACKS all the library environment, simplifies the utilization of a privatized environment.
#define HACK_PROCS_VERB_FILES        "HACK_PROCS_VERB_FILES"   // This variable makes all processes having a log verbose file each one.
/** @} */

/**
 * @defgroup OTHER Other
 * These variables are not HACKS and there is not other way to specify in EAR.
 * @{
 */
#define FLAG_EAR_DEBUG							 "EAR_DEBUG"
#define FLAG_TRACE_PLUGIN            "EAR_TRACE_PLUGIN"       // Sets the trace plugin in EARL.
#define FLAG_GPU_DEF_FREQ            "EAR_GPU_DEF_FREQ"       // This var sets a default GPU frequency.
#define FLAG_GPU_DCGMI_ENABLED       "EAR_GPU_DCGMI_ENABLED"  // if DCGMI is enable at compile time, this flag allows to explicitly disable it: Default 1. (Use 1/0)
#define FLAG_DCGMI_SAMPLING_PERIOD	 "EAR_DCGMI_SAMPLING_PERIOD" // Set the period of the DCGMI metrics sampling monitor. Be careful, as very low values can introduce considerable overhead. This variable is read as a single precision floating point number.
#define FLAG_DCGM_ALL_EVENTS 				 "EAR_DCGM_ALL_EVENTS" // Request to collect and report all DCGM events.

#define FLAG_LOADER_APPLICATION      "EAR_LOADER_APPLICATION" // This variable forces to load the NON MPI version of the library for a given application (i.e. SLURM_LOADER_LOAD_NO_MPI_LIB=gromacs).
#define FLAG_LOAD_MPI_VERSION        "EAR_LOAD_MPI_VERSION"   // This variable is used for not detected MPI application (python) to specify which version it is: intel, ompi, etc
#define FLAG_MAX_IMCFREQ             "EAR_MAX_IMCFREQ"        // Sets the maximum IMC/DF frequency at which the application must run.
#define FLAG_MIN_IMCFREQ             "EAR_MIN_IMCFREQ"        // Sets the minimum IMC/DF frequency at which the application must run.
#define FLAG_SET_IMCFREQ             "EAR_SET_IMCFREQ"        // This variable specifies the IMC freq must be selected by the EAR policy.
#define FLAG_IMC_TH                  "EAR_POLICY_IMC_TH"      // Sets the threshold penalty tolered by the IMC/DF policy.
#define FLAG_LET_HW_IMC              "EAR_LET_HW_IMC"         // Tells the IMC policy to be first guided by hardware's UFS algorithm.

#define FLAG_EXCLUSIVE_MODE          "EAR_JOB_EXCLUSIVE_MODE" // Tells EAR the current job is the unique executing in the node.
#define FLAG_EARL_PHASES             "USE_EARL_PHASES"        // Allows EARL to detect application phases (e.g., compute bound, memory bound, mpi bound) and improve policy default decisions.
#define FLAG_LOAD_BALANCE  	         "EAR_LOAD_BALANCE"       // Force changing policies' default decisions if they detect the application is load unbalanced.
#define FLAG_LOAD_BALANCE_TH         "EAR_LOAD_BALANCE_TH"    // Sets a threshold to considere the application is load unbalanced.
#define FLAG_TURBO_CP                "USE_TURBO_FOR_CP"
#define FLAG_MIN_CPUFREQ             "EAR_MIN_CPUFREQ"        // Sets a minimum CPU frequency policies can set.
#define FLAG_NTWRK_IMC               "EAR_NTWRK_IMC"          // Tells the IMC policy the application uses IMC freq for network operations, making the policy less aggressive.
#define FLAG_DISABLE_NODE_METRICS    "EAR_DISABLE_NODE_METRICS" // If defined, This job will not be considered for node metrics and policy will not be applied : To be used in MASTER/WORKED use cases
#define FLAG_NTASK_WORK_SHARING      "EAR_NTASK_WORK_SHARING"   // Tells EAR to consider the N tasks created by the scheduler as a single job (for non MPI use cases).
#define FLAG_TRY_TURBO               "EAR_TRY_TURBO"          // If policies have make CPU frequency be at maximum, try to boost the CPU frequency.
#define FLAG_REPORT_ADD              "EAR_REPORT_ADD"
#define FLAG_USE_ENERGY_MODELS       "EAR_USE_ENERGY_MODELS"  // Tells policies to use power models instead of probe and error DVFS.
#define FLAG_EARL_VERBOSE_PATH       "EARL_VERBOSE_PATH"      // Asks EARL to write verbose messages in separate per-task files.
#define FLAG_LOADER_VERBOSE          "EAR_LOADER_VERBOSE"     // Loader's verbosity.
#define FLAG_MPI_OPT                 "EAR_MPI_OPTIMIZATION"   // Enables/Disables MPI optimization when supported by policies
#define FLAG_MIN_USEC_MPI            "EAR_MIN_USEC_MPI_OPT"   // Minimum time to apply MPI optimization
#define FLAG_MPI_SAMPLING_ENABLED    "EAR_MPI_SAMPLING_ENABLED"  // Allows MPI sampling monitoring to be enabled(1) or disabled (0). Default is 1.

#define FLAG_NO_AFFINITY_MASK        "EARL_NO_AFFINITY_MASK"   // Prevents EARL from using the affinity mask. Only for special use cases
#define FLAG_REPORT_LOOPS            "EARL_REPORT_LOOPS"       // Reports or not report EAR library loops
#define FLAG_EAR_DEBUG               "EAR_DEBUG"

//#define FLAG_USE_PRIO                "EAR_USE_PRIO"           // Enables CPU priority system.
#define FLAG_PRIO_CPUS               "EAR_PRIO_CPUS"          // A list of priorities for each CPU.
#define FLAG_PRIO_TASKS              "EAR_PRIO_TASKS"         // A list of priorities for each task.

/* Hardware info */
#define FLAG_CPU_TDP		             "EAR_CPU_TDP"            // CPU TPD for models not yet supported
#define FLAG_IO_FAKE_PHASE           "EARL_IO_FAKE_PHASE"
#define FLAG_BW_FAKE_PHASE           "EARL_BW_FAKE_PHASE"
#define FLAG_FORCE_SHARED_NODE       "EARL_FORCE_SHARED_NODE"
#define FLAG_DISABLE_CNTD            "EAR_DISABLE_CNTD"       // disables automatic CNTD loadin
#define FLAG_USE_DLB                 "EAR_USE_DLB_TALP"

#define FLAG_PYTHON_MULTIPROC "EARL_PYTHON_MULTIPROC" // Enables features supporting applications which use Python's multiprocessing module. Currently, this variable has the same effect as FLAG_NO_SIGHANDLER
#define FLAG_NO_SIGHANDLER 				"EARL_NO_SIGHANDLER" 		// Configures EARL to not configure any signal handler. This is useful to avoid conflicts with applications which want to handle signals.

/** @} */

/**
 * @defgroup DEVELOPMENT Under development
 * These variables controls the behaviour for new infrastructure.
 * They affect features under development.
 * @{
 */
/** @} */

/**
 * @defgroup VERBOSE Verbose effects
 * Prints all the signatures in state.c, and print per node or all shared signatures.
 * @{
 */
#define FLAG_SHOW_SIGNATURES         "EAR_SHOW_SIGNATURES"
#define FLAG_REPORT_NODE_SIGNATURES  "EAR_REPORT_NODE_SIGNATURES"
#define FLAG_REPORT_ALL_SIGNATURES   "EAR_REPORT_ALL_SIGNATURES"
#define FLAG_REPORT_CPUPOW_MODEL     "EAR_REPORT_CPUPOW_MODEL"    // Asks the cpu power model to print per process data
#define FLAG_SHARE_INFO_PPROC        "EAR_SHARE_INFO_PER_PROCESS" // Forces to share all the signatures.
#define FLAG_SHARE_INFO_PNODE 	     "EAR_SHARE_INFO_PER_NODE"    // Forces to share only the average signature per node.
#define FLAG_APP_MGR_POLICIES	     "EAR_APP_MGR_POLICIES"
#define FLAG_GET_MPI_STATS           "EAR_GET_MPI_STATS"
#define FLAG_REPORT_EARL_EVENTS      "REPORT_EARL_EVENTS" // Generates extra events during the execution of the EARL
//#define FLAG_REPORT_EARL_LOOPS       "REPORT_EARL_LOOPS"
/** @} */

/**
 * @defgroup DEPRECATED Deprecated variables
 * Users will be warned and these variables will be removed in future versions.
 * @{
 */
#define SCHED_LOADER_LOAD_NO_MPI_LIB "SLURM_LOADER_APPLICATION"            // TODO: Deprectated. This variable is maintaned here in order to avoid errors in users' scripts.
#define SCHED_LOAD_MPI_VERSION       "SLURM_LOAD_MPI_VERSION"              // TODO: Deprectated. This variable is maintaned here in order to avoid errors in users' scripts.
#define SCHED_LET_HW_CTRL_IMC        "SLURM_LET_HW_CTRL_IMC"               // TODO: Deprectated.
#define SCHED_ENABLE_LOAD_BALANCE  	 "SLURM_ENABLE_LOAD_BALANCE"           // TODO: Deprectated.
#define SCHED_NETWORK_USE_IMC        "SLURM_NETWORK_USE_IMC"               // TODO: Deprectated.
#define SCHED_TRY_TURBO              "SLURM_TRY_TURBO"                     // TODO: Deprectated.
#define SCHED_LOAD_BALANCE_TH        "SLURM_LOAD_BALANCE_TH"               // TODO: Deprectated.
/** @} */


/*
 * EARD flags
 */

#define EARD_USE_MINCPUFREQ_PC       "EARD_USE_MINCPUFREQ_PC"							// EARD will apply min CPU freq when idle periods
#define EAR_POWERCAP_POLICY_NODE     "EAR_POWERCAP_POLICY_NODE"           // Defines the EARD powercapo plugin for domain node
#define EAR_POWERCAP_POLICY_CPU      "EAR_POWERCAP_POLICY_CPU"            // Defines the EARD powercapo plugin for domain CPU
#define EAR_POWERCAP_POLICY_DRAM     "EAR_POWERCAP_POLICY_DRAM"           // Defines the EARD powercapo plugin for domain DRAM
#define EAR_POWERCAP_POLICY_GPU      "EAR_POWERCAP_POLICY_GPU"            // Defines the EARD powercapo plugin for domain GPU

#endif //COMMON_CONFIG_ENV_H
