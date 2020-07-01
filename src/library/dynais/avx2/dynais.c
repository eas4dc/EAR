/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
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

#include <library/dynais/dynais.h>
#include <library/dynais/avx2/dynais_core.h>

// General indexes.
uint _levels;
uint _window;
uint _topmos;

// Circular buffers
uint *circ_samps[MAX_LEVELS];
uint *circ_zeros[MAX_LEVELS];
uint *circ_sizes[MAX_LEVELS];
uint *circ_indxs[MAX_LEVELS];
uint *circ_accus[MAX_LEVELS];

// Current and previous data
 int cur_resul[MAX_LEVELS];
uint cur_width[MAX_LEVELS];
uint cur_index[MAX_LEVELS];
uint cur_fight[MAX_LEVELS];
uint old_sizes[MAX_LEVELS];
uint old_width[MAX_LEVELS];

// Static replicas
__m256i zmmx31; // Ones
__m256i zmmx30; //
__m256i zmmx29; // Shifts

//
//
// Dynamic Application Iterative Structure Detection (DynAIS)
//
//

static int dynais_alloc(uint **c, size_t o)
{
	uint *p;
	int i;

	o = sizeof(int) * (_window + o) * _levels;

	if (posix_memalign((void *) &p, sizeof(__m256i), sizeof(int) * (_window + o) * _levels) != 0) {
		return -1;
	}

	memset((void *) p, 0, sizeof(int) * (_window + o) * _levels);

	for (i = 0; i < _levels; ++i) {
		c[i] = &p[i * (_window + o)];
	}

	return 0;
}

int dynais_init(uint window, uint levels)
{
	int i, k;

	unsigned int multiple = window / 16;
	window = 16 * (multiple + 1);

	_window = (window < METRICS_WINDOW) ? window : METRICS_WINDOW;
	_levels = (levels < MAX_LEVELS) ? levels : MAX_LEVELS;

	if (dynais_alloc(circ_samps, 00) != 0) return -1;
	if (dynais_alloc(circ_sizes, 00) != 0) return -1;
	if (dynais_alloc(circ_zeros, 16) != 0) return -1;
	if (dynais_alloc(circ_indxs, 16) != 0) return -1;
	if (dynais_alloc(circ_accus, 16) != 0) return -1;

	// Filling index array
	for (i = 0; i < levels; ++i) {
		for (k = 0; k < _window; ++k) {
			circ_indxs[i][k] = k - 1;
		}

		circ_indxs[i][0] = _window - 1;
	}

	//
	static uint shifts_array[16] __attribute__ ((aligned (64))) =
			{  1,  2,  3,  4,  5,  6,  7,  7 };

	zmmx31 = _mm256_set1_epi32(1);
	//zmmx30 = _mm512_set1_epi16(65535);
	zmmx29 = _mm256_load_si256((__m256i *) &shifts_array);

	return 0;
}

void dynais_dispose()
{
	free((void *) circ_samps[0]);
	free((void *) circ_zeros[0]);
	free((void *) circ_sizes[0]);
	free((void *) circ_indxs[0]);
	free((void *) circ_accus[0]);
}

int dynais_build_type()
{
	// AVX 512
	return 1;
}

// Returns the highest level.
static int dynais_hierarchical(uint sample, uint size, uint level)
{
	if (level >= _levels) {
		return level - 1;
	}

	// DynAIS basic algorithm call.
	if (level) dynais_core_n(sample, size, level);
	else dynais_core_0(sample, size, level);

	// If new loop is detected, the sample and the size
	// is passed recursively to dynais_hierarchical.
	if (cur_resul[level] >= NEW_LOOP) {
		return dynais_hierarchical(sample, old_sizes[level], level + 1);
	}

	// If is not a NEW_LOOP.
	return level;
}

int dynais(uint sample, uint *size, uint *govern_level)
{
	int end_loop = 0;
	int reach;
	int l, ll;
	
	// Hierarchical algorithm call. The maximum level
	// reached is returned. All those values were updated
	// by the basic DynAIS algorithm call.
	reach = dynais_hierarchical(sample, 1, 0);

	if (reach > _topmos) {
		_topmos = reach;
	}

	// Cleans didn't reach levels. Cleaning means previous
	// loops with a state greater than IN_LOOP have to be
	// converted to IN_LOOP and also END_LOOP have to be
	// converted to NO_LOOP.
	for (l = _topmos - 1; l > reach; --l) {
		if (cur_resul[l] > IN_LOOP) cur_resul[l] = IN_LOOP;
		if (cur_resul[l] < NO_LOOP) cur_resul[l] = NO_LOOP;
	}

	// After cleaning, the highest IN_LOOP or greater
	// level is returned with its state data. If an
	// END_LOOP is detected before a NEW_LOOP,
	// END_NEW_LOOP is returned.
	for (l = _topmos - 1; l >= 0; --l)
	{
		end_loop = end_loop | (cur_resul[l] == END_LOOP);

		if (cur_resul[l] >= IN_LOOP) {
			*govern_level = l;
			//*size = old_width[l];
			*size = old_sizes[l];

			// END_LOOP is detected above, it means that in this and
			// below levels the status is NEW_LOOP or END_NEW_LOOP,
			// because the only way to break a loop is with the
			// detection of a new loop.
			if (end_loop) {
				return END_NEW_LOOP;
			}

			// If the status of this level is NEW_LOOP, it means
			// that the status in all below levels is NEW_LOOP or
			// END_NEW_LOOP. If there is at least one END_NEW_LOOP
			// the END part have to be propagated to this level.
			if (cur_resul[l] == NEW_LOOP) {
				for (ll = l - 1; ll >= 0; --ll) {
					end_loop |= cur_resul[ll] == END_NEW_LOOP;

					if (cur_resul[ll] < NEW_LOOP) {
						return IN_LOOP;
					}
				}
			}

			if (end_loop) {
				return END_NEW_LOOP;
			}

			return cur_resul[l];
		}
	}


	// In case no loop were found: NO_LOOP or END_LOOP
	// in level 0, size and government level are 0.
	*size = 0;
	*govern_level = 0;

	return -end_loop;
}

uint dynais_sample_convert(ulong sample)
{
	uint *p = (uint *) &sample;
	p[0] = _mm_crc32_u32(p[0], p[1]);
	return (uint) p[0];
}
