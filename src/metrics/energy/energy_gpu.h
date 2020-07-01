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

#ifndef METRICS_ENERGY_GPU_H
#define METRICS_ENERGY_GPU_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/types/generic.h>

typedef struct gpu_power_s
{
	timestamp_t time;
	ulong samples;
	ulong freq_gpu_mhz;
	ulong freq_mem_mhz;
	ulong util_gpu; // percent
	ulong util_mem; // percent
	ulong temp_gpu; // celsius
	ulong temp_mem; // celsius
	double energy_j;
	double power_w;
	uint correct;
} gpu_energy_t;

state_t energy_gpu_init(pcontext_t *c, uint loop_ms);

state_t energy_gpu_dispose(pcontext_t *c);

state_t energy_gpu_count(pcontext_t *c, uint *count);

state_t energy_gpu_read(pcontext_t *c, gpu_energy_t *data_read);

state_t energy_gpu_data_alloc(pcontext_t *c, gpu_energy_t **data_read);

state_t energy_gpu_data_free(pcontext_t *c, gpu_energy_t **data_read);

state_t energy_gpu_data_diff(pcontext_t *c, gpu_energy_t *data_read1, gpu_energy_t *data_read2, gpu_energy_t *data_avrg);

state_t energy_gpu_data_null(pcontext_t *c, gpu_energy_t *data_read);

state_t energy_gpu_data_copy(pcontext_t *c, gpu_energy_t *data_dst, gpu_energy_t *data_src);

#endif
