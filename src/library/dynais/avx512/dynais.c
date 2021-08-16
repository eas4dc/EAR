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

#include <stdlib.h>
#include <unistd.h>
#include <immintrin.h>
#include <library/dynais/dynais.h>
#include <library/dynais/avx512/dynais.h>
#include <library/dynais/avx512/dynais_core.h>

// General indexes.
ushort avx512_levels;
ushort avx512_window;
ushort avx512_topmos;
// Circular buffers
ushort *avx512_circular_samps[MAX_LEVELS];
ushort *avx512_circular_zeros[MAX_LEVELS];
ushort *avx512_circular_sizes[MAX_LEVELS];
ushort *avx512_circular_indxs[MAX_LEVELS];
ushort *avx512_circular_accus[MAX_LEVELS];
// Current data
 short avx512_current_resul[MAX_LEVELS];
ushort avx512_current_width[MAX_LEVELS];
ushort avx512_current_index[MAX_LEVELS];
ushort avx512_current_fight[MAX_LEVELS];
// Previous data
ushort avx512_previous_sizes[MAX_LEVELS];
ushort avx512_previous_width[MAX_LEVELS];
// Static replicas
__m512i zmmx31; // Ones
__m512i zmmx30; // 65535
__m512i zmmx29; // Shifts

static int avx512_dynais_alloc(ushort **c, size_t o)
{
	ushort *p;
	size_t t;
	int i;

	t = sizeof(short) * (avx512_window + o) * avx512_levels;
	if (posix_memalign((void *) &p, sizeof(__m512i), t) != 0) {
		return -1;
	}
	memset((void *) p, 0, t);
	for (i = 0; i < avx512_levels; ++i) {
		c[i] = &p[i * (avx512_window + o)];
	}
	return 0;
}

dynais_call_t avx512_dynais_init(ushort window, ushort levels)
{
	int i, k;

	uint multiple = window / 32;
	window = 32 * (multiple + 1);

	avx512_window = (window < METRICS_WINDOW) ? window : METRICS_WINDOW;
	avx512_levels = (levels < MAX_LEVELS)     ? levels : MAX_LEVELS;
	// Truncating level (at least 1)
	if (levels == 0) {
		levels = 1;
	}
	// Allocating space
	if (avx512_dynais_alloc(avx512_circular_samps, 00) != 0) return NULL;
	if (avx512_dynais_alloc(avx512_circular_sizes, 00) != 0) return NULL;
	if (avx512_dynais_alloc(avx512_circular_zeros, 32) != 0) return NULL;
	if (avx512_dynais_alloc(avx512_circular_indxs, 32) != 0) return NULL;
	if (avx512_dynais_alloc(avx512_circular_accus, 32) != 0) return NULL;
	// Filling index array
	for (i = 0; i < levels; ++i) {
		for (k = 0; k < avx512_window; ++k) {
			avx512_circular_indxs[i][k] = k - 1;
		}
		avx512_circular_indxs[i][0] = avx512_window - 1;
	}
	//
	static ushort shifts_array[32] __attribute__ ((aligned (64))) =
			{   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
			   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 31 };

	zmmx31 = _mm512_set1_epi16(1);
	//zmmx30 = _mm512_set1_epi16(65535);
	zmmx29 = _mm512_load_si512((__m512i *) &shifts_array);

	return avx512_dynais;
}

void avx512_dynais_dispose()
{
	free((void *) avx512_circular_samps[0]);
	free((void *) avx512_circular_zeros[0]);
	free((void *) avx512_circular_sizes[0]);
	free((void *) avx512_circular_indxs[0]);
	free((void *) avx512_circular_accus[0]);
}

// Returns the highest level.
static short avx512_dynais_hierarchical(ushort sample, ushort size, ushort level)
{
	if (level >= avx512_levels) {
		return level - 1;
	}
	// DynAIS basic algorithm call.
	if (level) avx512_dynais_core_n(sample, size, level);
	else       avx512_dynais_core_0(sample, size, level);
	// If new loop is detected, the sample and the size
	// is passed recursively to avx512_dynais_hierarchical.
	if (avx512_current_resul[level] >= NEW_LOOP) {
		return avx512_dynais_hierarchical(sample, avx512_previous_sizes[level], level + 1);
	}
	// If is not a NEW_LOOP.
	return level;
}

int avx512_dynais(uint sample, uint *size, uint *govern_level)
{
	short end_loop = 0;
	short reach;
	short l, ll;

	// Hierarchical algorithm call. The maximum level reached is returned. All
	// those values were updated by the basic DynAIS algorithm call.
	reach = avx512_dynais_hierarchical((ushort) sample, 1, 0);

	if (reach > avx512_topmos) {
		avx512_topmos = reach;
	}
	// Cleans didn't reach levels. Cleaning means previous loops with a state
	// greater than IN_LOOP have to be converted to IN_LOOP and also END_LOOP
	// have to be converted to NO_LOOP.
	for (l = avx512_topmos - 1; l > reach; --l) {
		if (avx512_current_resul[l] > IN_LOOP) avx512_current_resul[l] = IN_LOOP;
		if (avx512_current_resul[l] < NO_LOOP) avx512_current_resul[l] = NO_LOOP;
	}
	// After cleaning, the highest IN_LOOP or greater level is returned with its
	// state data. If an END_LOOP is detected before a NEW_LOOP, END_NEW_LOOP is
	// returned.
	for (l = avx512_topmos - 1; l >= 0; --l)
	{
		end_loop = end_loop | (avx512_current_resul[l] == END_LOOP);

		if (avx512_current_resul[l] >= IN_LOOP) {
			//*size = avx512_previous_width[l];
			*size = (uint) avx512_previous_sizes[l];
			*govern_level = (uint) l;

			// END_LOOP is detected above, it means that in this and below
			// levels the status is NEW_LOOP or END_NEW_LOOP, because the only
			// way to break a loop is with the detection of a new loop.
			if (end_loop) {
				return END_NEW_LOOP;
			}
			// If the status of this level is NEW_LOOP, it means that the status
			// in all below levels is NEW_LOOP or END_NEW_LOOP. If there is at
			// least one END_NEW_LOOP the END part have to be propagated to this
			// level.
			if (avx512_current_resul[l] == NEW_LOOP) {
				for (ll = l - 1; ll >= 0; --ll) {
					end_loop |= avx512_current_resul[ll] == END_NEW_LOOP;

					if (avx512_current_resul[ll] < NEW_LOOP) {
						return IN_LOOP;
					}
				}
			}
			if (end_loop) {
				return END_NEW_LOOP;
			}
			return (int) avx512_current_resul[l];
		}
	}
	// In case no loop were found: NO_LOOP or END_LOOP in level 0, size and
	// government level are 0.
	*govern_level = 0;
	*size         = 0;

	return (int) -end_loop;
}
