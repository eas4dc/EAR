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
#include <common/output/verbose.h>
#include <common/utils/dtools.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <link.h>

#if DTOOLS_MALLOC
static void *(*next_malloc)(size_t);
#endif
#if DTOOLS_FREE
static void (*next_free)(void *);
#endif

static long address;

#if (DTOOLS_MALLOC || DTOOLS_FREE)
__attribute__((constructor)) void dtools_init()
{
#if DTOOLS_MALLOC
    if ((next_malloc = dlsym(RTLD_NEXT, "malloc")) == NULL) {
        sserror("when dlsym 'malloc': %s\n", dlerror());
    }
#endif
#if DTOOLS_FREE
    if ((next_free = dlsym(RTLD_NEXT, "free")) == NULL) {
        sserror("when dlsym 'free': %s\n", dlerror());
    }
#endif
}
#endif

void dtools_break()
{
    // nothing
}

void dtools_set_address(void *address_in)
{
    verbose(0, "DTOOLS: address set %p", address_in);
    address = (long) address_in;
}

#if DTOOLS_FREE
void free(void *ptr)
{
    long diff;
    if (next_free == NULL) {
        dtools_init();
    }
    if (ptr == NULL) {
        return;
    }
    if (address != 0L) {
        diff = address - (long) ptr;
        if (diff < 0L) {
            diff = -diff;
        }
        if (diff <= 4096) {
            verbose(0, "DTOOLS: found address %p, %lu bytes of difference", ptr, diff);
            dtools_break();
        }
    }
    next_free(ptr);
}
#endif

char *dtools_get_backtrace_library(char *buffer, int calls_count)
{
    void *callstack[32];
    Dl_info info;

    if (backtrace(callstack, 32) < calls_count + 1) {
        return NULL;
    }
    if (dladdr(callstack[calls_count + 1], &info) == 0) {
        return NULL;
    }
    if (strcmp(&info.dli_fname[strlen(info.dli_fname) - 3], ".so") != 0) {
        return NULL;
    }
    return strcpy(buffer, info.dli_fname);
}

typedef struct cb_data_s {
    char *library;
    int found;
} cb_data_t;

static int dtools_is_ldd_library_callback(struct dl_phdr_info *info, size_t size, void *data)
{
    cb_data_t *cb_data = (cb_data_t *) data;
    cb_data->found     = strstr(info->dlpi_name, cb_data->library) != NULL;
    return cb_data->found;
}

int dtools_is_ldd_library(char *library)
{
    cb_data_t cb_data;
    cb_data.found   = 0;
    cb_data.library = library;
    dl_iterate_phdr(dtools_is_ldd_library_callback, (void *) &cb_data);
    return cb_data.found;
}