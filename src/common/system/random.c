/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/hardware/defines.h>
#include <common/output/debug.h>
#include <common/system/random.h>
#include <stdlib.h>
#include <time.h>
#if __ARCH_X86
#include <immintrin.h>
#endif

uint random_get()
{
    int i = 0;
    unsigned int v;

#if __ARCH_X86
    // It is supported in AMD architectures too
    i = _rdrand32_step(&v);
#endif
    if (i == 0) {
        debug("hardware did not generate a random number");
        clock_t c = clock();
        v         = (unsigned int) c;
    }

    return v;
}

uint random_getrank(uint min, uint offset)
{
    uint v = random_get();
    return min + (v % offset);
}
