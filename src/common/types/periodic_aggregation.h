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

#ifndef EAR_PERIODIC_AGGREGATION_H
#define EAR_PERIODIC_AGGREGATION_H

#include <time.h>
#include <common/types/generic.h>

typedef struct periodic_aggregation {
	ulong DC_energy;
	time_t start_time;
	time_t end_time;
	uint n_samples;
	uint id_isle;
	char eardbd_host[64];
} periodic_aggregation_t;

typedef periodic_aggregation_t peraggr_t;

// Functions
void init_periodic_aggregation(peraggr_t *aggr, char *hostname);
void add_periodic_aggregation(peraggr_t *aggr, ulong DC_energy, time_t start_time, time_t end_time);


#endif //EAR_PERIODIC_AGGREGATION_H
