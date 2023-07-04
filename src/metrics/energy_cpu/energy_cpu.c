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
#include <stdlib.h>
#include <common/plugins.h>
#include <common/system/lock.h>
#include <common/output/debug.h>
#include <metrics/energy_cpu/archs/msr.h>
#include <metrics/energy_cpu/archs/eard.h>
#include <metrics/energy_cpu/archs/dummy.h>
#include <metrics/energy_cpu/archs/cpu_util.h>
#include <metrics/energy_cpu/energy_cpu.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint api = API_DUMMY;
static uint socket_count;


static struct energy_cpu_ops
{
	state_t (*init)          (ctx_t *c);
	state_t (*dispose)       (ctx_t *c);
	state_t (*count_devices) (ctx_t *c, uint *count);
	state_t (*read)          (ctx_t *c, ullong *values);
} ops;

state_t energy_cpu_load(topology_t *tp, uint eard)
{
	
	debug("entering cpu_load with tp->initialized %d", tp->initialized);
	state_t s;
	if (state_fail(s = ear_trylock(&lock))) {
		return s;	
	}
	if (ops.init != NULL) {
		ear_unlock(&lock);
		return EAR_SUCCESS;
	}

	if (state_ok(rapl_msr_load(tp))) {
		ops.init          = rapl_msr_init;
		ops.dispose       = rapl_msr_dispose;
		ops.read          = rapl_msr_read;
		ops.count_devices = rapl_msr_count_devices;
		//common functions, they don't need to be binded since they are all the same and can be found in this file
		/* ops.diff          = energy_cpu_data_diff;
		ops.data_copy     = energy_cpu_data_copy;
		ops.data_tostr    = energy_cpu_data_tostr; */
		api = API_RAPL;
		debug("selected rapl_msr energy_cpu (api %u)", api);
	} else if (state_ok(energy_cpu_eard_load(tp, eard))) {
		ops.init          = energy_cpu_eard_init;
		ops.dispose       = energy_cpu_eard_dispose;
		ops.read          = energy_cpu_eard_read;
		ops.count_devices = energy_cpu_eard_count_devices;
		api = API_EARD;
		debug("selected eard energy_cpu (api %u)", api);
	} 
	else if (state_ok(energy_cpu_util_load(tp))) {
		ops.init          = energy_cpu_util_init;
		ops.dispose       = energy_cpu_util_dispose;
		ops.read          = energy_cpu_util_read;
		ops.count_devices = energy_cpu_util_count_devices;
		api = API_CPUMODEL;
		debug("selected CPUMODEL energy_cpu (api %u)", api);
	} 
	else if (state_ok(energy_cpu_dummy_load(tp))) {
		ops.init          = energy_cpu_dummy_init;
		ops.dispose       = energy_cpu_dummy_dispose;
		ops.read          = energy_cpu_dummy_read;
		ops.count_devices = energy_cpu_dummy_count_devices;
		api = API_DUMMY;
		debug("selected dummy energy_cpu (api %u)", api);
	}

	ear_unlock(&lock);

	if (ops.init == NULL) {
		debug("cannot load energy_cpu");
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	return EAR_SUCCESS;
}

state_t energy_cpu_init(ctx_t *c)
{
	int s;
	if (xtate_fail(s, energy_cpu_count_devices(c, &socket_count))) {
		return s;
	}
	preturn(ops.init, c);
}

state_t energy_cpu_dispose(ctx_t *c)
{
	preturn(ops.dispose, c);
}

state_t energy_cpu_data_diff(ctx_t *c, ullong *start, ullong *end, ullong *result)
{
	int i;
	for (i = 0; i < socket_count*NUM_PACKS; ++i) {
		if (start[i] > end[i])
			result[i] = ullong_diff_overflow(start[i], end[i]);
		else
			result[i] = end[i] - start[i];
		debug("doing diff %d, result %llu", i, result[i]);
	}
	return EAR_SUCCESS;
}

state_t energy_cpu_get_api(uint *api_int)
{
	*api_int = api;
	return EAR_SUCCESS;
}

state_t energy_cpu_count_devices(ctx_t *c, uint *count)
{
	preturn(ops.count_devices, c, count);
}

// Data
state_t energy_cpu_data_alloc(ctx_t *c, ullong **values, uint *rapl_count)
{
	if (values == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if ((*values = (ullong *) calloc(socket_count*NUM_PACKS, sizeof(ullong))) == NULL) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	if (rapl_count != NULL) {
		*rapl_count = socket_count*NUM_PACKS;
	}
	return EAR_SUCCESS;
}

state_t energy_cpu_data_copy(ctx_t *c, ullong *dest, ullong *source)
{
	if (dest == NULL || source == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (memcpy((void *) dest, (const void *) source, NUM_PACKS*sizeof(ullong)*socket_count) != dest) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t energy_cpu_data_free(ctx_t *c, ullong **values)
{
	if (values == NULL || *values == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	free(*values);
	*values = NULL;
	return EAR_SUCCESS;
}

// Getters
state_t energy_cpu_read(ctx_t *c, ullong *values)
{
	state_t s;
	debug("entering energy_cpu_read");
	if (state_fail(s = ear_trylock(&lock))) {
		debug("cannot get pthread lock");
		return s;	
	}
	debug("got energy_cpu_lock, reading");
	s = ops.read(c, values);
	ear_unlock(&lock);
	debug("finished read");
	int i;
	for (i = 0; i < socket_count*NUM_PACKS; ++i) {
		debug("cpu read %d returns %llu", i, values[i]);
	}

	return s;

}

state_t energy_cpu_to_str(ctx_t *c, char *str, ullong *values)
{
	char buffer[512];
	strcpy(str, "energy_cpu (");
	int i;

  /* DRAM values + PCK values */
	for (i = 0; i < socket_count; i++)
	{
		sprintf(buffer, "DRAM%d: %llu ",  i, values[i]);
		strcat(str, buffer);
		sprintf(buffer, "PKG%d: %llu ", i, values[socket_count+i]);
		strcat(str, buffer);
		/*sprintf(buffer, "CPU%d: %llu ",  i, values[i*NUM_PACKS+2]);
		strcat(str, buffer); */
	}
	strcat(str, ")");
    return EAR_SUCCESS;
}

double energy_cpu_compute_power(double energy, double elapsed_time)
{
    
  if (elapsed_time > 0){
	  double computed_power = ((energy / 1000000000.0) / elapsed_time);

    debug("computed power: %lf", computed_power);
    return computed_power;
  }
  return (double)0;
}

