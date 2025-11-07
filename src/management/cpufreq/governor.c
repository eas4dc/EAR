/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/plugins.h>
#include <management/cpufreq/governor.h>

state_t governor_tostr(uint governor, char *buffer)
{
    if (governor == Governor.conservative) {
        sprintf(buffer, "%s", Goverstr.conservative);
    } else if (governor == Governor.performance) {
        sprintf(buffer, "%s", Goverstr.performance);
    } else if (governor == Governor.userspace) {
        sprintf(buffer, "%s", Goverstr.userspace);
    } else if (governor == Governor.powersave) {
        sprintf(buffer, "%s", Goverstr.powersave);
    } else if (governor == Governor.ondemand) {
        sprintf(buffer, "%s", Goverstr.ondemand);
    } else {
        sprintf(buffer, "%s", Goverstr.unknown);
        return_msg(EAR_ERROR, "undefined governor");
    }
    return EAR_SUCCESS;
}

state_t governor_toint(char *buffer, uint *governor)
{
    if (strncmp(buffer, Goverstr.conservative, 12) == 0) {
        *governor = Governor.conservative;
    } else if (strncmp(buffer, Goverstr.performance, 11) == 0) {
        *governor = Governor.performance;
    } else if (strncmp(buffer, Goverstr.userspace, 9) == 0) {
        *governor = Governor.userspace;
    } else if (strncmp(buffer, Goverstr.powersave, 9) == 0) {
        *governor = Governor.powersave;
    } else if (strncmp(buffer, Goverstr.ondemand, 8) == 0) {
        *governor = Governor.ondemand;
    } else {
        *governor = Governor.unknown;
        return_msg(EAR_ERROR, "undefined governor");
    }
    return EAR_SUCCESS;
}

int governor_is(char *buffer, uint governor)
{
    uint aux;
    if (state_fail(mgt_governor_toint(buffer, &aux))) {
        return 0;
    }
    return (aux == governor);
}
