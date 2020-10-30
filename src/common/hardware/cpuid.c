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

#include <common/hardware/cpuid.h>

void cpuid_native(uint *eax, uint *ebx, uint *ecx, uint *edx)
{
	asm volatile("cpuid"
	: "=a" (*eax),
	  "=b" (*ebx),
	  "=c" (*ecx),
	  "=d" (*edx)
	: "0"  (*eax), "2" (*ecx));
}

uint cpuid_getbits(uint reg, int left_bit, int right_bit)
{
	uint shift = left_bit - right_bit;
	uint mask  = (((0x01 << shift) - 1) << 1) + 1;
	return ((reg >> right_bit) & mask);
}

uint cpuid_isleaf(uint leaf)
{
	cpuid_regs_t r;

	CPUID(r,0,0);
	// If max leafs are less
	if (r.eax < leaf) {
		return 0;
	}

	CPUID(r,leaf,0);
	// EBX confirms its presence
	if (r.ebx == 0) {
		return 0;
	}

	return 1;
}
