/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

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
