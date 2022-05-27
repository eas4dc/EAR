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


// #define SHOW_DEBUGS 1
#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <library/loader/loader.h>
#include <library/loader/module_common.h>
#include <library/loader/module_mpi.h>
#include <library/loader/module_cuda.h>
#include <library/loader/module_openmp.h>
#include <library/loader/module_default.h>
#include <common/config/config_env.h>
#include <common/output/debug.h>

static int _loaded;

static int init()
{
    char *verb;
    char *task;
    pid_t pid1 = 1;
    pid_t pid2 = 0;

    // Activating verbose
    if ((verb = getenv(FLAG_LOADER_VERBOSE)) != NULL) {
        VERB_SET_EN(1);
        VERB_SET_LV(atoi(verb));
    }
    pid1 = getpid();

    if (SCHED_IS(SCHED_SLURM)) {
        // SLURM prolog/epilog control
        if ((task = getenv(FLAG_TASK_PID)) == NULL) {
            return 0;
        }
        pid2 = (pid_t) atoi(task);
        verbose(4, "LOADER: loader pids: %d/%d", pid1, pid2);
    } else {
        verbose(4, "LOADER: loader pid: %d", pid1);
    }

    return 1;	
    // return pid1 == pid2;
}

int must_load()
{
    if ((strstr(program_invocation_name, "bash") == NULL) &&
            (strstr(program_invocation_name, "hydra") == NULL)){
        verbose(2,"LOADER: Program %s loaded with EAR",program_invocation_name);
        return 1;
    }
    verbose(2,"LOADER: Program %s loaded without EAR",program_invocation_name);
    return 0;
}

int load_no_official()
{
    char *app_to_load=getenv(FLAG_LOADER_APPLICATION);
    if (app_to_load == NULL) {
        // TODO: This check is for the transition to the new environment variables.
        // It will be removed when SCHED_LOADER_LOAD_NO_MPI_LIB will be removed, in the next release.
        app_to_load = getenv(SCHED_LOADER_LOAD_NO_MPI_LIB);
        if (app_to_load != NULL) {
            verbose(1, "LOADER: %sWARNING%s %s will be removed on the next EAR release. "
                    "Please, change it by %s in your submission scripts.",
                    COL_RED, COL_CLR, SCHED_LOADER_LOAD_NO_MPI_LIB, FLAG_LOADER_APPLICATION);
        } else {
            return 0;
        }
    }
    if (strstr(program_invocation_name,app_to_load) == NULL) return 0;
    return 1;
}

int load_no_parallel()
{
		if (strstr(program_invocation_name,"python") != NULL) return 1;
		return 0;
}

void  __attribute__ ((constructor)) loader()
{
    char *path_lib_so;
    char *libhack;


    // Initialization
    if (!init()) {
        verbose(4, "LOADER: escaping the application '%s'", program_invocation_name);
        return;
    }
    verbose(2, "LOADER: loading for application '%s'", program_invocation_name);

    if (must_load()){
        module_get_path_libear(&path_lib_so,&libhack);
        if ((path_lib_so == NULL) && (libhack == NULL)){
            verbose(1,"LOADER EAR path and EAR debug HACKS are NULL");
            return ;
        }

        // Module MPI
        verbose(2,"Tring MPI module");
        _loaded = module_mpi(path_lib_so, libhack);

        if (_loaded) return;
        
				verbose(2,"Tring openmp/mkl module");

        _loaded = module_constructor_openmp(path_lib_so,libhack);
        if (_loaded) return;

        _loaded = module_constructor_cuda(path_lib_so,libhack);
        if (_loaded) return;

        // Module default
        if (load_no_parallel()) {
					verbose(2,"Tring default module because app name is %s", program_invocation_name);
					_loaded = module_constructor(path_lib_so,libhack);
				}
        if (_loaded) return;

        if (load_no_official()) {
            verbose(2,"Tring default module");
            _loaded = module_constructor(path_lib_so,libhack);
        }
    }

    return;
}
