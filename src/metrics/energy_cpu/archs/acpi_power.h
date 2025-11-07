/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#ifndef METRICS_ENERGY_CPU_ACPI_POWER_METER_H
#define METRICS_ENERGY_CPU_ACPI_POWER_METER_H

#include <common/hardware/topology.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

state_t acpi_power_meter_load(topology_t *tp_in);

state_t acpi_power_meter_init(ctx_t *c);

state_t acpi_power_meter_dispose(ctx_t *c);

state_t acpi_power_meter_count_devices(ctx_t *c, uint *devs_count_in);

state_t acpi_power_meter_read(ctx_t *c, ullong *values);

#endif
