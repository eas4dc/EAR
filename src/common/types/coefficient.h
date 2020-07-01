/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
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
