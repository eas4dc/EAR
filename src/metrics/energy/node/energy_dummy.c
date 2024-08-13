/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <stdio.h>
#include <stdlib.h>

#include <common/output/debug.h>
#include <common/states.h>
#include <common/system/time.h>
#include <metrics/energy/node/energy_node.h>


static timestamp_t start_time;
static uint init;
/*
 * MAIN FUNCTIONS
 */

state_t energy_init(void **c)
{
	if (!init)
	{
		timestamp_get(&start_time);
		init = 1;
	}

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
  return end - init;
}

state_t energy_units(uint *units) {
  *units = 1000;
  return EAR_SUCCESS;
}

state_t energy_accumulated(unsigned long *e, edata_t init, edata_t end) {
  ulong *pinit = (ulong *) init, *pend = (ulong *) end;

  ulong total = diff_node_energy(*pinit, *pend);
  *e = total;
  return EAR_SUCCESS;
}



state_t energy_dc_read(void *c, edata_t energy_mj) {

	ulong *penergy_mj = (ulong *)energy_mj;

	ullong elapsed = timestamp_diffnow(&start_time, TIME_MSECS);

  *penergy_mj = (ulong) elapsed * 150; // Dummy power: 150 W
  debug("energy_dc_read: %lu elapsed: %llu\n", *penergy_mj, elapsed);

	return EAR_SUCCESS;
}

state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms) 
{
  ulong *penergy_mj = (ulong *)energy_mj;

	ullong elapsed = timestamp_diffnow(&start_time, TIME_MSECS);

  *penergy_mj = (ulong) elapsed * 150; // Dummy power: 150 W
  debug("energy_dc_read: %lu elapsed: %llu\n", *penergy_mj, elapsed);
	*time_ms = (ulong)time(NULL) * 1000;

  return EAR_SUCCESS;
}
uint energy_data_is_null(edata_t e)  
{
  ulong *pe = (ulong *)e;
  return (*pe == 0);
}

