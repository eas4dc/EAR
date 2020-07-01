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
