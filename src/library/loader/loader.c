/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#define _GNU_SOURCE
#include <errno.h>
#include <dlfcn.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <common/config.h>
#include <common/output/debug.h>
#include <common/utils/string.h>
#include <library/loader/loader.h>
#include <library/loader/module_common.h>
#include <library/loader/module_mpi.h>

static int init()
{
    char *verb;
    char *task;
    pid_t pid1 = 1;
    pid_t pid2 = 0;

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
        verbose(4, "LOADER: Loader pids: %d/%d", pid1, pid2);
    } else {
        verbose(4, "LOADER: Loader pid: %d", pid1);
    }
    return 1;
    // return pid1 == pid2;
}

static int is_a_valid_program(char **bin_names)
{
    int l;
    // If the program name is one of these is invalid and nothing is loaded
    for (l = 0; bin_names[l] != NULL; ++l) {
        if (strstr(program_invocation_name, bin_names[l]) != NULL) {
            verbose(2, "LOADER: Program %s invalid and is loaded without EAR", program_invocation_name);
            return 0;
        }
    }
    return 1;
}

static int is_a_forced_load(char **bin_names)
{
    char *envar     = NULL;
    char **list     = NULL;
    uint list_count = 0;
    int l           = 0;

    // If the program name is one of these we load the sequential library
    for (l = 0; bin_names[l] != NULL; ++l) {
        if (strstr(program_invocation_name, bin_names[l]) != NULL) {
            verbose(2, "LOADER: looking for forced program '%s': found!", bin_names[l]);
            return 1;
        }
        verbose(2, "LOADER: looking for forced program '%s': not found", bin_names[l]);
    }
    if ((envar = ear_getenv(FLAG_LOADER_APPLICATION)) == NULL) {
        return 0;
    }
    verbose(2, "LOADER: Using %s = %s as special cases", FLAG_LOADER_APPLICATION, envar);
    if (strtoa((const char *) envar, ',', &list, &list_count) == NULL) {
        return 0;
    }
    for (l = 0; l < list_count; ++l) {
        if (strstr(program_invocation_name, list[l]) != NULL) {
            verbose(2, "LOADER: looking for forced program '%s': found!", list[l]);
            strtoa_free(list);
            return 1;
        }
    }
    strtoa_free(list);
    return 0;
}

void __attribute__((constructor)) loader()
{
    char *bins_invalid[] = { "bash", "hydra", NULL };
    char *bins_forced[]  = { "python", "python3", "julia", NULL };
    char *syms_openmp[]  = { "omp_get_max_threads", "mkl_get_max_threads", NULL };
    char *syms_cuda[]    = { "cudaRuntimeGetVersion", NULL };
    char *libs_oneapi[]  = { "libsycl.so", NULL };
    // If FLAG_LOAD_MPI environment variable is set, the loader assumes the MPI
    // version of the library is the version the user wants and no other library
    // is loaded before. This is required because in example, if EAR MPI library
    // is loaded in lazy mode (from MPI symbols), we need a way to stop the
    // loader to open the sequential library because the program name matches
    // one of the forced programs names.
    if (!init()) {
        return;
    }
    if (!is_a_valid_program(bins_invalid)) {
        return;
    }
    if (load_libear_mpi()) { return; }
    if (load_libear_nonspec(OPENMP_EXT, syms_openmp, NULL       )) { return; }
    if (load_libear_nonspec(  CUDA_EXT, syms_cuda  , NULL       )) { return; }
    if (load_libear_nonspec(ONEAPI_EXT, NULL       , libs_oneapi)) { return; }
    if (!is_a_forced_load(bins_forced)) { return; }
    if (load_libear_nonspec(   DEF_EXT, NULL       , NULL       )) { return; }
    return;
}