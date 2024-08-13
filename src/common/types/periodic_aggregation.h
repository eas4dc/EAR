/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
