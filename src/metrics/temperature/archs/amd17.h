/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_TEMPERATURE_ARCHS_AMD17
#define METRICS_TEMPERATURE_ARCHS_AMD17

#include <metrics/temperature/temperature.h>

state_t temp_amd17_status(topology_t *topo);

state_t temp_amd17_init(ctx_t *c);

state_t temp_amd17_dispose(ctx_t *c);

// Data
state_t temp_amd17_count_devices(ctx_t *c, uint *count);

// Getters
state_t temp_amd17_read(ctx_t *c, llong *temp, llong *average);

#endif //METRICS_TEMPERATURE_ARCHS_AMD17
