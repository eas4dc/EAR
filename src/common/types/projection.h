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

/**
*    \file projection.h
*    \brief Projections are used by time&power models.
*
*/

#ifndef _EAR_TYPES_PROJECTION
#define _EAR_TYPES_PROJECTION

#include <common/config.h>
#include <common/types/signature.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/coefficient.h>
#include <common/hardware/architecture.h>

typedef struct projection
{
	double Time;
	double Power;
} projection_t;

state_t projections_init(uint user_type,conf_install_t *data,architecture_t *myarch);

// Projections

state_t project_time(signature_t *sign, ulong from,ulong to,double *ptime);

state_t project_power(signature_t *sign, ulong from,ulong to,double *ppower);

state_t projection_available(ulong from,ulong to);

// Inherited
/** Allocates memory to contain the projections for the p_states given by
*   parameter */
void projection_create(uint p_states);

/** Sets the values of the performance projection i to the ones given by parameter */
void projection_set(int i, double TP, double PP);

/** Resets the projections for power, CPI and time to 0 */
void projection_reset(uint p_states);

/** Given a frequency f, returns the projection of the associated p_state of said
*   frequency */
projection_t *projection_get(int pstate);

/* Basic for compatibility and tools */
double basic_project_time(signature_t *sign,coefficient_t *coeff);
double basic_project_cpi(signature_t *sign,coefficient_t *coeff);
double basic_project_power(signature_t *sign,coefficient_t *coeff);


#endif
