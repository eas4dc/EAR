/**************************************************************
*       Energy Aware Runtime (EAR)
*       This program is part of the Energy Aware Runtime (EAR).
*
*       EAR provides a dynamic, transparent and ligth-weigth solution for
*       Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*       BSC Contact     mailto:ear-support@bsc.es
*       Lenovo contact  mailto:hpchelp@lenovo.com
*
*       EAR is free software; you can redistribute it and/or
*       modify it under the terms of the GNU Lesser General Public
*       License as published by the Free Software Foundation; either
*       version 2.1 of the License, or (at your option) any later version.
*
*       EAR is distributed in the hope that it will be useful,
*       but WITHOUT ANY WARRANTY; without even the implied warranty of
*       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*       Lesser General Public License for more details.
*
*       You should have received a copy of the GNU Lesser General Public
*       License along with EAR; if not, write to the Free Software
*       Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*       The GNU LEsser General Public License is contained in the file COPYING
*/

#ifndef EAR_PRIVATE_SPANK_INTERPOSER_H
#define EAR_PRIVATE_SPANK_INTERPOSER_H

#ifndef ERUN
#include <slurm/slurm.h>
#include <slurm/spank.h>
#else
#endif

/*
 * Defines
 */

#define ESPANK_STOP     -1

#ifdef ERUN
#define NO_VAL (0xfffffffe)
#endif

#ifndef ERUN
#define NULL_C NULL
#else
#define NULL_C 0
#endif

#ifdef ERUN
#define SPANK_PLUGIN(name, version)
#endif

/*
 * Types
 */

#ifdef ERUN
struct action_s {
	int error;
	int init;
	int exit;
} Action __attribute__((weak)) = {
	.error = 0,
	.init  = 1,
	.exit  = 2,
};

typedef int spank_t;

enum erun_context {
	S_CTX_ERROR,
	S_CTX_LOCAL,
	S_CTX_REMOTE,
	S_CTX_ALLOCATOR,
};

typedef int spank_context_t;

enum spank_item {
	S_JOB_ID,
	S_JOB_STEPID,
	S_TASK_EXIT_STATUS,
};

typedef enum spank_item spank_item_t;

enum spank_err {
	ESPANK_SUCCESS     = 0,
	ESPANK_ERROR       = 1,
	ESPANK_BAD_ARG     = 2,
	ESPANK_NOSPACE     = 6,
};

typedef enum spank_err spank_err_t;

typedef int (*spank_opt_cb_f) (int val, const char *optarg, int remote);

struct spank_option {
	char *name;
	char *arginfo;
	char *usage;
	int has_arg;
	int val;
	spank_opt_cb_f cb;
};

typedef char *hostlist_t;
#endif

#ifdef ERUN
int slurm_spank_init(spank_t sp, int ac, char *argv[]);
int slurm_spank_slurmd_init(spank_t sp, int ac, char *argv[]);
int slurm_spank_job_prolog(spank_t sp, int ac, char *argv[]);
int slurm_spank_init_post_opt(spank_t sp, int ac, char *argv[]);
int slurm_spank_local_user_init(spank_t sp, int ac, char *argv[]);
int slurm_spank_user_init(spank_t sp, int ac, char *argv[]);
int slurm_spank_task_init_privileged(spank_t sp, int ac, char *argv[]);
int slurm_spank_task_init(spank_t sp, int ac, char *argv[]);
int slurm_spank_task_post_fork(spank_t sp, int ac, char *argv[]);
int slurm_spank_task_exit(spank_t sp, int ac, char *argv[]);
int slurm_spank_job_epilog(spank_t sp, int ac, char *argv[]);
int slurm_spank_slurmd_exit(spank_t sp, int ac, char *argv[]);
int slurm_spank_exit(spank_t sp, int ac, char *argv[]);
#endif

#ifdef ERUN
spank_context_t	 spank_context (void);
spank_err_t	 spank_getenv (spank_t spank, const char *var, char *buf, int len);
spank_err_t		 spank_setenv (spank_t spank, const char *var, const char *val, int overwrite);
spank_err_t		 spank_unsetenv (spank_t spank, const char *var);
spank_err_t		 spank_get_item (spank_t spank, spank_item_t item, int *p);
spank_err_t		 spank_option_register_print(spank_t sp, struct spank_option *opt);
spank_err_t		 spank_option_register_call(int argc, char *argv[], spank_t sp, struct spank_option *opt);
spank_err_t		 spank_option_register(spank_t sp, struct spank_option *opt);
char			*slurm_hostlist_shift (hostlist_t host_list);
hostlist_t		 slurm_hostlist_create (char *node_list);
#endif

#endif //EAR_PRIVATE_SPANK_INTERPOSER_H
