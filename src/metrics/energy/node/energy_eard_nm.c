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
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <common/output/verbose.h>
#include <common/states.h>
#include <ear.h>
#include <metrics/energy/node/energy_node.h>

static pthread_mutex_t eard_nm_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * MAIN FUNCTIONS
 */
state_t energy_not_privileged_init()
{
    state_t ret;
    pthread_mutex_lock(&eard_nm_lock);
    debug("EARD NM: connect. Using tmp %s", EAR_NM_TMP);
    char *curr_tmp;
    // Save
    curr_tmp = getenv("EAR_TMP");

    // Connect
    setenv("EAR_TMP", EAR_NM_TMP, 1);
    ret = ear_connect();
    // Restore
    if (curr_tmp != NULL)
        setenv("EAR_TMP", curr_tmp, 1);
    debug("EARD NM: connection status %s", ((ret == EAR_SUCCESS) ? "Connected" : "Not connected"));
    pthread_mutex_unlock(&eard_nm_lock);
    return ret;
}

void energy_dispose_not_priv()
{
    debug("EARD NM: Disconnecting");
    pthread_mutex_lock(&eard_nm_lock);
    ear_disconnect();
    pthread_mutex_unlock(&eard_nm_lock);
    return;
}

state_t energy_datasize(size_t *size)
{
    debug("EARD NM: datasize");
    *size = sizeof(unsigned long);
    return EAR_SUCCESS;
}

state_t energy_frequency(ulong *freq_us)
{
    debug("EARD NM: frequency");
    *freq_us = 1000000;
    return EAR_SUCCESS;
}

state_t energy_dc_read(void *c, edata_t energy_mj)
{
    unsigned long *pvalues = energy_mj;
    unsigned long t_ms_init;
    debug("EARD NM: Energy read");
    *pvalues = 0;
    return ear_energy(pvalues, &t_ms_init);
}

state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms)
{
    unsigned long *pvalues = energy_mj;
    *pvalues               = 0;
    debug("EARD NM: Energy read & time");
    return ear_energy(pvalues, time_ms);
}

state_t energy_ac_read(void *c, edata_t energy_mj)
{

    return EAR_SUCCESS;
}

state_t energy_units(uint *units)
{
    debug("EARD NM: units");
    *units = 1000;
    return EAR_SUCCESS;
}

state_t energy_accumulated(unsigned long *e, edata_t init, edata_t end)
{
    ulong *pinit = (ulong *) init;
    ulong *pend  = (ulong *) end;
    ulong tinit = 0, tend = 0, tdiff;
    ear_energy_diff(*pinit, *pend, e, tinit, tend, &tdiff);
    debug("EARD NM: energy diff: init %lu end %lu diff %lu", *pinit, *pend, *e);

    return EAR_SUCCESS;
}

state_t energy_to_str(char *str, edata_t e)
{
    ulong *ep = (ulong *) e;
    if (ep == NULL)
        return EAR_ERROR;
    sprintf(str, "%lu", *ep);
    return EAR_SUCCESS;
}

uint energy_data_is_null(edata_t e)
{
    ulong *ep = (ulong *) e;
    if (ep == NULL)
        return 1;
    return (*ep == 0);
}

state_t data_copy(edata_t dest, edata_t src)
{
    ulong *ed = (ulong *) dest;
    ulong *es = (ulong *) src;
    *ed       = *es;
    return EAR_SUCCESS;
}

uint energy_is_privileged()
{
    return 0;
}
