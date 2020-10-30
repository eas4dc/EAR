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

#include <metrics/gpu/dummy/gpu_dummy.h>





state_t gpu_dummy_data_print(gpu_t *data, int fd)
{
	int i;

	for (i = 0; i < dev_count; ++i)
	{
		dprintf(fd, "gpu%u: %0.2lfJ, %0.2lfW, %luMHz, %luMHz, %lu%%, %lu%%, %luº, %luº, %u, %lu\n",
		i                   ,
		data[i].energy_j    , data[i].power_w,
		data[i].freq_gpu_mhz, data[i].freq_mem_mhz,
		data[i].util_gpu    , data[i].util_mem,
		data[i].temp_gpu    , data[i].temp_mem,
		data[i].working     , data[i].samples);
	}
	
	return EAR_SUCCESS;
}

state_t gpu_dummy_data_tostr(gpu_t *data, char *buffer, int length)
{
	int accuml = 0;
	size_t s;
	int i;

	for (i = 0; i < dev_count && length > 0; ++i)
	{
		s = snprintf(&buffer[accuml], length,
			"gpu%u: %0.2lfJ, %0.2lfW, %luMHz, %luMHz, %lu%%, %lu%%, %luº, %luº, %u, %lu\n",
			i                   ,
			data[i].energy_j    , data[i].power_w,
			data[i].freq_gpu_mhz, data[i].freq_mem_mhz,
			data[i].util_gpu    , data[i].util_mem,
			data[i].temp_gpu    , data[i].temp_mem,
			data[i].working     , data[i].samples);
		length = length - s;
		accuml = accuml + s;
	}
	return EAR_SUCCESS;
}


state_t gpu_dummy_status() { return EAR_SUCCESS; }
state_t gpu_dummy_init(ctx_t *c) { return EAR_EAR_SUCCESS; }
state_t gpu_dummy_dispose(ctx_t *c) { return EAR_SUCCESS; }
state_t gpu_dummy_count(ctx_t *c, uint *_dev_count) { *_dev_count=1;return EAR_SUCCESS; }
state_t gpu_dummy_read(ctx_t *c, gpu_t *data) {gpu_dummy_data_null(data); return EAR_SUCCESS; }
state_t gpu_dummy_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff) { return EAR_ERROR; }
state_t gpu_dummy_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff) { return EAR_ERROR; }
state_t gpu_dummy_data_init(uint _dev_count) { return EAR_ERROR; }
state_t gpu_dummy_data_alloc(gpu_t **data) { return EAR_ERROR; }
state_t gpu_dummy_data_free(gpu_t **data) { return EAR_ERROR; }
state_t gpu_dummy_data_null(gpu_t *data) { return EAR_ERROR; }
state_t gpu_dummy_data_copy(gpu_t *data_dst, gpu_t *data_src) { return EAR_ERROR; }
state_t gpu_dummy_data_print(gpu_t *data, int fd) { return EAR_ERROR; }
state_t gpu_dummy_data_tostr(gpu_t *data, char *buffer, int length) { return EAR_ERROR; }

