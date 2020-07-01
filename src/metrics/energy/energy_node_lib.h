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

#ifndef EAR_ENERGY_H
#define EAR_ENERGY_H

#include <common/includes.h>
#include <daemon/shared_configuration.h>
#include <metrics/accumulators/types.h>

state_t energy_lib_init(settings_conf_t *conf);

state_t energy_lib_datasize(size_t *size);

state_t energy_lib_frequency(ulong *freq_us);

/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_lib_units(uint *units);

state_t energy_lib_accumulated(unsigned long *e,edata_t init,edata_t end);

//
state_t energy_lib_to_str(char *str,edata_t e);

#endif //EAR_ENERGY_H
