/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/states.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

state_t loadavg(float *min, float *Vmin, float *XVmin, uint *runnable, uint *total, uint *lastpid)
{
    FILE *f;
    f = fopen("/proc/loadavg", "r");
    if (f == NULL) {
        return EAR_ERROR;
    }
    if (fscanf(f, "%f %f %f %u/%u %u", min, Vmin, XVmin, runnable, total, lastpid)) {
        // Just for warning purposes
    }
    fclose(f);
    return EAR_SUCCESS;
}
