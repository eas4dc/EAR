/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/hardware/cpuid.h>
#include <common/hardware/defines.h>
#include <common/output/debug.h>

void cpuid_native(uint *eax, uint *ebx, uint *ecx, uint *edx)
{
#if __ARCH_X86
    asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "0"(*eax), "2"(*ecx));
#endif
}

uint cpuid_getbits(uint reg, int left_bit, int right_bit)
{
    uint shift = left_bit - right_bit;
    uint mask  = (((0x01 << shift) - 1) << 1) + 1;
    return ((reg >> right_bit) & mask);
}

uint cpuid_isleaf(uint leaf)
{
    uint aux = 0x80000000;
    cpuid_regs_t r;

    if (leaf < aux) {
        aux = 0;
    }
    CPUID(r, aux, 0);
    // If max leafs are less
    if (r.eax < leaf) {
        return 0;
    }
    CPUID(r, leaf, 0);
    // EBX confirms its presence
    if (r.ebx == 0) {
        return 0;
    }
    return 1;
}
