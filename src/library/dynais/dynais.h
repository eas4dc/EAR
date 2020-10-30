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

#ifndef EAR_DYNAIS_H
#define EAR_DYNAIS_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <immintrin.h>
#include <common/types/generic.h>
#include <common/config/config_install.h>

#define MAX_LEVELS      10
#define METRICS_WINDOW  40000

// DynAIS output states
#define END_LOOP       -1
#define NO_LOOP         0
#define IN_LOOP         1
#define NEW_ITERATION   2
#define NEW_LOOP        3
#define END_NEW_LOOP    4

#if FEAT_AVX512
#define udyn_t ushort
#define sdyn_t short
#else
#define udyn_t uint
#define sdyn_t int
#endif

// Functions
/** Given a sample and its size, returns the state the application is in (in
*   a loop, in an iteration, etc.). */
sdyn_t dynais(udyn_t sample, udyn_t *size, udyn_t *level);

/** Converts a long sample to short sample. */
udyn_t dynais_sample_convert(ulong sample);

int dynais_build_type();

/** Allocates memory in preparation to use dynais. Returns 0 on success */
int dynais_init(udyn_t window, udyn_t levels);

/** Frees the memory previously allocated. */
void dynais_dispose();

#endif //EAR_DYNAIS_H
