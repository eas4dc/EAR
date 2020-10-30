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
