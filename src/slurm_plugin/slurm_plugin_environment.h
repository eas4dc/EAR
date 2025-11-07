/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_SLURM_PLUGIN_ENVIRONMENT_H
#define EAR_SLURM_PLUGIN_ENVIRONMENT_H

#include <common/config/config_env.h>

// Verbosity
#ifdef ERUN
#define plug_verbose(sp, level, ...)                                                                                   \
    if (plug_verbosity_test(sp, level) == 1) {                                                                         \
        if (level > 0)                                                                                                 \
            fprintf(stderr, "%s %s ", plug_host(sp), plug_context_str(sp));                                            \
        fprintf(stderr, __VA_ARGS__);                                                                                  \
        fprintf(stderr, "\n");                                                                                         \
    }
#define plug_error(sp, ...)                                                                                            \
    if (plug_verbosity_test(sp, 1) == 1) {                                                                             \
        fprintf(stderr, "%s %s ERROR, ", plug_host(sp), plug_context_str(sp));                                         \
        fprintf(stderr, __VA_ARGS__);                                                                                  \
        fprintf(stderr, "\n");                                                                                         \
    }
#else
#define plug_verbose(sp, level, ...)                                                                                   \
    if (plug_verbosity_test(sp, level) == 1) {                                                                         \
        slurm_error("EARPLUG, " __VA_ARGS__);                                                                          \
    }
#define plug_error(sp, ...)                                                                                            \
    if (plug_verbosity_test(sp, 1) == 1) {                                                                             \
        slurm_error("EARPLUG ERROR, " __VA_ARGS__);                                                                    \
    }
#endif

#define fail(function) (function != ESPANK_SUCCESS)

typedef char *plug_component_t;
typedef int plug_context_t;

struct component_s {
    plug_component_t plugin;
    plug_component_t library;
} Component __attribute__((weak)) = {
    .plugin  = "SLURM_EPC_PLUGIN", // EPC: EAR PLUGIN (CORE) COMPONENT
    .library = "SLURM_EPC_LIBRARY",
};

struct context_s {
    plug_context_t error;
    plug_context_t srun;
    plug_context_t sbatch;
    plug_context_t remote;
    plug_context_t local;
} Context __attribute__((weak)) = {
    .error = S_CTX_ERROR, .srun = S_CTX_LOCAL, .sbatch = S_CTX_ALLOCATOR, .remote = S_CTX_REMOTE, .local = -1};

struct constring_s {
    char *error;
    char *srun;
    char *sbatch;
} Constring __attribute__((weak)) = {
    .error  = "error",
    .srun   = "srun",
    .sbatch = "sbatch",
};

typedef struct varname_s {
    char *comp;  // Plugin core features enabling/disable
    char *mod;   // Plugin small behavior modificators/variables
    char *flag;  // Plugin flags
    char *slurm; // Own SLURM variables
    char *ear;   // Inner EAR variables
} varnames_t;

struct variables_s {
    varnames_t comp_libr;
    varnames_t comp_plug;
    varnames_t plug_verbose;
    varnames_t hack_loader;
    varnames_t ctx_last;
    varnames_t ctx_was_sbatch;
    varnames_t ctx_was_srun;
    varnames_t con_eard_sbatch;
    varnames_t con_eard_srun;
    varnames_t con_eard_task;
    varnames_t con_eargmd;
    varnames_t user;
    varnames_t group;
    varnames_t account;
    varnames_t name_app;
    varnames_t job_id;
    varnames_t job_node_list;
    varnames_t job_node_count;
    varnames_t step_id;
    varnames_t step_node_list;
    varnames_t step_node_count;
    varnames_t local_id;
    varnames_t cpus_node_num;
    varnames_t verbose;
    varnames_t policy;
    varnames_t policy_th;
    varnames_t frequency;
    varnames_t p_state;
    varnames_t learning;
    varnames_t tag;
    varnames_t path_usdb;
    varnames_t path_trac;
    varnames_t perf_pen;
    varnames_t eff_gain;
    varnames_t path_temp;
    varnames_t path_install;
    varnames_t task_pid;
    varnames_t ld_preload;
    varnames_t ld_library;
    varnames_t is_erun;
} Var __attribute__((weak)) = {
    .comp_libr    = {.comp = "SLURM_COMP_LIBRARY"}, // COMP: COMPONENT
    .comp_plug    = {.comp = "SLURM_COMP_PLUGIN"},
    .plug_verbose = {.mod = "SLURM_COMP_VERBOSE"},
    .hack_loader =
        {
            .mod = HACK_LOADER_FILE,
        },
    .ctx_last =
        {
            .mod = "SLURM_EPV_LAST_CONTEXT",
        },
    .ctx_was_sbatch =
        {
            .mod = "SLURM_EPV_PASSED_SBATCH",
        },
    .ctx_was_srun =
        {
            .mod = "SLURM_EPV_PASSED_SRUN",
        },
    .con_eard_sbatch =
        {
            .mod = "SLURM_EPV_CON_EARD_SBATCH",
        },
    .con_eard_srun =
        {
            .mod = "SLURM_EPV_CON_EARD_SRUN",
        },
    .con_eard_task =
        {
            .mod = "SLURM_EPV_CON_EARD_TASK",
        },
    .con_eargmd =
        {
            .mod = "SLURM_EPV_CON_EARGMD",
        }, // EPV: EAR PLUGIN VARIABLE
    .user =
        {
            .mod = "SLURM_EPV_USER",
        },
    .group =
        {
            .mod = "SLURM_EPV_GROUP",
        },
    .account =
        {
            .slurm = "SLURM_JOB_ACCOUNT",
        },
    .name_app = {.slurm = "SLURM_JOB_NAME", .ear = ENV_APP_NAME},
    .job_id =
        {
            .slurm = "SLURM_JOB_ID",
        },
    .job_node_list =
        {
            .slurm = "SLURM_JOB_NODELIST",
        },
    .job_node_count =
        {
            .slurm = "SLURM_JOB_NUM_NODES",
        },
    .step_id = {.slurm = "SLURM_STEP_ID"},
    .step_node_list =
        {
            .slurm = "SLURM_STEP_NODELIST",
        },
    .step_node_count =
        {
            .slurm = "SLURM_STEP_NUM_NODES",
        },
    .local_id =
        {
            .slurm = "SLURM_LOCALID",
        },
    .cpus_node_num =
        {
            .slurm = "SLURM_CPUS_ON_NODE",
        },
    .verbose      = {.flag = "SLURM_EPF_VERBOSITY", .ear = ENV_FLAG_VERBOSITY}, // EPF: EAR PLUGIN FLAG
    .policy       = {.flag = "SLURM_EPF_POLICY", .ear = ENV_FLAG_POLICY},
    .policy_th    = {.flag = "SLURM_EPF_POLICY_TH", .ear = ENV_FLAG_POLICY_TH},
    .frequency    = {.flag = "SLURM_EPF_FREQUENCY", .ear = ENV_FLAG_FREQUENCY},
    .p_state      = {.flag = "SLURM_EPF_PSTATE", .ear = ENV_FLAG_PSTATE},
    .learning     = {.flag = "SLURM_EPF_LEARNING_PHASE", .ear = ENV_FLAG_IS_LEARNING},
    .tag          = {.flag = "SLURM_EPF_ENERGYT_AG", .ear = ENV_FLAG_ENERGY_TAG},
    .path_usdb    = {.flag = "SLURM_EPF_USER_DB", .ear = ENV_FLAG_PATH_USERDB},
    .path_trac    = {.flag = "SLURM_EPF_TRACE_PATH", .ear = ENV_FLAG_PATH_TRACE},
    .perf_pen     = {.ear = ENV_FLAG_POLICY_PENALTY},
    .eff_gain     = {.ear = ENV_FLAG_POLICY_GAIN},
    .path_temp    = {.ear = ENV_PATH_TMP},
    .path_install = {.ear = ENV_PATH_EAR},
    .task_pid     = {.ear = FLAG_TASK_PID},
    .ld_preload   = {.ear = "LD_PRELOAD"},
    .ld_library   = {.ear = "LD_LIBRARY_PATH"},
    .is_erun      = {.ear = SCHED_IS_ERUN},
};

/*
 * Agnostic environment manipulation
 */
int unsetenv_agnostic(spank_t sp, char *var);

int setenv_agnostic(spank_t sp, char *var, const char *val, int ow);

int getenv_agnostic(spank_t sp, char *var, char *buf, int len);
// Returns 1 if the value of the 'var' environment variable is equal to 'val'.
int isenv_agnostic(spank_t sp, char *var, char *val);
// Replaces the value of the 'var_dst' with the value of 'var_src'
int repenv_agnostic(spank_t sp, char *var_src, char *var_dst);

void printenv_agnostic(spank_t sp, char *var);

/*
 * Others
 */
int plug_component_setenabled(spank_t sp, plug_component_t comp, int enabled);

int plug_component_isenabled(spank_t sp, plug_component_t comp);

char *plug_host(spank_t sp);

char *plug_context_str(spank_t sp);

int plug_context_is(spank_t sp, plug_context_t ctxt);

int plug_context_was(plug_serialization_t *sd, plug_context_t ctxt);

int plug_verbosity_test(spank_t sp, int level);

char *plug_acav_get(int ac, char *av[], char *string);

#endif // EAR_SLURM_PLUGIN_ENVIRONMENT_H
