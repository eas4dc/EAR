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

#ifndef _EAR_TYPES_COEFFICIENT
#define _EAR_TYPES_COEFFICIENT

#include <common/config.h>
#include <common/types/generic.h>

typedef struct coefficient
{
	ulong pstate_ref;	
    ulong pstate;
    uint available;
    /* For power projection */
    double A;
    double B;
    double C;
    /* For CPI projection */
    double D;
    double E;
    double F;
} coefficient_t;

// File
int coeff_file_size(char *path);

int coeff_file_read(char *path, coefficient_t **coeffs);

int coeff_file_read_no_alloc(char *path, coefficient_t *coeffs, int size);

// Misc
void coeff_reset(coefficient_t *coeff);

void coeff_print(coefficient_t *coeff);

#endif
