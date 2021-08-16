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

#include <stdio.h>
#include <stdlib.h>
//#define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <metrics/imcfreq/archs/dummy.h>

static imcfreq_ops_t *ops_static;
static uint devs_count;

void imcfreq_dummy_load(topology_t *tp, imcfreq_ops_t *ops)
{
	// If there is an API already loaded
	if (apis_loaded(ops)) {
		apis_add(ops->init_static, imcfreq_dummy_init_static);
	}

	// Filling each gap
	apis_put(ops->get_api,       imcfreq_dummy_get_api);
	apis_put(ops->init,          imcfreq_dummy_init);
	apis_put(ops->dispose,       imcfreq_dummy_dispose);
	apis_put(ops->count_devices, imcfreq_dummy_count_devices);
	apis_put(ops->read,          imcfreq_dummy_read);
	apis_put(ops->data_alloc,    imcfreq_dummy_data_alloc);
	apis_put(ops->data_free,     imcfreq_dummy_data_free);
	apis_put(ops->data_copy,     imcfreq_dummy_data_copy);
	apis_put(ops->data_diff,     imcfreq_dummy_data_diff);
	apis_put(ops->data_print,    imcfreq_dummy_data_print);
	apis_put(ops->data_tostr,    imcfreq_dummy_data_tostr);

	// Static info
	devs_count = tp->socket_count;
	ops_static = ops;
}

void imcfreq_dummy_get_api(uint *api, uint *api_intern)
{
	*api = API_DUMMY;
	if (api_intern) {
		*api_intern = API_NONE;
	}
}

state_t imcfreq_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t imcfreq_dummy_init_static(ctx_t *c)
{
	return ops_static->count_devices(c, &devs_count);
}

state_t imcfreq_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t imcfreq_dummy_count_devices(ctx_t *c, uint *devs_count_in)
{
	*devs_count_in = devs_count;
	return EAR_SUCCESS;
}

state_t imcfreq_dummy_read(ctx_t *c, imcfreq_t *i)
{
	uint cpu;
	if (i == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
    // Cleaning
    memset(i, 0, devs_count * sizeof(imcfreq_t));
    for (cpu = 0; cpu < devs_count; ++cpu) {
        i[cpu].error = 1;
    }
	return EAR_SUCCESS;
}

state_t imcfreq_dummy_data_alloc(imcfreq_t **i, ulong **freq_list)
{
    if (i != NULL) {
        if ((*i = (imcfreq_t *) calloc(devs_count, sizeof(imcfreq_t))) == NULL) {
            return_msg(EAR_ERROR, strerror(errno));
        }
    }
    if (freq_list != NULL) {
        if ((*freq_list = (ulong *) calloc(devs_count, sizeof(ulong))) == NULL) {
            return_msg(EAR_ERROR, strerror(errno));
        }
    }
    return EAR_SUCCESS;
}

state_t imcfreq_dummy_data_free(imcfreq_t **i, ulong **freq_list)
{
    if (i != NULL && *i != NULL) {
        free(*i);
        *i = NULL;
    }
    if (freq_list != NULL && *freq_list != NULL) {
        free(*freq_list);
        *freq_list = NULL;
    }
    return EAR_SUCCESS;
}

state_t imcfreq_dummy_data_copy(imcfreq_t *i2, imcfreq_t *i1)
{
    memcpy(i2, i1, sizeof(imcfreq_t) * devs_count);
    return EAR_SUCCESS;
}

state_t imcfreq_dummy_data_diff(imcfreq_t *i2, imcfreq_t *i1, ulong *freq_list, ulong *average)
{
    ulong time;
    ulong freq;
    ulong aux1; // Adds frequencies
    ulong aux2; // Counts valid devices
    int cpu;

		debug("imcfreq_dummy_data_diff");

    if (i2 == NULL || i1 == NULL) {
        return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
    }
    if (freq_list != NULL) {
        memset((void *) freq_list, 0, sizeof(ulong) * devs_count);
    }
    if (average != NULL) {
        *average = 0LU;
    }
    time = (ulong) timestamp_diff(&i2[0].time, &i1[0].time, TIME_MSECS);
    //
    for (cpu = 0, aux1 = aux2 = 0LU; cpu < devs_count; ++cpu) {
        if (freq_list != NULL) {
            freq_list[cpu] = 0LU;
        }
        if (i2[cpu].error || i1[cpu].error || time == 0) {
            continue;
        }
        //
        freq  = (i2[cpu].freq - i1[cpu].freq) / time;
        aux1 += freq;
        aux2 += 1;
        //
        if (freq_list != NULL) {
            freq_list[cpu] = freq;
        }
    }
    if (average != NULL) {
        *average = 0LU;
        if (aux2 > 0LU) {
            *average = aux1 / aux2;
        }
    }
	return EAR_SUCCESS;
}

void imcfreq_dummy_data_print(ulong *freq_list, ulong *average, int fd)
{
	char buffer[SZ_BUFFER];
	imcfreq_data_tostr(freq_list, average, buffer, SZ_BUFFER);
	dprintf(fd, "%s", buffer);
}

void imcfreq_dummy_data_tostr(ulong *freq_list, ulong *average, char *buffer, size_t length)
{
    double freq_ghz;
    int accum = 0;
    int i;

    accum += sprintf(buffer, "IMC:");
    for (i = 0; freq_list != NULL && i < devs_count; ++i) {
        freq_ghz = ((double) freq_list[i]) / 1000000.0;
        accum += sprintf(&buffer[accum], " %0.1lf", freq_ghz);
    }
    if (freq_list != NULL) {
        accum += sprintf(&buffer[accum], ",");
    }
    if (average != NULL) {
        freq_ghz = ((double) *average) / 1000000.0;
        accum += sprintf(&buffer[accum], " avg %0.2lf", freq_ghz);
    }
    sprintf(&buffer[accum], " (GHz)\n");
}
