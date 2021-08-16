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

//#define SHOW_DEBUGS 1

#include <signal.h>

#include <pthread.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <metrics/common/apis.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/imcfreq/archs/eard.h>
#include <metrics/imcfreq/archs/dummy.h>
#include <metrics/imcfreq/archs/amd17.h>
#include <metrics/imcfreq/archs/intel63.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static imcfreq_ops_t ops;

void imcfreq_load(topology_t *tp, int eard)
{
	while (pthread_mutex_trylock(&lock));
	if (ops.init != NULL) {
		goto done;
    }
	debug("loading imcfreq metrics");
	imcfreq_eard_load(tp, &ops, eard);
	imcfreq_intel63_load(tp, &ops);
	imcfreq_amd17_load(tp, &ops, eard);
	imcfreq_dummy_load(tp, &ops);
done:
	pthread_mutex_unlock(&lock);
	debug("loading imcfreq metrics finalized");
}

void imcfreq_get_api(uint *api)
{
	if (!ops.get_api) {
    	*api = API_NONE;
	}
	ops.get_api(api, NULL);
}

state_t imcfreq_init(ctx_t *c)
{
	state_t s = EAR_SUCCESS;
	int i = 0;

	while (pthread_mutex_trylock(&lock));
	if (state_ok(s = ops.init(c))) {
		while(ops.init_static[i] != NULL) {
			ops.init_static[i](c);
			++i;
		}
	}
	pthread_mutex_unlock(&lock);
	return s;
}

state_t imcfreq_dispose(ctx_t *c)
{
	preturn(ops.dispose, c);
}

state_t imcfreq_count_devices(ctx_t *c, uint *dev_count)
{
	preturn(ops.count_devices, c, dev_count);
}

state_t imcfreq_read(ctx_t *c, imcfreq_t *i)
{
	preturn(ops.read, c, i);
}

state_t imcfreq_read_diff(ctx_t *c, imcfreq_t *i2, imcfreq_t *i1, ulong *freqs, ulong *average)
{
	state_t s;
	if (state_fail(s = imcfreq_read(c, i2))) {
		return s;
	}
	return imcfreq_data_diff(i2, i1, freqs, average);
}

state_t imcfreq_read_copy(ctx_t *c, imcfreq_t *i2, imcfreq_t *i1, ulong *freqs, ulong *average)
{
	state_t s;
	if (state_fail(s = imcfreq_read_diff(c, i2, i1, freqs, average))) {
		return s;
	}
	return imcfreq_data_copy(i1, i2);
}

state_t imcfreq_data_alloc(imcfreq_t **i, ulong **freq_list)
{
	preturn(ops.data_alloc, i, freq_list);
}

state_t imcfreq_data_free(imcfreq_t **i, ulong **freq_list)
{
	preturn(ops.data_free, i, freq_list);
}

state_t imcfreq_data_copy(imcfreq_t *i2, imcfreq_t *i1)
{
	preturn(ops.data_copy, i2, i1);
}

state_t imcfreq_data_diff(imcfreq_t *i2, imcfreq_t *i1, ulong *freq_list, ulong *average)
{
	preturn(ops.data_diff, i2, i1, freq_list, average);
}

void imcfreq_data_print(ulong *freq_list, ulong *average, int fd)
{
	ops.data_print(freq_list, average, fd);
}

void imcfreq_data_tostr(ulong *freq_list, ulong *average, char *buffer, size_t length)
{
	ops.data_tostr(freq_list, average, buffer, length);
}
