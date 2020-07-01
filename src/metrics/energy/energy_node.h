/**************************************************************
 * *	Energy Aware Runtime (EAR)
 * *	This program is part of the Energy Aware Runtime (EAR).
 * *
 * *	EAR provides a dynamic, transparent and ligth-weigth solution for
 * *	Energy management.
 * *
 * *    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
 * *
 * *       Copyright (C) 2017
 * *	BSC Contact 	mailto:ear-support@bsc.es
 * *	Lenovo contact 	mailto:hpchelp@lenovo.com
 * *
 * *	EAR is free software; you can redistribute it and/or
 * *	modify it under the terms of the GNU Lesser General Public
 * *	License as published by the Free Software Foundation; either
 * *	version 2.1 of the License, or (at your option) any later version.
 * *
 * *	EAR is distributed in the hope that it will be useful,
 * *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * *	Lesser General Public License for more details.
 * *
 * *	You should have received a copy of the GNU Lesser General Public
 * *	License along with EAR; if not, write to the Free Software
 * *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * *	The GNU LEsser General Public License is contained in the file COPYING
 * */

#ifndef METRICS_ENERGY_NODE_H
#define METRICS_ENERGY_NODE_H

#include <common/includes.h>
#include <common/system/time.h>
#include <metrics/accumulators/types.h>
#include <common/types/configuration/cluster_conf.h>

state_t energy_init(cluster_conf_t *conf, ehandler_t *eh);

state_t energy_dispose(ehandler_t *eh);

state_t energy_handler_clean(ehandler_t *eh);

state_t energy_datasize(ehandler_t *eh, size_t *size);

state_t energy_frequency(ehandler_t *eh, ulong *freq_us);

state_t energy_dc_read(ehandler_t *eh, edata_t  energy_mj);

state_t energy_dc_time_read(ehandler_t *eh, edata_t energy_mj, ulong *time_ms);

state_t energy_ac_read(ehandler_t *eh, edata_t energy_mj);

/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_units(ehandler_t *eh,uint *units);

state_t energy_accumulated(ehandler_t *eh,unsigned long *e,edata_t init,edata_t end);

state_t energy_to_str(ehandler_t *eh,char *str,edata_t e);


#endif //EAR_ENERGY_H

