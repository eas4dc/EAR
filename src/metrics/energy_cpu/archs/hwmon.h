/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#ifndef METRICS_ENERGY_CPU_HWMON_H
#define METRICS_ENERGY_CPU_HWMON_H

#include <metrics/energy_cpu/energy_cpu.h>

state_t energy_cpu_hwmon_load(topology_t *tp_in);

state_t energy_cpu_hwmon_init(ctx_t *c);

state_t energy_cpu_hwmon_dispose(ctx_t *c);

state_t energy_cpu_hwmon_count_devices(ctx_t *c, uint *devs_count_in);

state_t energy_cpu_hwmon_read(ctx_t *c, ullong *values);

#endif
