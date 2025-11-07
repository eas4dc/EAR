/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _ARCH_INFO_H_
#define _ARCH_INFO_H_

#include <common/hardware/topology.h>
#include <common/sizes.h>
#include <common/states.h>

typedef struct architecture {
    unsigned long max_freq_avx512;
    unsigned long max_freq_avx2;
    int pstates;
    topology_t top;
} architecture_t;

/** Fills the current architecture in arch*/
state_t get_arch_desc(architecture_t *arch);

/** Copy src in dest */
state_t copy_arch_desc(architecture_t *dest, architecture_t *src);

/** Prints in stdout the current architecture*/
void print_arch_desc(architecture_t *arch);

/** Uses the verbose to print the current architecture*/
void verbose_architecture(int v, architecture_t *arch);

#endif
