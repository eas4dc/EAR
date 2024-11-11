/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
void periodic_aggregation_clean_before_db(periodic_aggregation_t *pa)
{
	if (pa->DC_energy > INT_MAX) pa->DC_energy = INT_MAX;
	if (pa->n_samples > INT_MAX) pa->n_samples = INT_MAX;
	if (pa->id_isle   > INT_MAX) pa->id_isle   = INT_MAX;
}
