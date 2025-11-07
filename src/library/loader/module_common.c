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
#include <common/config.h>
#include <common/sizes.h>
#include <common/utils/dtools.h>
#include <common/utils/string.h>
#include <dlfcn.h>
#include <library/loader/module_common.h>
#include <stdlib.h>
#include <unistd.h>

static void (*call_constructor)(void);
static void (*call_destructor)(void);
static int libear_already_loaded;

// Deprecated, clean.
int is_cuda_enabled()
{
    return 1;
}

// Non-deprecated.
int is_libear_loaded()
{
    return libear_already_loaded;
}

void set_libear_loaded()
{
    libear_already_loaded = 1;
}

static int syms_present(char *syms[])
{
    while (*syms != NULL) {
        if (dlsym(RTLD_DEFAULT, *syms) != NULL) {
            verbose(2, "LOADER: looking for symbol '%s': found!", *syms);
            return 1;
        }
        verbose(2, "LOADER: looking for symbol '%s': not found", *syms);
        ++syms;
    }
    return 0;
}

static int libs_present(char *libs[])
{
    while (*libs != NULL) {
        if (dtools_is_ldd_library(*libs)) {
            verbose(2, "LOADER: looking for library '%s': found!", *libs);
            return 1;
        }
        verbose(2, "LOADER: looking for library '%s': not found", *libs);
        ++libs;
    }
    return 0;
}

static int module_file_exists(char *path)
{
    return (access(path, X_OK) == 0);
}

void loader_get_path_libear(char **path_lib, char **hack_lib)
{
    static char path_lib_so[4096];
    char *env_hack = NULL;
    char *env_path = NULL;

    *path_lib = NULL;
    *hack_lib = NULL;
    // HACK_EARL_INSTALL_PATH is a hack for the whole library path, includes "lib" in the path
    env_path = ear_getenv(HACK_EARL_INSTALL_PATH);
    env_hack = ear_getenv(HACK_LIBRARY_FILE);

    if (env_path == NULL) {
        verbose(2, "HACK not defined %s", HACK_EARL_INSTALL_PATH);
        // ENV_PATH_EAR is the official installation path, doesn't include "lib"
        if ((env_path = ear_getenv(ENV_PATH_EAR)) == NULL) {
            // This last hack is to load a specific library with the whole path including library name
            if (env_hack == NULL) {
                verbose(2, "LOADER: neither installation path exists nor HACK library defined");
                return;
            }
        } else {
            snprintf(path_lib_so, sizeof(path_lib_so), "%s/%s", env_path, REL_PATH_LIBR);
            *path_lib = malloc(strlen(path_lib_so) + SZ_FILENAME);
            strcpy(*path_lib, path_lib_so);
        }
    } else {
        *path_lib = malloc(strlen(env_path) + SZ_FILENAME);
        strcpy(*path_lib, env_path);
    }
    verbose(4, "LOADER: path_lib_so %s", *path_lib);
    if (env_hack != NULL) {
        *hack_lib = malloc(strlen(env_hack) + SZ_FILENAME);
        strcpy(*hack_lib, env_hack);
        verbose(4, "LOADER: hack file %s", *hack_lib);
    }
    return;
}

static void module_other_destructor()
{
    if (call_destructor != NULL) {
        call_destructor();
    }
}

int load_libear_nonspec(char *lib_extension, char *syms[], char *libs[])
{
    char *buffer_path = NULL;
    char *buffer_hack = NULL;
    void *libear      = NULL;
    char path[4096];

    if (syms != NULL) {
        if (!syms_present(syms)) {
            return 0;
        }
    }
    if (libs != NULL) {
        if (!libs_present(libs)) {
            return 0;
        }
    }
    // Computing library path
    loader_get_path_libear(&buffer_path, &buffer_hack);
    verbose(2, "LOADER: env HACK/EAR_INSTALL_PATH => %s", buffer_path);
    verbose(2, "LOADER: env HACK_LIBRARY_FILE     => %s", buffer_hack);
    if (buffer_path == NULL && buffer_hack == NULL) {
        return 0;
    }
    if (buffer_hack == NULL) {
        xsnprintf(path, sizeof(path), "%s/%s.%s", buffer_path, REL_NAME_LIBR, lib_extension);
    } else {
        xsnprintf(path, sizeof(path), "%s", buffer_hack);
    }
    if (!module_file_exists(path)) {
        return 0;
    }
    // Loading library
    verbose(2, "LOADER: loading library '%s'", path);
    if ((libear = dlopen(path, RTLD_GLOBAL | RTLD_NOW)) == NULL) {
        verbose(2, "LOADER: library %s failed error (%s)", path, dlerror());
        return 0;
    }
    verbose(2, "LOADER: library %s successfuly loaded", path);
    // Announce libear is already loaded
    set_libear_loaded();
    // Constructor function call
    call_constructor = dlsym(libear, "ear_constructor");
    call_destructor  = dlsym(libear, "ear_destructor");
    if (atexit(module_other_destructor) != 0) {
        verbose(2, "LOADER: cannot set exit function");
    }
    // module_other_constructor()
    if (call_constructor != NULL) {
        call_constructor();
    }
    return 1;
}
