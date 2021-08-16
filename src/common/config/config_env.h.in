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

#ifndef CONFIG_ENVIRONMENT_H
#define CONFIG_ENVIRONMENT_H

/* These variables are used by the plugin to configure the EAR environment */
#define VAR_INS_PATH "EAR_INSTALL_PATH"
#define VAR_TMP_PATH "EAR_TMP"
#define VAR_ETC_PATH "EAR_ETC"
#define VAR_APP_NAME "EAR_APP_NAME"
#define VAR_OPT_VERB "EAR_VERBOSE"
#define VAR_OPT_POLI "EAR_POWER_POLICY"
#define VAR_OPT_FREQ "EAR_FREQUENCY"
#define VAR_OPT_PSTA "EAR_P_STATE"
#define VAR_OPT_LERN "EAR_LEARNING_PHASE"
#define VAR_OPT_ETAG "EAR_ENERGY_TAG"
#define VAR_OPT_USDB "EAR_USER_DB_PATHNAME"
#define VAR_OPT_THRA "EAR_POWER_POLICY_TH"
#define VAR_OPT_THRB "EAR_PERFORMANCE_PENALTY"
#define VAR_OPT_THRC "EAR_MIN_PERFORMANCE_EFFICIENCY_GAIN"

/* EAR paths and names */
#define REL_PATH_LIBR "lib"
#define REL_NAME_LIBR "libear"
#define REL_PATH_LOAD "lib/libearld.so"

/* Scheduler definitions */
/* SLURM removes some env vars. Variables starting with SLURM are not removed */
#define SCHED_SLURM 1
#ifdef  SCHED_SLURM

#define VAR_OPT_TRAC "SLURM_EAR_TRACE_PATH"

/* HACK variables, they modify the default configuration */
// Sets a specific library path (the loader selects the file).
#define HACK_PATH_LIBR "SLURM_HACK_LIBRARY"
// Sets a specific library file.
#define HACK_FILE_LIBR "SLURM_HACK_LIBRARY_FILE"
// Sets a specific loader file.
#define HACK_FILE_LOAD "SLURM_HACK_LOADER"
// The GPU API loads a specific NVML library file.
#define HACK_FILE_NVML "SLURM_HACK_NVML"
// Adds a suffix to libear.so (i.e: libear.hello.so). 
#define FLAG_NAME_LIBR "SLURM_HACK_EAR_MPI_VERSION"
/* This var forces to load a specific power policy. Replaces SLURM_EAR_POWER_POLICY */
#define SCHED_EAR_POWER_POLICY "SLURM_HACK_EAR_POWER_POLICY"
/* This var forces to load a specific gpu power policy. Replaces SLURM_EAR_GPU_POWER_POLICY */
#define SCHED_EAR_GPU_POWER_POLICY "SLURM_HACK_EAR_GPU_POWER_POLICY"
/* This var forces to load a specific power model plugin.  Replaces SLURM_EAR_POWER_MODEL */
#define SCHED_EAR_POWER_MODEL "SLURM_HACK_EAR_POWER_MODEL"
/* This var forces a specific dynais windows size. Replaces SLURM_EAR_DYNAIS_WINDOW_SIZE */
#define SCHED_EAR_DYNAIS_WINDOW_SIZE "SLURM_HACK_EAR_DYNAIS_WINDOW_SIZE"
/* This var forces a different default frequency than the predefined for a policy. replaces SLURM_EAR_DEF_FREQ */
#define SCHED_EAR_DEF_FREQ "SLURM_HACK_EAR_DEF_FREQ"
/* This var forces an EARL verbosity */
#define SCHED_EARL_VERBOSE "SLURM_HACK_EARL_VERBOSE"
/* This variables HACKS all the library environment, simplifies the utilization of a privatized environment */
#define SCHED_EARL_INSTALL_PATH "SLURM_HACK_EARL_INSTALL_PATH"

/* END HACK section */


/* These variables are not HACKS given there is no other way to specify in EAR */
/* Sets the trace plugin in EARL */
#define SCHED_EAR_TRACE_PLUGIN "SLURM_EAR_TRACE_PLUGIN"
/* This var sets a default GPU ferquency. */
#define SCHED_EAR_GPU_DEF_FREQ "SLURM_EAR_GPU_DEF_FREQ"
/* This variable forces to load the NON MPI version of the library for a given application.
 * SLURM_LOADER_LOAD_NO_MPI_LIB=gromacs */
#define SCHED_LOADER_LOAD_NO_MPI_LIB "SLURM_LOADER_LOAD_NO_MPI_LIB"
#define SCHED_MAX_IMC_FREQ						"SLURM_MAX_IMC_FREQ"
#define SCHED_MIN_IMC_FREQ						"SLURM_MIN_IMC_FREQ"
/* This variable specifies the IMC freq must be selected by the EAR policy */
#define SCHED_SET_IMC_FREQ          "SLURM_SET_IMC_FREQ"
#define SCHED_IMC_EXTRA_TH          "SLURM_IMC_EXTRA_TH"
#define SCHED_LET_HW_CTRL_IMC       "SLURM_LET_HW_CTRL_IMC"
#define SCHED_EXCLUSIVE_MODE        "SLURM_EXCLUSIVE_MODE"
#define SCHED_USE_EARL_PHASES       "SLURM_USE_EARL_PHASES"
#define SCHED_ENABLE_LOAD_BALANCE  	"SLURM_ENABLE_LOAD_BALANCE"
#define SCHED_USE_TURBO_FOR_CP      "SLURM_USE_TURBO_FOR_CP"
#define SCHED_MIN_CPUFREQ           "SLURM_MIN_CPUFREQ"
#define SCHED_NETWORK_USE_IMC       "SLURM_NETWORK_USE_IMC"
#define SCHED_TRY_TURBO             "SLURM_TRY_TURBO"
#define SCHED_LOAD_BALANCE_TH       "SLURM_LOAD_BALANCE_TH"
#define SCHED_EAR_REPORT_ADD				"SLURM_EAR_REPORT_ADD"
#define SCHED_USE_ENERGY_MODELS			"SLURM_USE_ENERGY_MODELS"






/* Asks EARL to write verbose messages in separate per_task files */
#define SCHED_EARL_VERBOSE_PATH "SLURM_EARL_VERBOSE_PATH"



/* Modifies the verbosity of the loader */
#define SCHED_LOADER_VERBOSE "SLURM_LOADER_VERBOSE"
// Sets the value of the loader's verbosity.
#define FLAG_LOAD_VERB "SLURM_LOADER_VERBOSE"


// Delivered by SLURM, this flag contains the task PID.
#define FLAG_TASK_PID  "SLURM_TASK_PID"
#define SCHED_JOB_ID  "SLURM_JOB_ID"
#define SCHED_STEP_ID "SLURM_STEP_ID"
#define SCHED_STEPID "SLURM_STEPID"
#define SCHED_JOB_ACCOUNT "SLURM_JOB_ACCOUNT"
#define SCHED_NUM_TASKS "SLURM_STEP_TASKS_PER_NODE"
#define SCHED_JOB_NAME "SLURM_JOB_NAME"
#define SCHED_NUM_NODES "SLURM_NNODES"
#define SCHED_STEP_NUM_NODES "SLURM_STEP_NUM_NODES"

#define NULL_JOB_ID getpid()
#define NULL_STEPID (0xfffffffe)
#define NULL_ACCOUNT "NO_SLURM_ACCOUNT"





// These variables controls the behaviour for new infrastructure. They affect features under development.
// These three affects to verbose effects: Prints all the signatures in state.c, and print per node or all shared signatures
#define SCHED_EAR_MIN_PERC_MPI "SLURM_EAR_MIN_PERC_MPI"
#define SCHED_EAR_RED_FREQ_IN_MPI "SLURM_EAR_RED_FREQ_IN_MPI"
#define SCHED_EAR_SHOW_SIGNATURES "SLURM_EAR_SHOW_SIGNATURES"
#define REPORT_NODE_SIGNATURES "SLURM_REPORT_NODE_SIGNATURES"
#define REPORT_ALL_SIGNATURES "SLURM_REPORT_ALL_SIGNATURES"
/* Force to shares all the signatures */
#define SHARE_INFO_PER_PROCESS "SLURM_SHARE_INFO_PER_PROCESS"
/* Force to share only the average signature per node */
#define SHARE_INFO_PER_NODE 	"SLURM_SHARE_INFO_PER_NODE"
#define USE_APP_MGR_POLICIES	"SLURM_APP_MGR_POLICIES"
#define EAR_STATS 						"SLURM_GET_EAR_STATS"
#define SCHED_GET_EAR_STATS   "SLURM_GET_EAR_STATS"
#define EAR_POLICY_GRAIN			"SLURM_EAR_POLICY_GRAIN"

#endif //SCHED_SLURM

#endif //CONFIG_ENVIRONMENT_H
