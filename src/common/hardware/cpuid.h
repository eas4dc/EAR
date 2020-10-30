/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef EAR_CPUID_H
#define EAR_CPUID_H

#include <common/types/generic.h>

typedef struct cpuid_regs {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
} cpuid_regs_t;

#define CPUID(t, EAX, ECX) \
    t.eax = EAX;           \
    t.ebx = 0;             \
    t.ecx = ECX;           \
    t.edx = 0;             \
    cpuid_native(&t.eax, &t.ebx, &t.ecx, &t.edx);

void cpuid_native(uint *eax, uint *ebx, uint *ecx, uint *edx);

uint cpuid_getbits(uint reg, int left_bit, int right_bit);

uint cpuid_isleaf(uint leaf);

#endif //EAR_CPUID_H
