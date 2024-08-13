/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef NMETICS_ENERGY_CPU_H
#define NMETICS_ENERGY_CPU_H

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/energy_cpu/archs/msr.h>
#include <metrics/energy_cpu/archs/eard.h>
#include <metrics/energy_cpu/archs/dummy.h>

#define NUM_PACKS 2 //pck, dram and cpu

// Plugin
state_t energy_cpu_load(topology_t *tp, uint eard);

state_t energy_cpu_init(ctx_t *c);

state_t energy_cpu_dispose(ctx_t *c);

state_t energy_cpu_get_api(uint *api_int);


// Data
state_t energy_cpu_data_alloc(ctx_t *c, ullong **values, uint *rapl_count);

state_t energy_cpu_data_copy(ctx_t *c, ullong *dest, ullong *source);

state_t energy_cpu_data_free(ctx_t *c, ullong **temp);

state_t energy_cpu_data_diff(ctx_t *c, ullong *start, ullong *end, ullong *result);

state_t energy_cpu_count_devices(ctx_t *c, uint *count);


// Getters
state_t energy_cpu_read(ctx_t *c, ullong *values);

state_t energy_cpu_read_copy(ctx_t *c, ullong *v2, ullong *v1, ullong *vD);

// Other
double energy_cpu_compute_power(double energy, double elapsed_time);

state_t energy_cpu_to_str(ctx_t *c, char *str, ullong *values);
#endif
