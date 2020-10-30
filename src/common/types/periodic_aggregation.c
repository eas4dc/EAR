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

#include <string.h>
#include <common/types/periodic_aggregation.h>

void init_periodic_aggregation(peraggr_t *aggr, char *hostname)
{
	memset(aggr, 0, sizeof(peraggr_t));
	memcpy(aggr->eardbd_host, hostname, sizeof(aggr->eardbd_host));
}

void add_periodic_aggregation(peraggr_t *aggr, ulong DC_energy, time_t start_time, time_t end_time)
{
	aggr->DC_energy += DC_energy;
	aggr->n_samples += 1;

	if (end_time > aggr->end_time) {
		aggr->end_time = end_time;
	}

	if (start_time < aggr->start_time) {
		aggr->start_time = start_time;
	}

	if (aggr->start_time == 0) {
		aggr->start_time = start_time;
	}
}
