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

#include <metrics/energy/energy_gpu.h>
#include <metrics/energy/gpu/nvsmi.h>

struct energy_gpu_ops
{
	state_t (*init)			(pcontext_t *c, uint loop_ms);
	state_t (*dispose)		(pcontext_t *c);
	state_t (*count)		(pcontext_t *c, uint *count);
	state_t (*read)			(pcontext_t *c, gpu_energy_t  *dr);
	state_t (*data_alloc)	(pcontext_t *c, gpu_energy_t **dr);
	state_t (*data_free)	(pcontext_t *c, gpu_energy_t **dr);
	state_t (*data_null)	(pcontext_t *c, gpu_energy_t  *dr);
	state_t (*data_diff)	(pcontext_t *c, gpu_energy_t  *dr1, gpu_energy_t *dr2, gpu_energy_t *da);
	state_t (*data_copy)	(pcontext_t *c, gpu_energy_t  *dst, gpu_energy_t *src);
} ops;

state_t energy_gpu_init(pcontext_t *c, uint loop_ms)
{
	if (state_ok(nvsmi_gpu_status())) {
		ops.init		= nvsmi_gpu_init;
		ops.dispose		= nvsmi_gpu_dispose;
		ops.read		= nvsmi_gpu_read;
		ops.count		= nvsmi_gpu_count;
		ops.data_alloc	= nvsmi_gpu_data_alloc;
		ops.data_free	= nvsmi_gpu_data_free;
		ops.data_null	= nvsmi_gpu_data_null;
		ops.data_diff	= nvsmi_gpu_data_diff;
		ops.data_copy   = NULL;
		return ops.init(c, loop_ms);
	} else {
		state_return_msg(EAR_INCOMPATIBLE, 0, "no energy GPU API available");
	}
}

state_t energy_gpu_dispose(pcontext_t *c)
{
	preturn(ops.dispose, c);
}

state_t energy_gpu_read(pcontext_t *c, gpu_energy_t *data_read)
{
	preturn(ops.read, c, data_read);
}

state_t energy_gpu_count(pcontext_t *c, uint *count)
{
	preturn(ops.count, c, count);
}

state_t energy_gpu_data_alloc(pcontext_t *c, gpu_energy_t **data_read)
{
	preturn(ops.data_alloc, c, data_read);
}

state_t energy_gpu_data_free(pcontext_t *c, gpu_energy_t **data_read)
{
	preturn(ops.data_free, c, data_read);
}

state_t energy_gpu_data_null(pcontext_t *c, gpu_energy_t *data_read)
{
	preturn(ops.data_null, c, data_read);
}

state_t energy_gpu_data_diff(pcontext_t *c, gpu_energy_t *data_read1, gpu_energy_t *data_read2, gpu_energy_t *data_avrg)
{
	preturn(ops.data_diff, c, data_read1, data_read2, data_avrg);
}

state_t energy_gpu_data_copy(pcontext_t *c, gpu_energy_t *data_dst, gpu_energy_t *data_src)
{
	return EAR_SUCCESS;
}
