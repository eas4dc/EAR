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
