/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_CPUFREQ_BASE_H
#define MANAGEMENT_CPUFREQ_BASE_H

#include <common/hardware/topology.h>
#include <common/states.h>

typedef struct cpufreq_base_s {
    ullong frequency; // In KHZ
    uint boost_enabled;
    uint not_reliable;
} cpufreq_base_t;

// Alias
typedef cpufreq_base_t cfb_t;

void cpufreq_base_init(topology_t *tp, cpufreq_base_t *base);

#endif // MANAGEMENT_CPUFREQ_BASE_H