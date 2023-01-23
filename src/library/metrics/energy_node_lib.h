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

#ifndef EAR_ENERGY_H
#define EAR_ENERGY_H

#include <common/includes.h>
#include <daemon/shared_configuration.h>
#include <metrics/accumulators/types.h>

/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_units(uint *units);

state_t energy_node_load(char *path, int eard);
state_t energy_datasize(size_t *size);
state_t energy_frequency(ulong *freq_us);
state_t energy_data_accumulated(unsigned long *e,edata_t init,edata_t end);
state_t energy_data_to_str(char *str,edata_t e);
state_t energy_data_copy(edata_t dst,edata_t src);
state_t energy_data_alloc(edata_t *data);
state_t energy_read(void *ctx, edata_t e);
uint energy_data_is_null(edata_t e);


#endif //EAR_ENERGY_H
