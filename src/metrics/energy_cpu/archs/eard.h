/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#ifndef METRICS_ENERGY_CPU_EARD_H
#define METRICS_ENERGY_CPU_EARD_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <metrics/common/pci.h>
#include <metrics/common/apis.h>
#include <metrics/energy_cpu/energy_cpu.h>
#include <common/hardware/topology.h>

state_t energy_cpu_eard_load(topology_t *tp_in, uint eard);

state_t energy_cpu_eard_get_granularity(ctx_t *c, uint *granularity);

state_t energy_cpu_eard_init(ctx_t *c);

state_t energy_cpu_eard_dispose(ctx_t *c);

state_t energy_cpu_eard_count_devices(ctx_t *c, uint *devs_count_in);

state_t energy_cpu_eard_read(ctx_t *c, ullong *values);

#endif
