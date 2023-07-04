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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <time.h>
#include <stdlib.h>
#include <common/output/debug.h>
#include <common/system/random.h>
#include <common/hardware/defines.h>
#if __ARCH_X86
#include <immintrin.h>
#endif

uint random_get()
{
	int i = 0;
	unsigned int v;

	// It is supported in AMD architectures too
        #if __ARCH_X86
	// It is supported in AMD architectures too
	i =  _rdrand32_step(&v);
	#endif
	if (i == 0) {
		debug("hardware did not generate a random number");
		clock_t c = clock();
		v = (unsigned int) c;
	}

	return v;
}

uint random_getrank(uint min, uint offset)
{
	uint v = random_get();
	return min + (v % offset);
}
