/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_HARDWARE_CPUID_H
#define COMMON_HARDWARE_CPUID_H

#include <common/types/generic.h>

typedef struct cpuid_regs {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
} cpuid_regs_t;

#define CPUID(t, EAX, ECX)                                                                                             \
    t.eax = EAX;                                                                                                       \
    t.ebx = 0;                                                                                                         \
    t.ecx = ECX;                                                                                                       \
    t.edx = 0;                                                                                                         \
    cpuid_native(&t.eax, &t.ebx, &t.ecx, &t.edx);

void cpuid_native(uint *eax, uint *ebx, uint *ecx, uint *edx);

uint cpuid_getbits(uint reg, int left_bit, int right_bit);

uint cpuid_isleaf(uint leaf);

#endif // COMMON_HARDWARE_CPUID_H
