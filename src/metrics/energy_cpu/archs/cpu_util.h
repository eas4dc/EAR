/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_ENERGY_CPU_UTIL_H
#define METRICS_ENERGY_CPU_UTIL_H

// #define SHOW_DEBUGS 1

#include <common/hardware/topology.h>
#include <metrics/common/apis.h>
#include <sys/stat.h>

state_t energy_cpu_util_load(topology_t *tp_in);

state_t energy_cpu_util_get_granularity(ctx_t *c, uint *granularity);

state_t energy_cpu_util_init(ctx_t *c);

state_t energy_cpu_util_dispose(ctx_t *c);

state_t energy_cpu_util_count_devices(ctx_t *c, uint *devs_count_in);

state_t energy_cpu_util_read(ctx_t *c, ullong *values);

#endif
