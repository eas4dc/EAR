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

#include <time.h>
#include <stdlib.h>
#include <common/output/debug.h>
#include <immintrin.h>

uint random_get()
{
	unsigned int i;
	unsigned int v;

	// It is supported in AMD architectures too
	i = _rdrand32_step(&v);
	
	if (i == 0)
	{
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
