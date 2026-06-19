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

IMCFREQ_DEFINES(intel63);

// External functions for other Intel63 APIs

// Load addreses given a CPU model.
state_t imcfreq_intel63_ext_load_addresses(topology_t *tp);
// Enables the control registers. MSR have to be opened before.
state_t imcfreq_intel63_ext_enable_cpu(int cpu);
// Reads a 48 bit counter and returns a frequency in KHz.
state_t imcfreq_intel63_ext_read_cpu(int cpu, ulong *freq);

#endif // METRICS_IMCFREQ_INTEL63_H
