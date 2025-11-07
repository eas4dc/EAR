/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_IMCFREQ_INTEL63_H
#define METRICS_IMCFREQ_INTEL63_H

#include <metrics/imcfreq/imcfreq.h>

void imcfreq_intel63_load(topology_t *tp, imcfreq_ops_t *ops);

void imcfreq_intel63_get_api(uint *api, uint *api_intern);

state_t imcfreq_intel63_init(ctx_t *c);

state_t imcfreq_intel63_dispose(ctx_t *c);

state_t imcfreq_intel63_count_devices(ctx_t *c, uint *dev_count);

state_t imcfreq_intel63_read(ctx_t *c, imcfreq_t *i);

// External functions for other Intel63 APIs

// Load addreses given a CPU model.
state_t imcfreq_intel63_ext_load_addresses(topology_t *tp);
// Opens an MSR and enables the control register.
state_t imcfreq_intel63_ext_enable_cpu(int cpu);
// Reads a 48 bit counter and returns a frequency in KHz.
state_t imcfreq_intel63_ext_read_cpu(int cpu, ulong *freq);

#endif // METRICS_IMCFREQ_INTEL63_H
