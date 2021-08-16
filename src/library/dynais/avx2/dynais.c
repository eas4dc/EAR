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
#include <string.h>
#include <unistd.h>
#include <immintrin.h>
#include <library/dynais/dynais.h>
#include <library/dynais/avx2/dynais.h>
#include <library/dynais/avx2/dynais_core.h>

// General indexes.
uint avx2_levels;
uint avx2_window;
uint avx2_topmos;
// Circular buffers
uint *avx2_circular_samps[MAX_LEVELS];
uint *avx2_circular_zeros[MAX_LEVELS];
uint *avx2_circular_sizes[MAX_LEVELS];
uint *avx2_circular_indxs[MAX_LEVELS];
uint *avx2_circular_accus[MAX_LEVELS];
// Current data
 int avx2_current_resul[MAX_LEVELS];
uint avx2_current_width[MAX_LEVELS];
uint avx2_current_index[MAX_LEVELS];
uint avx2_current_fight[MAX_LEVELS];
// Previous data
uint avx2_previous_sizes[MAX_LEVELS];
uint avx2_previous_width[MAX_LEVELS];
// Static replicas
__m256i ymmx31; // Ones
__m256i ymmx30; //
__m256i ymmx29; // Shifts

static int avx2_dynais_alloc(uint **c, size_t o)
{
	uint *p;
	size_t t;
	int i;

	t = sizeof(int) * (avx2_window + o) * avx2_levels;
	if (posix_memalign((void *) &p, sizeof(__m256i), t) != 0) {
		return -1;
	}
	memset((void *) p, 0, t);
	for (i = 0; i < avx2_levels; ++i) {
		c[i] = &p[i * (avx2_window + o)];
	}

	return 0;
}

dynais_call_t avx2_dynais_init(uint window, uint levels)
{
	int i, k;

	unsigned int multiple = window / 16;
	window = 16 * (multiple + 1);

	avx2_window = (window < METRICS_WINDOW) ? window : METRICS_WINDOW;
	avx2_levels = (levels < MAX_LEVELS)     ? levels : MAX_LEVELS;

	if (avx2_dynais_alloc(avx2_circular_samps, 00) != 0) return NULL;
	if (avx2_dynais_alloc(avx2_circular_sizes, 00) != 0) return NULL;
	if (avx2_dynais_alloc(avx2_circular_zeros, 16) != 0) return NULL;
	if (avx2_dynais_alloc(avx2_circular_indxs, 16) != 0) return NULL;
	if (avx2_dynais_alloc(avx2_circular_accus, 16) != 0) return NULL;

	// Filling index array
	for (i = 0; i < levels; ++i) {
		for (k = 0; k < avx2_window; ++k) {
			avx2_circular_indxs[i][k] = k - 1;
		}
		avx2_circular_indxs[i][0] = avx2_window - 1;
	}

	//
	static uint shifts_array[16] __attribute__ ((aligned (64))) =
			{  1,  2,  3,  4,  5,  6,  7,  7 };

	ymmx31 = _mm256_set1_epi32(1);
	//ymmx30 = _mm512_set1_epi16(65535);
	ymmx29 = _mm256_load_si256((__m256i *) &shifts_array);

	return avx2_dynais;
}

void avx2_dynais_dispose()
{
	free((void *) avx2_circular_samps[0]);
	free((void *) avx2_circular_zeros[0]);
	free((void *) avx2_circular_sizes[0]);
	free((void *) avx2_circular_indxs[0]);
	free((void *) avx2_circular_accus[0]);
}

// Returns the highest level.
static int avx2_dynais_hierarchical(uint sample, uint size, uint level)
{
	if (level >= avx2_levels) {
		return level - 1;
	}
	// DynAIS basic algorithm call.
	if (level) avx2_dynais_core_n(sample, size, level);
	else       avx2_dynais_core_0(sample, size, level);
	// If new loop is detected, the sample and the size
	// is passed recursively to avx2_dynais_hierarchical.
	if (avx2_current_resul[level] >= NEW_LOOP) {
		return avx2_dynais_hierarchical(sample, avx2_previous_sizes[level], level + 1);
	}
	// If is not a NEW_LOOP.
	return level;
}

int avx2_dynais(uint sample, uint *size, uint *govern_level)
{
	int end_loop = 0;
	int reach;
	int l, ll;
	
	// Hierarchical algorithm call. The maximum level reached is returned. All
	// those values were updated by the basic DynAIS algorithm call.
	reach = avx2_dynais_hierarchical(sample, 1, 0);

	if (reach > avx2_topmos) {
		avx2_topmos = reach;
	}
	// Cleans didn't reach levels. Cleaning means previous loops with a state
	// greater than IN_LOOP have to be converted to IN_LOOP and also END_LOOP
	// have to be converted to NO_LOOP.
	for (l = avx2_topmos - 1; l > reach; --l) {
		if (avx2_current_resul[l] > IN_LOOP) avx2_current_resul[l] = IN_LOOP;
		if (avx2_current_resul[l] < NO_LOOP) avx2_current_resul[l] = NO_LOOP;
	}
	// After cleaning, the highest IN_LOOP or greater level is returned with its
	// state data. If an END_LOOP is detected before a NEW_LOOP, END_NEW_LOOP is
	// returned.
	for (l = avx2_topmos - 1; l >= 0; --l)
	{
		end_loop = end_loop | (avx2_current_resul[l] == END_LOOP);

		if (avx2_current_resul[l] >= IN_LOOP) {
			//*size = avx2_previous_width[l];
			*size = avx2_previous_sizes[l];
			*govern_level = l;

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
			if (avx2_current_resul[l] == NEW_LOOP) {
				for (ll = l - 1; ll >= 0; --ll) {
					end_loop |= avx2_current_resul[ll] == END_NEW_LOOP;

					if (avx2_current_resul[ll] < NEW_LOOP) {
						return IN_LOOP;
					}
				}
			}
			if (end_loop) {
				return END_NEW_LOOP;
			}
			return avx2_current_resul[l];
		}
	}
	// In case no loop were found: NO_LOOP or END_LOOP in level 0, size and
	// government level are 0.
	*govern_level = 0;
	*size         = 0;

	return -end_loop;
}
