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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1

#include <unistd.h>

#include <common/config.h>
#include <common/includes.h>

#include <common/output/verbose.h>

#include <common/system/lock.h>
#include <common/system/symplug.h>

#include <daemon/local_api/eard_api.h>

#include <library/common/utils.h>
#include <library/metrics/energy_node_lib.h>

static struct energy_op
{
    state_t (*datasize)	     (size_t *size);
    state_t (*frequency)     (ulong *freq);
    state_t (*units)	     (uint *units);
    state_t (*accumulated)   (unsigned long *e, edata_t init,edata_t end);
    state_t (*energy_to_str) (char *str, edata_t end);
	state_t (*read)          (void *ctx, edata_t t);
    state_t (*data_alloc)    (edata_t *data);
    uint    (*is_null)       (edata_t end);
    state_t (*data_copy)     (edata_t dest, edata_t src);
} energy_ops;

static int  energy_loaded  = 0;

static size_t my_energy_data_size = 0;

static const int   energy_nops    = 9;
static const char *energy_names[] = {
	"energy_datasize",
	"energy_frequency",
	"energy_units",
	"energy_accumulated",
	"energy_to_str",
	"energy_dc_read",
	"energy_data_alloc",
	"energy_data_is_null",
	"energy_data_copy"
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

state_t dummy_dc_read_substitute(void *ctx, edata_t t)
{
	memset(t, 0, my_energy_data_size);
	debug("dummy read");
	return EAR_SUCCESS;
}

state_t eards_dc_read_substitute(void *ctx, edata_t t)
{
	if (!eards_connected()) {
		debug("eard not connected");
		return EAR_ERROR;
	}
	debug("eards read");

	return eards_node_dc_energy(t, my_energy_data_size);
}

state_t energy_node_load(char *path, int eard)
{

	state_t ret;
	if (state_fail(ret = ear_trylock(&lock))) {
		return ret;
	}

	if (energy_loaded) {
		ear_unlock(&lock);
		return EAR_SUCCESS;
	}

    debug("Loading %s plugin...", path);
    #if FAKE_ERROR_ERROR_PATH
    ret = EAR_ERROR;
    #else
    ret = symplug_open(path, (void **) &energy_ops, 
    energy_names, energy_nops);
    #endif
    if (state_fail(ret)) {
        debug("symplug_open() returned %d (%s)", ret, intern_error_str);
		    ear_unlock(&lock);
        return_msg(ret, "Energy plugin symbols can not be opened.");
    }
    
    energy_loaded = 1;

	int root_privileges = (getuid() == 0);

	if (!root_privileges) {
  #if FAKE_ERROR_EARD_NOT_CONNECTED
    if (0){
  #else
		if (eard && eards_connected()) {
  #endif
			energy_ops.read = eards_dc_read_substitute;
			debug("energy_dc_read is eards");
		} else {
			energy_ops.read = dummy_dc_read_substitute;
			debug("energy_dc_read is dummy");
		}
	}
	if (energy_ops.datasize != NULL) {
		energy_ops.datasize(&my_energy_data_size);
		debug("read energy datasize %lu", my_energy_data_size);
	} else {
		my_energy_data_size = 1;
	}


	ear_unlock(&lock);

	return EAR_SUCCESS;
}

state_t energy_datasize(size_t *size)
{
	state_t ret;
	if (state_fail(ret = ear_trylock(&lock))) {
		return ret;
	}
	if (my_energy_data_size) {
		*size = my_energy_data_size;
		ret = EAR_SUCCESS;
	}
	else {
		ret = energy_ops.datasize(size);
	}
	//debug("read energy datasize %lu", *size);
	ear_unlock(&lock);
	return ret;
}

state_t energy_frequency(ulong *fus)
{
	*fus=ULONG_MAX;
	preturn(energy_ops.frequency,fus);
}

/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_units(uint *units)
{
	*units=1;
	preturn(energy_ops.units, units);
}

state_t energy_data_accumulated(unsigned long *e, edata_t init, edata_t end)
{
	*e=0;
	preturn(energy_ops.accumulated,e,init,end );
}

state_t energy_read(void *ctx, edata_t e)
{
	state_t ret;
	if (state_fail(ret = ear_trylock(&lock))) {
		debug("cannot get lock");
		return ret;
	}
	if (energy_ops.read == NULL) {
		debug("energy_dc_read is NULL!!!!!");
		ear_unlock(&lock);
		return EAR_ERROR;
	}
	ret = energy_ops.read(ctx, e);
	debug("energy_read finished");
	ear_unlock(&lock);
	return ret;
}

state_t energy_data_to_str(char *str, edata_t e)
{
	preturn(energy_ops.energy_to_str, str, e);
}

uint energy_data_is_null(edata_t e)
{
	if (energy_ops.is_null != NULL){
		preturn(energy_ops.is_null,e);
	}else{
		/* Should we return 1 or 0 by default */
		return 1;
	}

}

state_t energy_data_alloc(edata_t *data)
{
	if (data == NULL) return EAR_ERROR;
	//if (*data != NULL) return EAR_ERROR;

	if (energy_ops.data_alloc != NULL) {
		return energy_ops.data_alloc(data);
	}
	*data = (edata_t) calloc(1, my_energy_data_size);
	if (*data == NULL) return EAR_ERROR;

	return EAR_SUCCESS;
}

state_t energy_data_copy(edata_t dst, edata_t src)
{
	if (energy_ops.data_copy != NULL){
		preturn(energy_ops.data_copy, dst, src);
	}else{
		ulong *udst, *usrc;
		udst = (ulong *)dst;
		usrc = (ulong *)src;
		memcpy(udst,usrc, my_energy_data_size);
	}
	return EAR_SUCCESS;
}
