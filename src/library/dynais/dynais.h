/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
#define DYNAIS_SVE      4
#define DYNAIS_AVX512   3
#define DYNAIS_AVX2     2
#define DYNAIS_DUMMY    1
#define DYNAIS_NONE     0 

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
