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

#include <stdio.h>
#include <stdlib.h>

#include <common/output/debug.h>
#include <common/states.h>
#include <metrics/energy/node/energy_node.h>



/*
 * MAIN FUNCTIONS
 */

state_t energy_init(void **c)
{
	debug("Energy dummy plugin initialized");
	return EAR_SUCCESS;
}

state_t energy_dispose(void **c) {
	return EAR_SUCCESS;
}
state_t energy_datasize(size_t *size)
{
	debug("energy_datasize %lu\n",sizeof(unsigned long));
	*size=sizeof(unsigned long);
	return EAR_SUCCESS;
}

state_t energy_frequency(ulong *freq_us) {
	*freq_us = 1000000;
	return EAR_SUCCESS;
}

state_t energy_to_str(char *str, edata_t e) {
        sprintf(str, "NA-dummy" );
        return EAR_SUCCESS;
}

unsigned long diff_node_energy(ulong init,ulong end)
{
  return 0;
}
state_t energy_units(uint *units) {
  *units = 1000;
  return EAR_SUCCESS;
}

state_t energy_accumulated(unsigned long *e, edata_t init, edata_t end) {
	*e = 0;
  return EAR_SUCCESS;
}



state_t energy_dc_read(void *c, edata_t energy_mj) {

	ulong *penergy_mj = (ulong *)energy_mj;
  debug("energy_dc_read\n");
  *penergy_mj = 0;

	return EAR_SUCCESS;
}

state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms) 
{
  ulong *penergy_mj = (ulong *)energy_mj;

  debug("energy_dc_read\n");

  *penergy_mj = 0;
	*time_ms = (ulong)time(NULL) * 1000;

  return EAR_SUCCESS;

}
uint energy_data_is_null(edata_t e)  
{
  ulong *pe = (ulong *)e;
  return (*pe == 0);

}

