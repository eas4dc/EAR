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

#include <stdlib.h>
#include <common/output/debug.h>
#include <metrics/gpu/archs/dummy.h>

static struct error_s {
	char *null_data;
} Error = {
	.null_data    = "data pointer is NULL",
};

state_t gpu_dummy_status()
{
	return 1;
}

state_t gpu_dummy_init(ctx_t *c)
{
	debug("gpu_dummy_init");
	return EAR_SUCCESS;
}

state_t gpu_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t gpu_dummy_count(ctx_t *c, uint *_dev_count)
{
	debug("count=1");
	if (_dev_count != NULL) {
		*_dev_count = 1;
	}
	return EAR_SUCCESS;
}

state_t gpu_dummy_get_info(ctx_t *c, const gpu_info_t **info_in, uint *dev_count_in)
{
    static gpu_info_t info;

	memset(&info, 0, sizeof(gpu_info_t)); 
	*info_in = &info;

    if (dev_count_in != NULL) {
		*dev_count_in = 1;
	}

    return EAR_SUCCESS;
}

state_t gpu_dummy_read(ctx_t *c, gpu_t *data)
{
	debug("read is null");
	return gpu_dummy_data_null(data);
}

state_t gpu_dummy_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	state_t s;
	if (xtate_fail(s, gpu_dummy_read(c, data2))) {
		return s;
	}
	if (xtate_fail(s, gpu_dummy_data_diff(data2, data1, data_diff))) {
		return s;
	}
	return gpu_dummy_data_copy(data1, data2);
}

state_t gpu_dummy_read_raw(ctx_t *c, gpu_t *data)
{
	return gpu_dummy_data_null(data);
}

state_t gpu_dummy_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	if (data2 == NULL || data1 == NULL || data_diff == NULL) {
		debug("some pointer in data_diff is null");
		return_msg(EAR_ERROR, Error.null_data);
	}
	return gpu_dummy_data_null(data_diff);
}

state_t gpu_dummy_data_merge(gpu_t *data_diff, gpu_t *data_merge)
{
	if (data_diff == NULL || data_merge == NULL) {
		debug("some pointer in data_merge is null");
		return_msg(EAR_ERROR, Error.null_data);
	}
	return gpu_dummy_data_null(data_merge);
}

state_t gpu_dummy_data_alloc(gpu_t **data)
{
	if (data == NULL) {
		debug("Error data pointer in alloc is null");
		return_msg(EAR_ERROR, Error.null_data);
	}
	*data = calloc(1, sizeof(gpu_t));
	if (*data == NULL) {
		return_msg(EAR_SYSCALL_ERROR, strerror(errno));
	}
	debug("Data allocated");
	return EAR_SUCCESS;
}

state_t gpu_dummy_data_free(gpu_t **data)
{
	if (data != NULL) {
		free(*data);
	}
	return EAR_SUCCESS;
}

state_t gpu_dummy_data_null(gpu_t *data)
{
	if (data == NULL) {
		return_msg(EAR_ERROR, Error.null_data);
	}
	memset(data, 0, 1 * sizeof(gpu_t));
	return EAR_SUCCESS;
}

state_t gpu_dummy_data_copy(gpu_t *data_dst, gpu_t *data_src)
{
	return EAR_SUCCESS;
}

state_t gpu_dummy_data_print(gpu_t *data, int fd)
{
	int i;
	if (data == NULL){ 
		debug("data is null");
		return_msg(EAR_ERROR, Error.null_data);
	}
	for (i = 0; i < 1; ++i)
	{
		dprintf(fd, "gpu%u: %0.2lfJ, %0.2lfW, %luMHz, %luMHz, %lu%%, %lu%%, %luº, %luº, %u, %lu\n",
		i                   ,
		data[i].energy_j    , data[i].power_w,
		data[i].freq_gpu    , data[i].freq_mem,
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
	if ((data == NULL) || (buffer == NULL)){ 
		debug("data or buffer are null");
		return_msg(EAR_ERROR, Error.null_data);
	}
	for (i = 0; i < 1 && length > 0; ++i)
	{
		
		s = snprintf(&buffer[accuml], length,
			"gpu%u: %.2lfJ, %0.2lfW, %luMHz, %luMHz, %lu%%, %lu%%, %luº, %luº, %u, %lu\n",
			i                   ,
			data[i].energy_j    , data[i].power_w,
			data[i].freq_gpu    , data[i].freq_mem,
			data[i].util_gpu    , data[i].util_mem,
			data[i].temp_gpu    , data[i].temp_mem,
			data[i].working     , data[i].samples);
		length = length - s;
		accuml = accuml + s;
	}
	return EAR_SUCCESS;
}
