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
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef EAR_SLURM_PLUGIN_ENVIRONMENT_H
#define EAR_SLURM_PLUGIN_ENVIRONMENT_H

#include <common/config/config_env.h>

// Verbosity
#ifdef ERUN
	#define plug_verbose(sp, level, ...) \
        	if (plug_verbosity_test(sp, level) == 1) { \
				if (level > 0) fprintf(stderr, "%s %s ", plug_host(sp), plug_context_str(sp)); \
				fprintf(stderr, __VA_ARGS__); \
				fprintf(stderr, "\n"); \
        }
	#define plug_error(sp, ...) \
        	if (plug_verbosity_test(sp, 1) == 1) { \
				fprintf(stderr, "%s %s ERROR, ", plug_host(sp), plug_context_str(sp)); \
				fprintf(stderr, __VA_ARGS__); \
				fprintf(stderr, "\n"); \
        	}
#else
	#define plug_verbose(sp, level, ...) \
        	if (plug_verbosity_test(sp, level) == 1) { \
                	slurm_error("EARPLUG, " __VA_ARGS__); \
        	}
	#define plug_error(sp, ...) \
        	if (plug_verbosity_test(sp, 1) == 1) { \
				slurm_error("EARPLUG ERROR, " __VA_ARGS__); \
        	}
#endif

#define fail(function) \
	(function != ESPANK_SUCCESS)


typedef char *plug_component_t;
typedef int   plug_context_t;

struct component_s {
	plug_component_t plugin;
	plug_component_t library;
} Component __attribute__((weak)) = {
	.plugin  = "SLURM_ECPLUG",
	.library = "SLURM_ECLIBR",
};

struct context_s {
	plug_context_t error;
	plug_context_t srun;
	plug_context_t sbatch;
	plug_context_t remote;
	plug_context_t local;
} Context __attribute__((weak)) = {
	.error  = S_CTX_ERROR,
	.srun   = S_CTX_LOCAL,
	.sbatch = S_CTX_ALLOCATOR,
	.remote = S_CTX_REMOTE,
	.local  = -1
};

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
    char *comp; // Plugin full features enabling/disable
    char *mod;  // Plugin small behavior modificators
	char *flag; // Plugin flags
	char *slurm;  // Own SLURM variables
	char *ear;  // Inner EAR variables
    char *info; // Auxiliar variables for the sake of information
    char *erun; // Just for ERUN
} varnames_t;

struct variables_s {
    varnames_t comp_libr;
    varnames_t comp_plug;
    varnames_t plug_verbose;
    varnames_t hack_loader;
    varnames_t gm_secure;
    varnames_t ctx_last;
    varnames_t was_sbatch;
    varnames_t was_srun;
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
    varnames_t path_etc;
    varnames_t lib_enabled;
    varnames_t task_pid;
    varnames_t ld_preload;
    varnames_t ld_library;
    varnames_t is_erun;
}
	Var __attribute__((weak)) =
{
.comp_libr       = { .comp  = "SLURM_COMP_LIBRARY"                                    }, // MOD
.comp_plug       = { .comp  = "SLURM_COMP_PLUGIN"                                     }, // MOD
.plug_verbose    = { .mod   = "SLURM_COMP_VERBOSE"                                    }, // MOD (not a component)
.hack_loader     = { .mod   = HACK_LOADER_FILE,        .ear = ""                      }, // MOD
.gm_secure       = { .mod   = "SLURM_LOC_GMSC",        .ear = ""                      }, // MOD
.ctx_last        = { .mod   = "SLURM_ERLAST",          .ear = ""                      }, // MOD
.was_sbatch      = { .mod   = "SLURM_ERSBAC",          .ear = ""                      }, // MOD
.was_srun        = { .mod   = "SLURM_ERSRUN",          .ear = ""                      }, // MOD
.user            = { .info  = "SLURM_ERUSER",          .ear = ""                      }, // INFO
.group           = { .info  = "SLURM_ERGRUP",          .ear = ""                      }, // INFO ---------
.account         = { .slurm = "SLURM_JOB_ACCOUNT",     .ear = ""                      }, // SLURM
.name_app        = { .slurm = "SLURM_JOB_NAME",        .ear = ENV_APP_NAME            }, // SLURM & EAR
.job_id          = { .slurm = "SLURM_JOB_ID",                                         }, // SLURM (used in erun)
.job_node_list   = { .slurm = "SLURM_JOB_NODELIST",    .ear = ""                      }, // SLURM
.job_node_count  = { .slurm = "SLURM_JOB_NUM_NODES",   .ear = ""                      }, // SLURM
.step_id         = { .slurm = "SLURM_STEP_ID"                                         }, // SLURM (used in erun)
.step_node_list  = { .slurm = "SLURM_STEP_NODELIST",   .ear = ""                      }, // SLURM
.step_node_count = { .slurm = "SLURM_STEP_NUM_NODES",  .ear = ENV_APP_NUM_NODES       }, // SLURM & EAR
.local_id        = { .slurm = "SLURM_LOCALID",                                        }, // SLURM
.cpus_node_num   = { .slurm = "SLURM_CPUS_ON_NODE",    .ear = ""                      }, // SLURM
.verbose         = { .flag  = "SLURM_LOC_VERB",        .ear = ENV_FLAG_VERBOSITY      }, // FLAG & EAR
.policy          = { .flag  = "SLURM_LOC_POLI",        .ear = ENV_FLAG_POLICY         }, // FLAG & EAR
.policy_th       = { .flag  = "SLURM_LOC_POTH",        .ear = ENV_FLAG_POLICY_TH      }, // FLAG & EAR
.frequency       = { .flag  = "SLURM_LOC_FREQ",        .ear = ENV_FLAG_FREQUENCY      }, // FLAG & EAR
.p_state         = { .flag  = "SLURM_LOC_PSTA",        .ear = ENV_FLAG_PSTATE         }, // FLAG & EAR
.learning        = { .flag  = "SLURM_LOC_LERN",        .ear = ENV_FLAG_IS_LEARNING    }, // FLAG & EAR
.tag             = { .flag  = "SLURM_LOC_ETAG",        .ear = ENV_FLAG_ENERGY_TAG     }, // FLAG & EAR
.path_usdb       = { .flag  = "SLURM_LOC_USDB",        .ear = ENV_FLAG_PATH_USERDB    }, // FLAG & EAR
.path_trac       = { .flag  = "SLURM_LOC_TRAC",        .ear = ENV_FLAG_PATH_TRACE     }, // FLAG & EAR
.perf_pen        = {                                   .ear = ENV_FLAG_POLICY_PENALTY }, // EAR
.eff_gain        = {                                   .ear = ENV_FLAG_POLICY_GAIN    }, // EAR
.path_temp       = {                                   .ear = ENV_PATH_TMP            }, // EAR (from args)
.path_install    = {                                   .ear = ENV_PATH_EAR            }, // EAR (from args)
.path_etc        = { .erun  = "EAR_ETC",               .ear = "",                     }, // (for erun, not passed to EAR)
.lib_enabled     = { .erun  = "EAR_DEFAULT",           .ear = "",                     }, // (for erun)
.task_pid        = {                                   .ear = "TASK_PID"              }, // getpid() why are we redefining?
.ld_preload      = {                                   .ear = "LD_PRELOAD"            }, // EAR
.ld_library      = {                                   .ear = "LD_LIBRARY_PATH"       }, // EAR
.is_erun         = {                                   .ear = SCHED_IS_ERUN           }, // EAR
};

/*
 * Agnostic environment manipulation
 */
int unsetenv_agnostic(spank_t sp, char *var);

int setenv_agnostic(spank_t sp, char *var, const char *val, int ow);

int getenv_agnostic(spank_t sp, char *var, char *buf, int len);

int isenv_agnostic(spank_t sp, char *var, char *val);

int repenv_agnostic(spank_t sp, char *var_old, char *var_new);

int apenv_agnostic(char *dst, char *src, int len);

int exenv_agnostic(spank_t sp, char *var);

int valenv_agnostic(spank_t sp, char *var, int *val);

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

int plug_verbosity_silence(spank_t sp);

char *plug_acav_get(int ac, char *av[], char *string);

#endif //EAR_SLURM_PLUGIN_ENVIRONMENT_H
