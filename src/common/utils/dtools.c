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
#include <limits.h>
#include <signal.h>
#include <common/types.h>
#include <common/states.h>
#include <common/utils/dtools.h>
#include <common/output/verbose.h>

#if DTOOLS_MALLOC
static void *(*next_malloc) (size_t);
#endif
#if DTOOLS_FREE
static void  (*next_free)   (void *);
#endif

static long address;

#if (DTOOLS_MALLOC || DTOOLS_FREE)
__attribute__ ((constructor)) void dtools_init()
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
    //nothing
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
    if(next_free == NULL) {
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
