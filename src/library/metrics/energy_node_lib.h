/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_ENERGY_H
#define EAR_ENERGY_H

#include <common/includes.h>
#include <daemon/shared_configuration.h>
#include <metrics/accumulators/types.h>

/* Energy units are 1=Joules, 1000=mJ, 1000000=uJ, 1000000000nJ */
state_t energy_units(uint *units);

state_t energy_node_load(char *path, int eard);
void energy_node_dispose();
state_t energy_datasize(size_t *size);
state_t energy_frequency(ulong *freq_us);
state_t energy_data_accumulated(unsigned long *e, edata_t init, edata_t end);
state_t energy_data_to_str(char *str, edata_t e);
state_t energy_data_copy(edata_t dst, edata_t src);
state_t energy_data_alloc(edata_t *data);
state_t energy_read(void *ctx, edata_t e);
uint energy_data_is_null(edata_t e);

#endif // EAR_ENERGY_H
