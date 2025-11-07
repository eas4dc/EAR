/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

// #define SHOW_DEBUGS 1
#define VCRAY_PM_VL 2

#include <common/math_operations.h>
#include <common/output/verbose.h>
#include <common/states.h> //clean
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <metrics/common/cray_pm_counters.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * ENERGY NODE API
 */
#include <common/system/time.h>
#include <metrics/energy/node/energy_node.h>

static uint cray_pm_energy_init = 0;
static pthread_mutex_t elock    = PTHREAD_MUTEX_INITIALIZER;

/*
 * MAIN FUNCTIONS
 */

static state_t static_energy_init()
{
    state_t st = EAR_SUCCESS;

    while (pthread_mutex_trylock(&elock)) {
    };
    if (cray_pm_energy_init) {
        pthread_mutex_unlock(&elock);
        return st;
    }
    verbose(VCRAY_PM_VL, "CRAY PM: init");
    if (cray_pm_init() != EAR_SUCCESS) {
        pthread_mutex_unlock(&elock);
        return EAR_ERROR;
    }
    cray_pm_energy_init = 1;
    pthread_mutex_unlock(&elock);
    return st;
}

state_t energy_init(void **c)
{
    return static_energy_init();
}

state_t energy_dispose(void **c)
{
    verbose(VCRAY_PM_VL, "CRAY PM: dispose");
    if (cray_pm_energy_init)
        cray_pm_dispose();
    return EAR_SUCCESS;
}

state_t energy_datasize(size_t *size)
{
    verbose(VCRAY_PM_VL, "CRAY PM: datasize");
    debug("energy_datasize %lu\n", sizeof(unsigned long));
    *size = sizeof(unsigned long);
    return EAR_SUCCESS;
}

state_t energy_frequency(ulong *freq_us)
{
    verbose(VCRAY_PM_VL, "CRAY PM: freq");
    *freq_us = 1000000;
    return EAR_SUCCESS;
}

state_t energy_to_str(char *str, edata_t e)
{
    ulong *pe = (ulong *) e;
    sprintf(str, "%lu", *pe);
    return EAR_SUCCESS;
}

static unsigned long gh_diff_node_energy(ulong init, ulong end)
{
    ulong ret = 0;
    if (end > init) {
        ret = end - init;
    } else {
        ret = ulong_diff_overflow(init, end);
    }
    return ret;
}

state_t energy_units(uint *units)
{
    verbose(VCRAY_PM_VL, "CRAY PM: units");
    *units = 1000;
    return EAR_SUCCESS;
}

state_t energy_accumulated(unsigned long *e, edata_t init, edata_t end)
{

    verbose(VCRAY_PM_VL, "CRAY PM: energy accumulated");
    ulong *pinit = (ulong *) init, *pend = (ulong *) end;

    ulong total = gh_diff_node_energy(*pinit, *pend);
    *e          = total;
    return EAR_SUCCESS;
}

/* TODO: These two functions are pending to use a lock and update the energy since the last reading */
state_t energy_dc_read(void *c, edata_t energy_mj)
{
    verbose(VCRAY_PM_VL, "CRAY PM: energy read");
    state_t st = EAR_ERROR;
    if (!cray_pm_energy_init)
        return st;

    ulong energy;
    ulong *penergy_mj = (ulong *) energy_mj;

    // TODO: I don't know why we should lock
    // while (pthread_mutex_trylock(&elock)){};

    if (state_ok(cray_pm_get("energy", &energy))) {
        energy      = energy * 1000;
        *penergy_mj = energy;
        st          = EAR_SUCCESS;
    }

    // pthread_mutex_unlock(&elock);

    return st;
}

state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms)
{
    verbose(VCRAY_PM_VL, "CRAY PM: energy & time read");
    state_t st = EAR_ERROR;
    if (!cray_pm_energy_init)
        return st;

    ulong *penergy_mj = (ulong *) energy_mj;
    ulong energy;

    // TODO: I don't know why we should lock
    // while (pthread_mutex_trylock(&elock)){};

    if (state_ok(cray_pm_get("energy", &energy))) {
        energy      = energy * 1000;
        *penergy_mj = energy;
        st          = EAR_SUCCESS;
    }

    // pthread_mutex_unlock(&elock);

    debug("energy_dc_read: %lu \n", *penergy_mj);
    *time_ms = (ulong) time(NULL) * 1000;

    return st;
}

uint energy_data_is_null(edata_t e)
{
    verbose(VCRAY_PM_VL, "CRAY PM: data is null");
    ulong *pe = (ulong *) e;
    return (*pe == 0);
}

state_t energy_not_privileged_init()
{
    return static_energy_init();
}

uint energy_is_privileged()
{
    // This function needs more robustness.
    // We should call the static read and check state
    return 0;
}
