/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#pragma once

#include <stdbool.h>

#include <common/states.h>
#include <common/types/generic.h>

typedef struct synthetic_powercap_sample {
    ulong iteration;
    uint total_nodes;
    uint idle_nodes;
    ulong requested_power_per_node;
    ulong demand_total;
    ulong current_power;
    ulong requested;
    ulong freeable_power;
    ulong extra_power;
} synthetic_powercap_sample_t;

void cluster_powercap_enable_synthetic_mode(ulong avg_node_power);
state_t cluster_powercap_enable_synthetic_trace(const char *csv_path);
bool cluster_powercap_is_synthetic_mode(void);
state_t synthetic_powercap_sample(ulong current_powercap, ulong default_powercap, uint max_nodes,
                                  synthetic_powercap_sample_t *sample);
