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

#ifndef EAR_ENERGY_H
#define EAR_ENERGY_H

#include <common/includes.h>
#include <daemon/shared_configuration.h>
#include <metrics/accumulators/types.h>

state_t energy_lib_init(settings_conf_t *conf);
state_t energy_lib_datasize(size_t *size);
state_t energy_lib_frequency(ulong *freq_us);
/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_lib_units(uint *units);
state_t energy_lib_data_accumulated(unsigned long *e,edata_t init,edata_t end);
state_t energy_lib_data_to_str(char *str,edata_t e);
uint energy_lib_data_is_null(edata_t e);
state_t energy_lib_data_copy(edata_t dst,edata_t src);


#endif //EAR_ENERGY_H
