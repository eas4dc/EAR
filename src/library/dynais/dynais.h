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

/*
 * Usage summary:
 * Just call dynais() passing the sample and the size of this sample. It will be
 * returned one of these states:
 *      END_LOOP
 *      NO_LOOP
 *      IN_LOOP
 *      NEW_ITERATION
 *      NEW_LOOP
 *      END_NEW_LOOP
 *
 * To initialize, you have to call dynais_init() method before, passing a
 * topology, window length and number of levels. The function
 * dynais_dispose() frees its memory allocation.
 *
 * Level is capped to a maximum of 10, and window to 40.000. But it is
 * recommended to set a window of 500 at most to perform at its best.
 *
 * Errors:
 * A NULL returned by dynais_init() means that something went wrong while
 * allocating memory.
 *
 */

#ifndef DYNAIS_H
#define DYNAIS_H

#include <common/types/generic.h>
#include <common/hardware/topology.h>

#define MAX_LEVELS      10
#define METRICS_WINDOW  40000

// DynAIS output states
#define END_LOOP       -1
#define NO_LOOP         0
#define IN_LOOP         1
#define NEW_ITERATION   2
#define NEW_LOOP        3
#define END_NEW_LOOP    4

typedef int (*dynais_call_t) (uint sample, uint *size, uint *level);

// Returns a dynais_call_t type. It is a pointer a specific AVX dynais call.
dynais_call_t dynais_init(topology_t *tp, uint window, uint levels);

void dynais_dispose();

// Returns DynAIS type. 512 if it's AVX512, 2 if it's AVX2.
int dynais_build_type();

// Applies CRC to 64 bits sample, converting it to 32 bit value.
uint dynais_sample_convert(ulong sample);

#endif //DYNAIS_H
