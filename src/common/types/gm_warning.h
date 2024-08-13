/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/config.h>

#ifndef _EAR_TYPES_GM_WARNING
#define _EAR_TYPES_GM_WARNING

#include <common/types/generic.h>
#define NODE_SIZE 256

typedef struct gm_warning 
{
    ulong level;
    ulong new_p_state;
    double energy_percent;
    double inc_th;
    ulong energy_t1;
    ulong energy_t2;
    ulong energy_limit;
    ulong energy_p1;
    ulong energy_p2;
    char policy[64];
} gm_warning_t;


// Function declarations

/** Replicates the periodic_metric in *source to *destiny */
void copy_gm_warning(gm_warning_t *destiny, gm_warning_t *source);

/** Initializes all values of the periodic_metric to 0 , sets the nodename */
void init_gm_warning(gm_warning_t *pm);


#endif
