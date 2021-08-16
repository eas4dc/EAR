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

#include <common/config.h>
//#define SHOW_DEBUGS 0
#include <common/includes.h>
#include <common/output/verbose.h>
#include <common/system/symplug.h>
#include <metrics/energy/energy_node_lib.h>

struct energy_op
{
	state_t (*datasize)	(size_t *size);
	state_t (*frequency)	(ulong *freq);
	state_t (*units)	(uint *units);
	state_t (*accumulated)	(unsigned long *e,edata_t init,edata_t end);
  state_t (*energy_to_str)	(char *str,edata_t end);
	uint    (*is_null)				(edata_t end);
	state_t (*copy)						(edata_t dest,edata_t src);
} energy_ops;
static char energy_manu[SZ_NAME_MEDIUM];
static char energy_prod[SZ_NAME_MEDIUM];
static char energy_objc[SZ_PATH];
static int  energy_loaded  = 0;
static size_t my_energy_data_size;
static const int   energy_nops    = 7;
static const char *energy_names[] = {
	"energy_datasize",
	"energy_frequency",
	"energy_units",
	"energy_accumulated",
	"energy_to_str",
	"energy_data_is_null",
	"energy_data_copy"
};

state_t energy_lib_init(settings_conf_t *conf)
{
	state_t ret=EAR_SUCCESS;
	int found=0;
	char *my_energy_plugin;

	debug("energy_init library");
	if (conf == NULL) {
		state_return_msg(EAR_BAD_ARGUMENT, 0,
		 "the conf value cannot be NULL if the plugin is not loaded");
	}
	my_energy_plugin=getenv("SLURM_EAR_ENERGY_PLUGIN");
	if ((my_energy_plugin==NULL) || (conf->user_type!=AUTHORIZED)){
		found = (strcmp(conf->installation.obj_ener, "default") != 0);

		if (found)
		{
			sprintf(energy_objc, "%s/energy/%s",
				conf->installation.dir_plug, conf->installation.obj_ener);
			sprintf(energy_prod, "custom");
			sprintf(energy_manu, "custom");
		}
	}else{
		found=1;
		strcpy(energy_objc,my_energy_plugin);
	}
	if (found) debug("loading shared object '%s'", energy_objc);

	if (found)
	{
		//
		ret = symplug_open(energy_objc, (void **) &energy_ops, energy_names, energy_nops);
		if (ret!=EAR_SUCCESS) debug("symplug_open() returned %d (%s)", ret, intern_error_str);		

		if (state_fail(ret)) {
			return ret;
		}

		//
		energy_loaded = 1;
	} else {
		ret = EAR_NOT_FOUND;
	}
	if (energy_ops.datasize != NULL){
		energy_ops.datasize(&my_energy_data_size);
	}else{
		my_energy_data_size = 1;
	}
	return ret;
}

state_t energy_lib_datasize(size_t *size)
{
	preturn(energy_ops.datasize,size);
}

state_t energy_lib_frequency(ulong *fus)
{
	*fus=ULONG_MAX;
	preturn(energy_ops.frequency,fus);
}

/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_lib_units(uint *units)
{
	*units=1;
	preturn (energy_ops.units, units);
}

state_t energy_lib_data_accumulated(unsigned long *e,edata_t init,edata_t end)
{
	*e=0;
	preturn (energy_ops.accumulated,e,init,end );
}

state_t energy_lib_data_to_str(char *str,edata_t e)
{
	preturn (energy_ops.energy_to_str,str,e );
}

uint energy_lib_data_is_null(edata_t e)
{
	if (energy_ops.is_null != NULL){
		preturn(energy_ops.is_null,e);
	}else{
		/* Should we return 1 or 0 by default */
		return 1;
	}
	
}

state_t energy_lib_data_copy(edata_t dst,edata_t src)
{
  if (energy_ops.copy != NULL){
    preturn(energy_ops.copy,dst,src);
  }else{
    ulong *udst,*usrc;
		udst = (ulong *)dst;
		usrc = (ulong *)src;
		memcpy(udst,usrc,my_energy_data_size);
  }
  return EAR_SUCCESS;
}
