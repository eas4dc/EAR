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

/*
 * Usage:
 * Just call dynais() passing the sample and the size of
 * this sample. It will be returned one of these states:
 *      END_LOOP
 *      NO_LOOP
 *      IN_LOOP
 *      NEW_ITERATION
 *      NEW_LOOP
 *      END_NEW_LOOP
 *
 * Errors:
 * A returned dynais_init() value different than 0,
 * means that something went wrong while allocating
 * memory.
 *
 * You HAVE to call dynais_init() method before with
 * the window length and the number of levels and
 * dynais_dispose() at the end of the execution.
 *
 * You can also set the METRICS define to 1 in case you
 * want some metrics at the end of the execution. Moreover,
 * you can increase MAX_LEVELS in case you need more or
 * METRICS_WINDOW, used to store the information of the
 * different loops found (sorted by size) because could
 * be necessary if you want to test big single level
 * windows.
 */

#include <stdlib.h>
#include <unistd.h>
#include <common/hardware/defines.h>
#include <library/dynais/dynais.h>
#if __ARCH_X86
#include <immintrin.h>
#include <library/dynais/avx2/dynais.h>
#endif
#if __ARCH_X86 && FEAT_AVX512
#include <library/dynais/avx512/dynais.h>
#endif

static int type;

#if 0
static int dynais_dummy(uint sample, uint *size, uint *level)
{
    return NO_LOOP;
}
#endif

dynais_call_t dynais_init(topology_t *tp, uint window, uint levels)
{
    // Default if x86
    #if __ARCH_X86
	#if FEAT_AVX512
            if (tp->avx512) {
		type = DYNAIS_AVX512;
		// Returning AVX512 dynais function
		debug("Selected DynAIS AVX-512");
		return avx512_dynais_init((ushort) window, (ushort) levels);
	    }
	#endif
	type = DYNAIS_AVX2;
	debug("Selected DynAIS AVX-2");
	return avx2_dynais_init(window, levels);
    #elif __ARCH_ARM
    // By now, ARM version doesn't exist (SVE in future)
    type = DYNAIS_NONE;
    return NULL;
    #endif
}

void dynais_dispose()
{
}

int dynais_build_type()
{
    return type;
}

uint dynais_sample_convert(ulong sample)
{
	#if __ARCH_X86
	uint *p = (uint *) &sample;
	p[0] = _mm_crc32_u32(p[0], p[1]);
	return (uint) p[0];
	#else
	return 0;
	#endif
}
