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

#ifndef METRICS_ENERGY_NVML_GPU_H
#define METRICS_ENERGY_NVML_GPU_H

#include <metrics/energy/energy_gpu.h>

/** **/
state_t nvml_status();

/** **/
state_t nvml_init(pcontext_t *c);

/** **/
state_t nvsmi_dispose(pcontext_t *c);

/** Counts the number of GPUs. **/
state_t nvml_count(pcontext_t *c, uint *count);

/** **/
state_t nvml_read(pcontext_t *c, gpu_energy_t *data_read);

/** **/
state_t nvml_data_alloc(pcontext_t *c, gpu_energy_t **data_read);

/** **/
state_t nvml_data_free(pcontext_t *c, gpu_energy_t **data_read);

/** **/
state_t nvml_data_null(pcontext_t *c, gpu_energy_t *data_read);

/** **/
state_t nvml_data_diff(pcontext_t *c, gpu_energy_t *data_read1, gpu_energy_t *data_read2, gpu_energy_t *data_avrg);

#endif // METRICS_ENERGY_NVML_GPU_H