/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_TEMPERATURE_ARCHS_DUMMY
#define METRICS_TEMPERATURE_ARCHS_DUMMY

#include <metrics/temperature/temperature.h>

state_t temp_dummy_status(topology_t *topo);

state_t temp_dummy_init(ctx_t *c);

state_t temp_dummy_dispose(ctx_t *c);

// Data
state_t temp_dummy_count_devices(ctx_t *c, uint *count);

// Getters
state_t temp_dummy_read(ctx_t *c, llong *temp, llong *average);

#endif //METRICS_TEMPERATURE_ARCHS_AMD17
