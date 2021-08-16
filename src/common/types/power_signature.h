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

#ifndef _EAR_TYPES_POWER_SIGNATURE
#define _EAR_TYPES_POWER_SIGNATURE

#include <common/types/generic.h>
#include <common/config.h>

typedef struct power_signature
{
    double DC_power;
    double DRAM_power;
    double PCK_power;
    double EDP;
    double max_DC_power;
    double min_DC_power;
	double time;
    ulong avg_f;
    ulong def_f;
} power_signature_t;


// Function declarations

/** Replicates the power_signature in *source to *destiny */
void copy_power_signature(power_signature_t *destiny, power_signature_t *source);

/** Initializes all values of the power_signature to 0.*/
void init_power_signature(power_signature_t *sig);

/** returns true if basic values for sig1 and sig2 are equal with a maximum %
*   of difference defined by threshold (th) */
uint are_equal_power_sig(power_signature_t *sig1,power_signature_t *sig2,double th);

/** Outputs the power_signature contents to the file pointed by the fd. */
void print_power_signature_fd(int fd, power_signature_t *sig);

void clean_db_power_signature(power_signature_t *ps, double limit);

#endif
