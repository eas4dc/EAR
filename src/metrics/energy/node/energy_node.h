/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_API_NODE_ENERGY_H_
#define _EAR_API_NODE_ENERGY_H_

#include <common/states.h>
#include <common/types/generic.h>
#include <metrics/accumulators/types.h>

state_t energy_init(void **c);

state_t energy_dispose(void **c);

state_t energy_datasize(size_t *size);

/** Frequency is the minimum time bettween two changes, in usecs */
state_t energy_frequency(ulong *freq_us);

state_t energy_dc_read(void *c, void *energy_mj);

state_t energy_dc_time_read(void *c, void *energy_mj, ulong *time_ms);

state_t energy_ac_read(void *c, void *energy_mj);

/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_units(uint *units);

state_t energy_accumulated(ulong *e, void *init, void *end);

state_t energy_to_str(char *str, void *e);

#endif
