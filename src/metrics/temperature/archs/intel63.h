/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_TEMPERATURE_ARCHS_INTEL63
#define METRICS_TEMPERATURE_ARCHS_INTEL63

#include <metrics/temperature/temperature.h>

state_t temp_intel63_status(topology_t *tp);

state_t temp_intel63_init(ctx_t *s);

state_t temp_intel63_dispose(ctx_t *s);

// Data
state_t temp_intel63_count_devices(ctx_t *c, uint *count);

// Getters
state_t temp_intel63_read(ctx_t *s, llong *temp, llong *average);

#endif //METRICS_TEMPERATURE_ARCHS_INTEL63
