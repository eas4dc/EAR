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

#include <stdlib.h>
#include <immintrin.h>
#include <common/output/error.h>
#include <common/utils/stress.h>

static char *mem1;
static char *mem2;

static void alloc_align(void **memptr, size_t alignment, size_t size)
{
    if (posix_memalign(memptr, alignment, size) != 0) {
        printf("posix_memalign failed\n");
    }
}

void stress_alloc()
{
    #define N 32*1024*1024 //32 MiB
    alloc_align((void **) &mem1, sizeof(__m256i), N);
    alloc_align((void **) &mem2, sizeof(__m256i), N);
}

void stress_free()
{
    free(mem1);
    free(mem2);
}

void stress_bandwidth(ullong seconds)
{
    int *pmem1 = (int *) mem1;
    int *pmem2 = (int *) mem2;
    int pN = N / sizeof(int);
    timestamp_t now;
    __m256i reg;
    int n;

    // Computing time
    timestamp_getfast(&now);
    do {
        for (n = 0; n < pN; n+=32) {
            reg = _mm256_set1_epi32(n);
            _mm256_store_si256((__m256i *) &pmem1[n+ 0], reg);
            _mm256_store_si256((__m256i *) &pmem2[n+ 0], reg);
            _mm256_store_si256((__m256i *) &pmem1[n+ 8], reg);
            _mm256_store_si256((__m256i *) &pmem2[n+ 8], reg);
            _mm256_store_si256((__m256i *) &pmem1[n+16], reg);
            _mm256_store_si256((__m256i *) &pmem2[n+16], reg);
            _mm256_store_si256((__m256i *) &pmem1[n+24], reg);
            _mm256_store_si256((__m256i *) &pmem2[n+24], reg);
        }
    } while (timestamp_diffnow(&now, TIME_SECS) < seconds);
}
