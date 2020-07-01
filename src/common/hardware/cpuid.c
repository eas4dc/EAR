/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
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
