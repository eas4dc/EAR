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

#include <common/config.h>
#include <common/includes.h>
#include <common/output/verbose.h>
#include <common/system/symplug.h>
#include <common/hardware/hardware_info.h>
#include <metrics/energy/energy_node.h>
#include <common/types/configuration/cluster_conf.h>

struct energy_op
{
	state_t (*init)               (void **c);
	state_t (*dispose)            (void **c);
	state_t (*datasize)           (size_t *size);
	state_t (*frequency)          (ulong *freq);
	state_t (*dc_read)            (void *c, edata_t emj);
	state_t (*dc_time_read)       (void *c, edata_t emj, ulong *tms);
	state_t (*ac_read)            (void *c, edata_t em);
	state_t (*units)							(uint *uints);
	state_t (*accumulated)				(ulong *e,edata_t init, edata_t end);
	state_t (*energy_to_str)			(char *str,edata_t end);
	state_t (*power_limit)				(void *c, ulong limit,ulong target);
	uint    (*is_null)        		(edata_t end);
} energy_ops;
static char energy_objc[SZ_PATH];
static int  energy_loaded  = 0;
const int   energy_nops    = 12;
const char *energy_names[] = {
	"energy_init",
	"energy_dispose",
	"energy_datasize",
	"energy_frequency",
	"energy_dc_read",
	"energy_dc_time_read",
	"energy_ac_read",
	"energy_units",
	"energy_accumulated",
	"energy_to_str",
	"energy_power_limit",
	"energy_data_is_null"
};

state_t energy_load(char *energy_obj)
{
	state_t ret;
	
	if (energy_obj == NULL) {
		return_msg(EAR_ERROR, "Trying to load NULL energy plugin");
	}
	debug("loading shared object '%s'", energy_obj);
	if (state_fail(ret = symplug_open(energy_obj, (void **) &energy_ops, energy_names, energy_nops))) {
		debug("symplug_open() returned %d (%s)", ret, state_msg);
		return ret;
	}
	energy_loaded = 1;
	return EAR_SUCCESS; 
}

state_t energy_initialization(ehandler_t *eh)
{
	state_t ret;
	if (!energy_loaded){
		state_return_msg(EAR_NOT_READY,0,"Energy plugin not loaded");
	} 
	ret = energy_ops.init(&eh->context);
  if (ret!=EAR_SUCCESS) debug("energy_ops.init() returned %d", ret);
  return ret;

}
state_t energy_init(cluster_conf_t *conf, ehandler_t *eh)
{
	state_t ret;

	if (energy_loaded) {
		debug("Energy plugin already loaded, executing basic init");	
		if (state_fail(ret = energy_ops.init(&eh->context))) {
			debug("energy_ops.init() returned %d", ret);
		}
		return ret;
	}
	if (conf == NULL) {
		return_msg(EAR_ERROR, "The conf value cannot be NULL if the plugin is not loaded");
	}

	debug("Using ear.conf energy plugin %s",conf->install.obj_ener);
	sprintf(energy_objc, "%s/energy/%s", conf->install.dir_plug, conf->install.obj_ener);
	debug("loading shared object '%s'", energy_objc);

	//
	if (state_fail(ret = symplug_open(energy_objc, (void **) &energy_ops, energy_names, energy_nops))) {
		debug("symplug_open() returned %d (%s)", ret, state_msg);
		return ret;
	}
	//
	energy_loaded = 1;
	
	if (energy_ops.init != NULL){ 
		return energy_ops.init(&eh->context);
	} else {
		return_print(EAR_ERROR, "The symbol energy_init was not found in %s", energy_objc);
	}
	return ret;
}

state_t energy_dispose(ehandler_t *eh)
{
	state_t s = EAR_SUCCESS;

	if (energy_ops.dispose != NULL) {
		s = energy_ops.dispose(&eh->context);
	}

	return s;
}

state_t energy_handler_clean(ehandler_t *eh)
{
	memset(eh, 0, sizeof(eh->context));
	return EAR_SUCCESS;
}

state_t energy_datasize(ehandler_t *eh, size_t *size)
{
	preturn (energy_ops.datasize, size);
}

state_t energy_frequency(ehandler_t *eh, ulong *fus)
{
	int intents = 0;
	timestamp ts1;
	timestamp ts2;
	ulong e1;
	ulong e2;
	void *c;

	c = (void *) eh->context;

	// Dedicated frequency
	if (energy_ops.frequency != NULL) {
		return energy_ops.frequency ( fus);
	}

	if (energy_ops.dc_read == NULL) {
		return EAR_UNDEFINED;
	}

	// Generic frequency
	if (state_ok(energy_ops.dc_read(eh->context, &e1)))
	{
		do {
			energy_ops.dc_read(c, &e2);
			intents++;
		} while ((e1 == e2) && (intents < 5000));

		//
		if (intents == 5000) {
			return 0;
		}

		timestamp_getprecise(&ts1);
		e1 = e2;

		do {
			energy_ops.dc_read(c, &e2);
		} while (e1 == e2);

		timestamp_getprecise(&ts2);
		*fus = (timestamp_diff(&ts2, &ts1, TIME_USECS) / 2) / MAX_POWER_ERROR;
	}

	return EAR_SUCCESS;
}

state_t energy_dc_read(ehandler_t *eh, edata_t emj)
{
	preturn (energy_ops.dc_read, eh->context, emj);
}

state_t energy_dc_time_read(ehandler_t *eh, edata_t emj, ulong *tms)
{
	preturn (energy_ops.dc_time_read, eh->context, emj, tms);
}

state_t energy_ac_read(ehandler_t *eh, edata_t emj)
{
	preturn (energy_ops.ac_read, eh->context, emj);
}

/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_units(ehandler_t *eh,uint *units)
{
  *units=1;
  preturn (energy_ops.units, units);
}

state_t energy_accumulated(ehandler_t *eh,unsigned long *e,edata_t init,edata_t end)
{
  *e=0;
  preturn (energy_ops.accumulated,e,init,end );
}

state_t energy_to_str(ehandler_t *eh,char *str,edata_t e)
{
  preturn (energy_ops.energy_to_str,str,e );
}

state_t energy_set_power_limit(ehandler_t *eh,ulong limit,ulong target)
{
  preturn (energy_ops.power_limit, eh->context, limit ,target);
}

uint energy_data_is_null(ehandler_t *eh,edata_t e)
{
  if (energy_ops.is_null != NULL){
    preturn(energy_ops.is_null,e);
  }else{
    /* Should we return 1 or 0 by default */
    return 1;
  }

}

