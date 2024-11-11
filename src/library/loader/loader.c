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

#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
// #define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <common/utils/string.h>
#include <common/string_enhanced.h>
#include <common/config/config_env.h>
#include <common/system/execute.h>
#include <library/loader/loader.h>
#include <library/loader/module_mpi.h>
#include <library/loader/module_cuda.h>
#include <library/loader/module_common.h>
#include <library/loader/module_openmp.h>
#include <library/loader/module_oneapi.h>
#include <library/loader/module_default.h>

/* This code is commented to save an example on
 * how to intercept a libc call.

static int (*original_close)(int);

int close(int fd) {
	if (fd == 0) {
		dprintf(2, "WARNING fd is 0!\n");
		print_stack(2);
	}
	return (*original_close)(fd);
}
*/

static int _loaded;

static int init()
{
    char *verb;
    char *task;
    pid_t pid1 = 1;
    pid_t pid2 = 0;

	// original_close = dlsym(RTLD_NEXT, "close");

    // Activating verbose
    if ((verb = ear_getenv(FLAG_LOADER_VERBOSE)) != NULL) {
        VERB_SET_EN(1);
        VERB_SET_LV(atoi(verb));
    }
    pid1 = getpid();

    if (SCHED_IS(SCHED_SLURM)) {
        // SLURM prolog/epilog control
        if ((task = ear_getenv(FLAG_TASK_PID)) == NULL) {
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
        (strstr(program_invocation_name, "hydra") == NULL)) {
        verbose(2, "LOADER: Program %s loaded with EAR", program_invocation_name);
        return 1;
    }
    verbose(2, "LOADER: Program %s loaded without EAR", program_invocation_name);
    return 0;
}

int load_no_official()
{
    char  *envar = ear_getenv(FLAG_LOADER_APPLICATION);
    char  **list;
    uint   list_count;
    int    l;

    if (envar == NULL) {
        // TODO: This check is for the transition to the new environment variables.
        // It will be removed when SCHED_LOADER_LOAD_NO_MPI_LIB will be removed, in the next release.
        envar = ear_getenv(SCHED_LOADER_LOAD_NO_MPI_LIB);
        if (envar != NULL) {
            verbose(1, "LOADER: %sWARNING%s %s will be removed on the next EAR release. "
                       "Please, change it by %s in your submission scripts.",
                    COL_RED, COL_CLR, SCHED_LOADER_LOAD_NO_MPI_LIB, FLAG_LOADER_APPLICATION);
        } else {
            return 0;
        }
    }
    if (envar == NULL) {
        return 0;
    }
    verbose(2, "LOADER: Using %s = %s as special cases", FLAG_LOADER_APPLICATION, envar);
    if (strtoa((const char *) envar, ',', &list, &list_count) == NULL) {
        return 0;
    }
    for (l = 0; l < list_count; ++l) {
        if (strstr(program_invocation_name, list[l]) != NULL) {
            strtoa_free(list);
            return 1;
        }
    }
    strtoa_free(list);
    return 0;
}

int load_no_parallel()
{
    // This cant be since we have developed a new Loader that works with python
    if (strstr(program_invocation_name, "python" ) != NULL) return 1;
    if (strstr(program_invocation_name, "python3") != NULL) return 1;
    if (strstr(program_invocation_name, "julia"  ) != NULL) return 1;
    return 0;
}

void __attribute__ ((constructor)) loader()
{
    char *path_lib_so;
    char *libhack;

    // Initialization
    if (!init()) {
        verbose(4, "LOADER[%d]: escaping the application '%s'", getpid(), program_invocation_name);
        return;
    }
    verbose(2, "LOADER[%d]: loading for application '%s'", getpid(), program_invocation_name);

    if (must_load()) {
        module_get_path_libear(&path_lib_so, &libhack);
        if ((path_lib_so == NULL) && (libhack == NULL)) {
            verbose(1, "LOADER: EAR path and EAR debug HACKS are NULL");
            return;
        }

        // Module MPI
        verbose(2, "LOADER: Trying MPI module");
        _loaded = module_mpi(path_lib_so, libhack);
        if (_loaded) return;
        verbose(2, "LOADER: Trying openmp/mkl module");
        _loaded = module_constructor_openmp(path_lib_so, libhack);
        if (_loaded) return;
        _loaded = module_constructor_cuda(path_lib_so, libhack);
        if (_loaded) return;
        _loaded = module_constructor_oneapi(path_lib_so, libhack);
        if (_loaded) return;
        // Module default
        if (load_no_parallel()) {
            verbose(2, "LOADER: Trying default module because app name is %s", program_invocation_name);
            _loaded = module_constructor(path_lib_so, libhack);
        }
        if (_loaded) return;
        if (load_no_official()) {
            verbose(2, "LOADER: Tring default module");
            _loaded = module_constructor(path_lib_so, libhack);
        }
    }
    return;
}
