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
	char *loc; // Variables from user environment
	char *rem; // Variables from local to remote
	char *ear; // Variables from remote to task
	char *cmp; // Component variables
	char *hck; // Hack variables
} varnames_t;

struct variables_s {
	varnames_t comp_libr;
	varnames_t comp_plug;
	varnames_t comp_verb;
	varnames_t hack_load;
	varnames_t verbose;
	varnames_t policy;
	varnames_t policy_th;
	varnames_t frequency;
	varnames_t p_state;
	varnames_t learning;
	varnames_t tag;
	varnames_t path_usdb;
	varnames_t path_trac;
//	varnames_t gm_host;
//	varnames_t gm_port;
	varnames_t gm_secure;
	varnames_t perf_pen;
	varnames_t eff_gain;
	varnames_t name_app;
	varnames_t user;
	varnames_t group;
	varnames_t account;
	varnames_t path_temp;
	varnames_t path_inst;
	varnames_t job_nodl;
	varnames_t job_nodn;
	varnames_t step_nodl;
	varnames_t step_nodn;
	varnames_t task_pid;
	varnames_t ctx_last;
	varnames_t was_sbac;
	varnames_t was_srun;
	varnames_t ld_prel;
	varnames_t ld_libr;
	varnames_t node_num;
	varnames_t version;
//	varnames_t nodes_allowed;
//	varnames_t nodes_excluded;
}
	Var __attribute__((weak)) =
{
.comp_libr = { .cmp = "SLURM_COMP_LIBRARY" },
.comp_plug = { .cmp = "SLURM_COMP_PLUGIN"  },
.comp_verb = { .cmp = "SLURM_COMP_VERBOSE" },
.hack_load = { .hck =  HACK_FILE_LOAD      },
.verbose   = { .loc = "SLURM_LOC_VERB",      .ear = VAR_OPT_VERB      },
.policy    = { .loc = "SLURM_LOC_POLI",      .ear = VAR_OPT_POLI      },
.policy_th = { .loc = "SLURM_LOC_POTH",      .ear = VAR_OPT_THRA      },
.frequency = { .loc = "SLURM_LOC_FREQ",      .ear = VAR_OPT_FREQ      },
.p_state   = { .loc = "SLURM_LOC_PSTA",      .ear = VAR_OPT_PSTA      },
.learning  = { .loc = "SLURM_LOC_LERN",      .ear = VAR_OPT_LERN      },
.tag       = { .loc = "SLURM_LOC_ETAG",      .ear = VAR_OPT_ETAG      },
.path_usdb = { .loc = "SLURM_LOC_USDB",      .ear = VAR_OPT_USDB      },
.path_trac = { .loc = "SLURM_LOC_TRAC",      .ear = VAR_OPT_TRAC      },
//.gm_host   = { .loc = "SLURM_LOC_GMHS",      .ear = ""                },
//.gm_port   = { .loc = "SLURM_LOC_GMPR",      .ear = ""                },
.gm_secure = { .loc = "SLURM_LOC_GMSC",      .ear = ""                },
.perf_pen  = { .loc = "",                    .ear = VAR_OPT_THRB      },
.eff_gain  = { .loc = "",                    .ear = VAR_OPT_THRC      },
.name_app  = { .rem = "SLURM_JOB_NAME",      .ear = VAR_APP_NAME      },
.user      = { .rem = "SLURM_ERUSER",        .ear = "" },
.group     = { .rem = "SLURM_ERGRUP",        .ear = "" },
.account   = { .rem = "SLURM_JOB_ACCOUNT",   .ear = "" },
.path_temp = { .rem = "",                    .ear = VAR_TMP_PATH      },
.path_inst = { .rem = "",                    .ear = VAR_INS_PATH      },
.job_nodl  = { .rem = "SLURM_JOB_NODELIST",  .ear = "" },
.job_nodn  = { .rem = "SLURM_JOB_NUM_NODES", .ear = "" },
.step_nodl = { .rem = "SLURM_STEP_NODELIST", .ear = "" },
.step_nodn = { .rem = "SLURM_STEP_NUM_NODES",.ear = "" },
.task_pid  = { .rem =  FLAG_TASK_PID,        .ear = "" },
.ctx_last  = { .rem = "SLURM_ERLAST",        .ear = "" },
.was_sbac  = { .rem = "SLURM_ERSBAC",        .ear = "" },
.was_srun  = { .rem = "SLURM_ERSRUN",        .ear = "" },
.ld_prel   = { .rem = "",                    .ear = "LD_PRELOAD"      },
.ld_libr   = { .rem = "",                    .ear = "LD_LIBRARY_PATH" },
.node_num  = { .loc = "SLURM_NNODES",        .ear = "" },
.version   = { .loc = "SLURM_EAR_MPI_VERSION",     .ear = ""          },
//.nodes_allowed  = { .rem = "SLURM_NODES_ALLOWED",  .ear = ""          },
//.nodes_excluded = { .rem = "SLURM_NODES_EXCLUDED", .ear = ""          },
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
