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

#include <dlfcn.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/system/symplug.h>
#include <common/string_enhanced.h>
#include <library/loader/loader.h>
#include <library/loader/module_mpi.h>
#include <library/loader/module_mpic.h>
#include <library/loader/module_mpif.h>
#include <library/loader/module_cuda.h>
#include <library/loader/module_common.h>
#if FEAT_MPI_LOADER
#include <library/api/mpi.h>
#endif
#include <library/api/mpic.h>
#include <library/api/mpif.h>

#if !FEAT_MPI_LOADER
typedef struct mpic_s {
    void *Init;
    void *init;
} mpic_t;

typedef mpic_t mpif_t;
#endif

static mpic_t next_mpic;
static mpif_t next_mpif;
       mpic_t ear_mpic;
       mpif_t ear_mpif;
static uint enabled_mpi   = 0;
static uint found_intel   = 0;
static uint found_open    = 0;
static uint found_mvapich = 0;
static uint found_fujitsu = 0;
static uint found_cray    = 0;
static uint forced_load   = 0;
static char *version_env  = NULL;
static uint func_version_detected = 0;

static void (*func_constructor) (void);
static void (*func_destructor)  (void);
static int  (*func_version)     (char *version, int *resultlen);

int module_mpi_is_enabled() {
    return enabled_mpi;
}

void set_mpi_enabled() {
    enabled_mpi = 1;
}

int module_mpi_is_detected() {
    return func_version_detected;
}

int module_mpi_is_intel() {
    return found_intel;
}

int module_mpi_is_open() {
    return found_open;
}

void module_mpi_set_forced() {
    forced_load = 1;
}

static void module_mpi_get_libear(char *path_lib_so, char *libhack, char *path_so, size_t path_so_size, int *lang_c, int *lang_f)
{
    static char buffer[4096];
    char *extension = NULL;
    char *vers = NULL;
    int len = 4096;

    *lang_c = 0;
    *lang_f = 0;

    if (path_lib_so != NULL) { verbose(2, "LOADER: path_lib_so %s", path_lib_so); }
    if (libhack     != NULL) { verbose(2, "LOADER: using HACK %s", libhack); }
    // Getting the library version
    if (func_version_detected) func_version(buffer, &len);
    else if (version_env != NULL) {
        strcpy(buffer, version_env);
    } else {
        return;
    }

    // Passing to lowercase
    strtolow(buffer);

    if (strstr(buffer, "intel") != NULL) {
        found_intel = 1;
    } else if ((strstr(buffer, "open mpi") != NULL) || (strstr(buffer, "ompi") != NULL)) {
        found_open = 1;
    } else if (strstr(buffer, "mvapich") != NULL) {
        found_mvapich = 1;
    } else if (strstr(buffer, "fujitsu mpi") != NULL) {
        verbose(2, "LOADER: fujitsu mpi found");
        found_fujitsu = 1;
    } else if (strstr(buffer, "cray mpich") != NULL ) {
        verbose(2, "LOADER: cray mpi found");
        found_cray = 1;
    } else {
        verbose(2, "LOADER: no mpi version found in list of recognized versions '%s'", buffer);
        return;
    }
    verbose(2, "LOADER: mpi_get_version (intel: %d, open: %d, mvapich: %d, fujitsu: %d, cray: %d)",
            found_intel, found_open, found_mvapich, found_fujitsu, found_cray);

    //
    extension = INTEL_EXT;
    if (found_open) {
        extension = OPENMPI_EXT;
    }
    if (found_fujitsu) {
        extension = FUJITSU_MPI_EXT;
    }
    if (found_cray) {
        extension = CRAY_MPI_EXT;
    }

    /* Warning , no actions yet for mvapich */
    if ((vers = ear_getenv(HACK_MPI_VERSION)) != NULL) {
        if (strlen(vers) > 0) {
            xsnprintf(buffer, sizeof(buffer), "%s.so", vers);
            extension = buffer;
        }
    }
    if (path_lib_so != NULL) {
        verbose(2, "LOADER: path %s rellibname %s extension %s", path_lib_so, REL_NAME_LIBR, extension);
    }
    //
    if (libhack == NULL) {
        xsnprintf(path_so, path_so_size, "%s/%s.%s", path_lib_so, REL_NAME_LIBR, extension);
    } else {
        xsnprintf(path_so, path_so_size, "%s", libhack);
    }

    verbose(2, "LOADER: lib path %s", path_so);
    if (!module_file_exists(path_so)) {
        verbose(0, "EARL cannot be found at '%s'", path_so);
        return;
    }
    *lang_f = !found_intel;
    *lang_c = 1;
    return;
}


static int module_mpi_dlsym(char *path_so, int lang_c, int lang_f)
{
    void **next_mpic_v = (void **) &next_mpic;
    void **next_mpif_v = (void **) &next_mpif;
    void **ear_mpic_v  = (void **) &ear_mpic;
    void **ear_mpif_v  = (void **) &ear_mpif;
    void *libear;
    int i;

    verbose(2, "LOADER: module_mpi_dlsym loading library %s (c: %d, f: %d)",
            path_so, lang_c, lang_f);

    if (func_version_detected) {
#if COUNTDOWN_BASE
        char *disable_cntd = ear_getenv(FLAG_DISABLE_CNTD);
        static char buffer[4096];
        snprintf(buffer, 4096, "%s/libcntd.so", COUNTDOWN_BASE);
        verbose(0, "Using countdown library path %s", buffer);
        //if ((disable_cntd != NULL) || state_fail(symplug_open(buffer, (void **) &next_mpic, mpic_names, MPIC_N))){
        if ((disable_cntd != NULL) || state_fail(symplug_open_flags(buffer, (void **) &next_mpic, mpic_names, MPIC_N, RTLD_GLOBAL | RTLD_LAZY))){
            if (disable_cntd == NULL){
                verbose(1, "Warning, countdown C symbols cannot be loaded, using NEXT symbols");
            } else {
                verbose(1, "CNTD disabled");
            }
            plug_join(RTLD_NEXT, (void **) &next_mpic, mpic_names, MPIC_N);
        }
        //if ((disable_cntd != NULL) || state_fail(symplug_open(buffer, (void **) &next_mpif, mpif_names, MPIF_N ))){
        if ((disable_cntd != NULL) || state_fail(symplug_open_flags(buffer, (void **) &next_mpif, mpif_names, MPIF_N, RTLD_GLOBAL | RTLD_LAZY))){
            if (disable_cntd == NULL){
                verbose(1, "Warning, countdown F symbols cannot be loaded, using NEXT symbols");
            } else {
                verbose(1, "CNTD disabled");
            }
            plug_join(RTLD_NEXT, (void **) &next_mpif, mpif_names, MPIF_N);
        }
#else
        plug_join(RTLD_NEXT, (void **) &next_mpic, mpic_names, MPIC_N);
        plug_join(RTLD_NEXT, (void **) &next_mpif, mpif_names, MPIF_N);
#endif // COUNTDOWN_BASE

        verbose(3, "LOADER: MPI (C) next symbols loaded, Init address is %p", next_mpic.Init);
        verbose(3, "LOADER: MPI (F) next symbols loaded, Init address is %p", next_mpif.init);
        if (next_mpif.init == NULL) {
            void *Initf = dlsym(RTLD_NEXT, "mpi_init_");
            if (Initf != NULL) {
                verbose(3, "LOADER: MPI (F_) mpi_init returns %p, loading again ", Initf);
                plug_join(RTLD_NEXT, (void **) &next_mpif, mpif_names_, MPIF_N);
                if (next_mpif.init != NULL) {
                    verbose(3, "LOADER: MPI (F_) next symbols loaded, Init address is %p", next_mpif.init);
                }
            }
        }
    }

    //
    libear = dlopen(path_so, RTLD_NOW | RTLD_GLOBAL);
    if (libear == NULL) {
        verbose(0, "EARL at %s canot be loaded %s", path_so, dlerror());
    } else {
        verbose(3, "LOADER: dlopen returned %p", libear);
    }

    void (*ear_mpic_setnext)(mpic_t *) = NULL;
    void (*ear_mpif_setnext)(mpif_t *) = NULL;

    if (libear != NULL) {
        ear_mpic_setnext = dlsym(libear, "ear_mpic_setnext");
        ear_mpif_setnext = dlsym(libear, "ear_mpif_setnext");

        if (ear_mpic_setnext == NULL && lang_c) {
            lang_c = 0;
        }
        if (ear_mpif_setnext == NULL && lang_f) {
            lang_f = 0;
        }
        if (!lang_c && !lang_f) {
            verbose(2, "LOADER: the loaded library does not have MPI symbols");
            dlclose(libear);
            libear = NULL;
        }
    }

    if (libear != NULL && lang_c) plug_join(libear, (void **) &ear_mpic, ear_mpic_names, MPIC_N);
    if (libear != NULL && lang_f) plug_join(libear, (void **) &ear_mpif, ear_mpif_names, MPIF_N);
    //
    for (i = 0; i < MPIC_N; ++i) {
        if (ear_mpic_v[i] == NULL) {
            ear_mpic_v[i] = next_mpic_v[i];
        }
    }
    for (i = 0; i < MPIF_N; ++i) {
        if (ear_mpif_v[i] == NULL) {
            ear_mpif_v[i] = next_mpif_v[i];
        }
    }
    // Setting MPI next symbols
    if (!lang_c && !lang_f) {
        return 0;
    }

    if (ear_mpic_setnext != NULL && func_version_detected) ear_mpic_setnext(&next_mpic);
    if (ear_mpif_setnext != NULL && func_version_detected) ear_mpif_setnext(&next_mpif);
    func_constructor = dlsym(libear, "ear_constructor");
    func_destructor  = dlsym(libear, "ear_destructor");
    verbose(3, "LOADER: function constructor %p", func_constructor);
    verbose(3, "LOADER: function destructor %p", func_destructor);

    return 1;
}

static int module_mpi_is()
{
    uint is_mpi = 0;
    version_env = NULL;
    /* We use the default approach of looking for the MPI symbol */
    func_version = dlsym(RTLD_DEFAULT, "PMPI_Get_library_version");
    if (func_version != NULL) {
        func_version_detected = 1;
        verbose(2, "MPI_Get_library_version detected");
        return 1;
    }
    verbose(2, "MPI_Get_library_version NOT detected");

    /* We check the env var only for MPI+Python */
    if ((strstr(program_invocation_name, "python") != NULL) || (strstr(program_invocation_name, "gmx") != NULL) ||
        (strstr(program_invocation_name, "julia") != NULL)) {
        // The env var oversubscribes the library version detected.
        version_env = ear_getenv(FLAG_LOAD_MPI_VERSION);
        if (version_env != NULL) {
            verbose(2, " %s defined = %s", FLAG_LOAD_MPI_VERSION, version_env);
            return 1;
        }
        // TODO: This check is for the transition to the new environment variables.
        // It will be removed when SCHED_LOAD_MPI_VERSION will be removed, on the next release.
        version_env = ear_getenv(SCHED_LOAD_MPI_VERSION);
        if (version_env != NULL) {
            verbose(1, "LOADER: %sWARNING%s %s will be removed on the next EAR release. "
                       "Please, change it by %s in your submission scripts.",
                    COL_RED, COL_CLR, SCHED_LOAD_MPI_VERSION, FLAG_LOAD_MPI_VERSION);
            verbose(2, " %s = %s", FLAG_LOAD_MPI_VERSION, version_env);
            return 1;
        }
    }
    return is_mpi;
}

int module_mpi(char *path_lib_so, char *libhack)
{
    static char path_so[4096];
    int lang_c;
    int lang_f;
    int is_mpi;

    #if !FEAT_MPI_LOADER
    return 0;
    #endif
    verbose(3, "LOADER: loading module MPI");
    if (!(is_mpi = module_mpi_is())) {
        return 0;
    }
    // If we have not detected the symbol and the load is not forced, we just
    // return 1 to mark is an mpi application. forced_load is set to 1 in the
    // MPI_Init or MPI_Init_thread function since we cannot postpone the load
    // there.
    if (is_mpi && !func_version_detected && !forced_load) {
        return 1;
    }
    //
    module_mpi_get_libear(path_lib_so, libhack, path_so, sizeof(path_so), &lang_c, &lang_f);
    //
    if (!module_mpi_dlsym(path_so, lang_c, lang_f)) {
        return is_mpi;
    }
    set_mpi_enabled();
    #if USE_GPUS
    // Once EARl is loaded we can check if the application uses CUDA and then
    // mark as a CUDA app.
    if (module_cuda_is()) ear_cuda_enabled();
    #endif
    if (atexit(module_mpi_destructor) != 0) {
        verbose(2, "LOADER: cannot set exit function");
    }
    if (func_constructor != NULL) {
        func_constructor();
    }
    return is_mpi;
}

void module_mpi_destructor()
{
    verbose(3, "LOADER: loading module mpi (destructor)");
    if (module_mpi_is_enabled()) {
        if (func_destructor != NULL) {
            func_destructor();
        }
    }
}
