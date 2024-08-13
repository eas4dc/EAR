/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_HARDWARE_TOPOLOGY_SPECIFIC_H
#define COMMON_HARDWARE_TOPOLOGY_SPECIFIC_H

#include <common/hardware/topology.h>

#define GENUINE_INTEL 1970169159
#define AUTHENTIC_AMD 0

void topology_asm(topology_t *topo);

ullong topology_asm_freqspec();

#endif