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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void (*call_constructor) (void);
static void (*call_destructor ) (void);
static mpic_t calls_mpi_c;
static mpif_t calls_mpi_f;
       mpic_t ear_mpic;
       mpif_t ear_mpif;
static uint   _found_intel   = 0;
static uint   _found_open    = 0;
static uint   _found_mvapich = 0;
static uint   _found_fujitsu = 0;
static uint   _found_cray    = 0;
static uint   _found_hack    = 0;

// Deprecated, clean. There is no sign of this in the header because we don't
// want to be used again, and the symbols are compiled for compatibility.
uint MPI_Get_library_version_detected = 0;
int is_mpi_enabled()      { return 1;            }
int is_openmpi()          { return _found_open;  }
int module_mpi_is_intel() { return _found_intel; }

int module_mpi_is_open() {
    return _found_open;
}

int module_mpi_is_enabled() {
    return _found_intel || _found_open || _found_mvapich || _found_fujitsu || _found_cray;
}

static void *load_mpi_version(void *handler)
{
    return dlsym(handler, "PMPI_Get_library_version");
}

static int load_mpi_functions(void *handler, mpic_t *calls_c, mpif_t *calls_f)
{
    if (handler       == NULL) return 0;
    if (calls_c->Init == NULL) plug_join(handler, (void **) calls_c, mpic_names , MPIC_N);
    if (calls_f->init == NULL) plug_join(handler, (void **) calls_f, mpif_names , MPIF_N);
    if (calls_f->init == NULL) plug_join(handler, (void **) calls_f, mpif_names_, MPIF_N);
    verbose(2, "LOADER: address of MPI_Init (C): %p", calls_c->Init);
    verbose(2, "LOADER: address of mpi_init (F): %p", calls_f->init);
    return (calls_c->Init != NULL);
}

static void *get_libear_handler(char *libear_path)
{
    static void *handler = NULL;
    if (handler == NULL && libear_path != NULL) {
        handler = dlopen(libear_path, RTLD_GLOBAL | RTLD_NOW);
	verbose(2, "LOADER: handler libear: %p (%s)", handler, dlerror());
    }
    return handler;
}

static void *get_libmpi_handler(char *libmpi_path)
{
    static void *handler = NULL;
    if (handler == NULL && libmpi_path != NULL) {
        handler = dlopen(libmpi_path, RTLD_NOLOAD | RTLD_LOCAL | RTLD_NOW);
	    verbose(2, "LOADER: handler libmpi: %p (%s)", handler, dlerror());
    }
    return handler;
}

static void *get_libcnt_handler()
{
    static void *handler = NULL;
    #if COUNTDOWN_BASE
    if (handler == NULL) {
        handler = dlopen(COUNTDOWN_BASE "/libcntd.so", RTLD_GLOBAL | RTLD_LAZY);
    }
    #endif
    return handler;
}

static int append_extension(char *string_version, char *string_cmp, char *extension, char *buffer)
{
    if (strstr(string_version, string_cmp) == NULL || extension == NULL) {
        verbose(2, "LOADER: is '%-10s' in version string? 0", string_cmp);
        return 0;
    }
    sprintf(&buffer[strlen(buffer)], "/%s.%s", REL_NAME_LIBR, extension);
    verbose(2, "LOADER: is '%-10s' in version string? 1", string_cmp);
    return 1;
}

static char *build_libear_path(void *call_mpi_version)
{
    static char *buffer_path = NULL;
    static char *buffer_hack = NULL;
    char buffer_version[1024];
    int length = sizeof(buffer_version);
    char HACK_EXT[64];

    // We already found that path
    if (buffer_path != NULL) {
        return buffer_path;
    }
    // Getting EAR installation path and/or HACK absolute path
    module_get_path_libear(&buffer_path, &buffer_hack);
    verbose(2, "LOADER: env HACK/EAR_INSTALL_PATH => %s", buffer_path);
    verbose(2, "LOADER: env HACK_LIBRARY_FILE     => %s", buffer_hack);
    if (buffer_path == NULL && buffer_hack == NULL) {
        return NULL;
    }
    // FLAG_LOAD_MPI_VERSION => fakes an MPI_Get_version call
    //      HACK_MPI_VERSION => writes an extension, in form of $val.so (without so)
    verbose(2, "LOADER: env EAR_LOAD_MPI_VERSION  => %s", ear_getenv(FLAG_LOAD_MPI_VERSION));
    verbose(2, "LOADER: env HACK_MPI_VERSION      => %s", ear_getenv(HACK_MPI_VERSION));
    if (ear_getenv(FLAG_LOAD_MPI_VERSION) != NULL) {
        sprintf(buffer_version, "%s", ear_getenv(FLAG_LOAD_MPI_VERSION));
    } else if (ear_getenv(HACK_MPI_VERSION) != NULL) {
        sprintf(HACK_EXT, "%s.so", ear_getenv(HACK_MPI_VERSION));
        sprintf(buffer_version, "%s", "hack");
    } else if (call_mpi_version != NULL) {
        ((int (*)(char *, int *)) call_mpi_version)(buffer_version, &length);
    } else {
        verbose(2, "LOADER: MPI version can not be read, no library can be loaded");
        return NULL;
    }
    verbose(2, "LOADER: version string '%s'", buffer_version);
    // Passing to lowercase
    strtolow(buffer_version);
    // Comparing the string to get the version
    _found_intel   = append_extension(buffer_version, "intel"     ,       INTEL_EXT, buffer_path);
    _found_open    = append_extension(buffer_version, "open mpi"  ,     OPENMPI_EXT, buffer_path);
    _found_open   |= append_extension(buffer_version, "ompi"      ,     OPENMPI_EXT, buffer_path);
    _found_mvapich = append_extension(buffer_version, "mvapich"   ,       INTEL_EXT, buffer_path);
    _found_fujitsu = append_extension(buffer_version, "fujitsu"   , FUJITSU_MPI_EXT, buffer_path);
    _found_cray    = append_extension(buffer_version, "cray mpich",    CRAY_MPI_EXT, buffer_path);
    _found_hack    = append_extension(buffer_version, "hack"      ,        HACK_EXT, buffer_path);
    verbose(2, "LOADER: EAR library compatible with this MPI: %s", buffer_path);
    // Hack overrides normal path, its a complete absolute path to the library
    if (buffer_hack != NULL) {
        buffer_path = buffer_hack;
    }
    verbose(2, "LOADER: final EAR library to load: %s", buffer_path);
    return buffer_path;
}

static int load_ear_functions(void *handler)
{
    static int built_mpi_structure = 0;
    void (*libear_setnext_c)(mpic_t *) = NULL;
    void (*libear_setnext_f)(mpif_t *) = NULL;
    void **libmpi_c = (void **) &calls_mpi_c;
    void **libmpi_f = (void **) &calls_mpi_f;
    void **libear_c = (void **) &ear_mpic;
    void **libear_f = (void **) &ear_mpif;
    int i;

    // Already built MPI structure, we are not going to repeat.
    if (built_mpi_structure) {
        return 0;
    }
    // If not enough data to load libear.so, there is no chance to postpone this
    // call to get that data later, so we build the complete MPI structure now.
    if (handler == NULL) {
        goto override;
    }
    // Loading EAR "setnext" functions
    libear_setnext_c = dlsym(handler, "ear_mpic_setnext");
    libear_setnext_f = dlsym(handler, "ear_mpif_setnext");
    verbose(2, "LOADER: address ear_mpic_setnext: %p", libear_setnext_c);
    verbose(2, "LOADER: address ear_mpif_setnext: %p", libear_setnext_f);
    // Disabling Fortran symbols for intel
    if (_found_intel) libear_setnext_f = NULL;
    // Loading LIBEAR MPI functions
    if (libear_setnext_c != NULL) plug_join(handler, libear_c, ear_mpic_names, MPIC_N);
    if (libear_setnext_f != NULL) plug_join(handler, libear_f, ear_mpif_names, MPIF_N);
    verbose(2, "LOADER: address ear_mpic_Init: %p", ear_mpic.Init);
    verbose(2, "LOADER: address ear_mpif_init: %p", ear_mpif.init);
    // Loading LIBEAR constructor/destructor
    call_constructor = dlsym(handler, "ear_constructor");
    call_destructor  = dlsym(handler, "ear_destructor" );
override:
    // Overriding LIBEAR MPI functions by MPI functions in case doesn't exists
    for (i = 0; i < MPIC_N; ++i) if (libear_c[i] == NULL) libear_c[i] = libmpi_c[i];
    for (i = 0; i < MPIF_N; ++i) if (libear_f[i] == NULL) libear_f[i] = libmpi_f[i];
    // Passing MPI functions to LIBEAR MPI functions
    if (libear_setnext_c != NULL) libear_setnext_c((mpic_t *) libmpi_c);
    if (libear_setnext_f != NULL) libear_setnext_f((mpif_t *) libmpi_f);
    built_mpi_structure = 1;
    return 1;
}

static void usage_and_exit()
{
    char message[1024];
    snprintf(message, sizeof(message),
             "Error, you are using your MPI4Py application with EAR library and the %s env var is not set."
             "If you want to run your application with EAR set \"export %s = \"intel\"|\"open mpi\". "
             "Otherwise use \"--ear=off\" to disable the EAR library. \n",
             FLAG_LOAD_MPI_VERSION, FLAG_LOAD_MPI_VERSION);
    write(2, message, strlen(message));
    exit(1);
}

int load_mpi_and_ear(char *libmpi_path)
{
    void *call_mpi_version = NULL;
    char *libear_path = NULL;
    verbose(2, "LOADER: Entering loader (%s)", libmpi_path);
    // Load libcntd.so (COUNTDOWN) functions.
    if (!load_mpi_functions(get_libcnt_handler(), &calls_mpi_c, &calls_mpi_f)) {
        // Load libmpi.so functions. Protected against multiple calls.
        if (!load_mpi_functions(RTLD_NEXT, &calls_mpi_c, &calls_mpi_f)) {
            if (libmpi_path != NULL) {
                if (!load_mpi_functions(get_libmpi_handler(libmpi_path), &calls_mpi_c, &calls_mpi_f)) {
                    // Fail, because if no MPI symbols, we are in trouble.
                    usage_and_exit();
                }
            } else return 0;
        }
    }
    // Looking for PMPI_Get_library_version function
    if ((call_mpi_version = load_mpi_version(RTLD_DEFAULT)) == NULL) {
         call_mpi_version = load_mpi_version(get_libmpi_handler(libmpi_path));
    }
    MPI_Get_library_version_detected = (call_mpi_version != NULL);
    verbose(2, "LOADER: address of PMPI_Get_library_version: %p", call_mpi_version);
    // Building libear.$ext.so final path. Protected against multiple calls.
    libear_path = build_libear_path(call_mpi_version);
    // Loading libear.$ext.so. Protected against multiple calls.
    if (load_ear_functions(get_libear_handler(libear_path))) {
        // Registering destructor and calling constructor
        if (atexit(module_mpi_destructor) != 0) {
            verbose(2, "LOADER: cannot set exit function");
        }
        if (call_constructor != NULL) {
            call_constructor();
        }
    }
    return 1;
}

int module_mpi(char *lib_path, char *lib_hack_name)
{
    // This is the call to a principal function
    if (load_mpi_and_ear(NULL)) {
        // This if is obsolete design and must be deleted as soon as possible.
        if (module_cuda_is()) {
            ear_cuda_enabled();
        }
        return 1;
    }
    // If one of these variables is active, it means the user wants to load the
    // MPI symbols. By returning 1 we do not allow other EAR version to be loaded.
    if (ear_getenv(FLAG_LOAD_MPI) != NULL || ear_getenv(FLAG_LOAD_MPI_VERSION) != NULL) {
        return 1;
    }
    return 0;
}

void module_mpi_destructor()
{
    verbose(3, "LOADER: loading module mpi (destructor)");
    if (module_mpi_is_enabled()) {
        if (call_destructor != NULL) {
            call_destructor();
        }
    }
}
