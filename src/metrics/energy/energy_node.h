/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/



#ifndef METRICS_ENERGY_NODE_H
#define METRICS_ENERGY_NODE_H

#include <common/includes.h>
#include <common/system/time.h>
#include <metrics/accumulators/types.h>
#include <common/types/configuration/cluster_conf.h>

/* These two functions breaks load from initialization and simplifies the path to the energy  plugin, it's pending to add in EARD */
state_t energy_load(char *energy_obj);
state_t energy_initialization(ehandler_t *eh);

state_t energy_init(cluster_conf_t *conf, ehandler_t *eh);

state_t energy_dispose(ehandler_t *eh);

state_t energy_handler_clean(ehandler_t *eh);

state_t energy_datasize(ehandler_t *eh, size_t *size);

state_t energy_frequency(ehandler_t *eh, ulong *freq_us);

/* edata_t is a pointer and memory must be allocated using energy_datasize before using it */
state_t energy_dc_read(ehandler_t *eh, edata_t  energy_mj);

/* edata_t is a pointer and memory must be allocated using energy_datasize before using it */
state_t energy_dc_time_read(ehandler_t *eh, edata_t energy_mj, ulong *time_ms);

state_t energy_ac_read(ehandler_t *eh, edata_t energy_mj);

/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_units(ehandler_t *eh,uint *units);

state_t energy_accumulated(ehandler_t *eh,unsigned long *e,edata_t init,edata_t end);

state_t energy_to_str(ehandler_t *eh,char *str,edata_t e);

state_t energy_set_power_limit(ehandler_t *eh,ulong limit,ulong target);

state_t energy_not_privileged_init();

#endif //EAR_ENERGY_H

