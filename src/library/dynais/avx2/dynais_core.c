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
#include <library/dynais/avx2/dynais_core.h>

// General indexes.
extern uint avx2_levels;
extern uint avx2_window;
extern uint avx2_topmos;
// Circular buffers
extern uint *avx2_circular_samps[MAX_LEVELS];
extern uint *avx2_circular_zeros[MAX_LEVELS];
extern uint *avx2_circular_sizes[MAX_LEVELS];
extern uint *avx2_circular_indxs[MAX_LEVELS];
extern uint *avx2_circular_accus[MAX_LEVELS];
// Current data
extern  int avx2_current_resul[MAX_LEVELS];
extern uint avx2_current_width[MAX_LEVELS];
extern uint avx2_current_index[MAX_LEVELS];
extern uint avx2_current_fight[MAX_LEVELS];
// Previous data
extern uint avx2_previous_sizes[MAX_LEVELS];
extern uint avx2_previous_width[MAX_LEVELS];
// Static replicas
extern __m256i ymmx31; // Ones
extern __m256i ymmx30; //
extern __m256i ymmx29; // Shifts

#ifdef DYN_CORE_N
void avx2_dynais_core_n(uint sample, uint size, uint level)
#else
void avx2_dynais_core_0(uint sample, uint size, uint level)
#endif
{
	__m256i ymmx00; // S
	__m256i ymmx04; // W
	__m256i ymmx08; // Z
	__m256i ymmx12; // I
	#ifdef DYN_CORE_N
	__m256i ymmx16; // A
	#endif
	__m256i ymmx28; // Replica S
	__m256i ymmx27; // Replica W
	__m256i ymmx26; // Maximum Z
	__m256i ymmx25; // Maximum I
	#ifdef DYN_CORE_N
	__m256i ymmx24; // Maximum A
	#endif
	__m256i mask00;
	__m256i mask01;
	__m256i mask02; // Macro aux
	__m256i mask03; // Macro aux
	__m256i mask04; // Macro aux (0xFF)
	// Others
	uint *p_samps;
	uint *p_zeros;
	uint *p_indxs;
	uint *p_sizes;
	#ifdef DYN_CORE_N
	uint *p_accus;
	#endif
	uint i, k, l;
	uint index;

	index = avx2_current_index[level];
	//
	p_samps = avx2_circular_samps[level];
	p_sizes = avx2_circular_sizes[level];
	p_zeros = avx2_circular_zeros[level];
	p_indxs = avx2_circular_indxs[level];
	#ifdef DYN_CORE_N
	p_accus = avx2_circular_accus[level];
	#endif
	p_samps[index] = 0;
	p_sizes[index] = 0;
	#ifdef DYN_CORE_N
	p_accus[index] = 0;
	#endif

	// Statics
	ymmx28 = _mm256_set1_epi32(sample);
	ymmx27 = _mm256_set1_epi32(size);
	ymmx26 = _mm256_setzero_si256();
	ymmx25 = _mm256_set1_epi32(0x7FFFFFFF);
	#ifdef DYN_CORE_N
	ymmx24 = _mm256_set1_epi32(0x7FFFFFFF);
	#endif
	mask04 = _mm256_set1_epi32(0xFFFFFFFF);

	/* Outsiders */
	p_zeros[avx2_window] = p_zeros[0];
	p_indxs[avx2_window] = p_indxs[0];
	#ifdef DYN_CORE_N
	p_accus[avx2_window] = p_accus[0];
	#endif

	/* Main iteration */
	for (k = 0, i = 8; k < avx2_window; k += 8, i += 8)
	{
		ymmx00 = _mm256_load_si256((__m256i *) &p_samps[k]);
		ymmx04 = _mm256_load_si256((__m256i *) &p_sizes[k]);
		ymmx08 = _mm256_load_si256((__m256i *) &p_zeros[k]);
		ymmx12 = _mm256_load_si256((__m256i *) &p_indxs[k]);
		#ifdef DYN_CORE_N
		ymmx16 = _mm256_load_si256((__m256i *) &p_accus[k]);
		#endif
		/* Circular buffer processing */
		mask00 = _mm256_cmpeq_epi32(ymmx00, ymmx28);
		mask01 = _mm256_cmpeq_epi32(ymmx04, ymmx27);
		mask00 = _mm256_and_si256  (mask00, mask01);
		/* */
		ymmx08 = _mm256_permutevar8x32_epi32(ymmx08, ymmx29);
		ymmx12 = _mm256_permutevar8x32_epi32(ymmx12, ymmx29);
		#ifdef DYN_CORE_N
		ymmx16 = _mm256_permutevar8x32_epi32(ymmx16, ymmx29);
		#endif
		/* */
		ymmx08 = _mm256_insert_epi32(ymmx08, p_zeros[i], 7);
		ymmx12 = _mm256_insert_epi32(ymmx12, p_indxs[i], 7);
		#ifdef DYN_CORE_N
		ymmx16 = _mm256_insert_epi32(ymmx16, p_accus[i], 7);
		#endif
		ymmx08 = _mm256_add_epi32(ymmx08, ymmx31);
		ymmx08 = _mm256_and_si256(ymmx08, mask00);
		#ifdef DYN_CORE_N
		ymmx16 = _mm256_add_epi32(ymmx16, ymmx04);
		ymmx16 = _mm256_and_si256(ymmx16, mask00);
		#endif
		/* Data storing */
		_mm256_store_si256((__m256i *) &p_zeros[k], ymmx08);
		_mm256_store_si256((__m256i *) &p_indxs[k], ymmx12);
		#ifdef DYN_CORE_N
		_mm256_store_si256((__m256i *) &p_accus[k], ymmx16);
		#endif

		#define _mm257_cmplt_epi32(src1, src2, dest) \
		{ \
			mask02 = _mm256_cmpgt_epi32(src1, src2); \
			mask03 = _mm256_cmpeq_epi32(src1, src2); \
			mask02 = _mm256_or_si256(mask02, mask03); \
			dest   = _mm256_andnot_si256(mask02, mask04); \
		}
		#define _mm257_cmpeq_epi32_mask(mask, src1, src2, dest)	\
		{ \
			mask02 = _mm256_cmpeq_epi32(src1, src2); \
			dest   = _mm256_and_si256(mask02, mask); \
		}
		#define _mm257_cmplt_epi32_mask(mask, src1, src2, dest)	\
		{ \
			mask02 = _mm256_cmpgt_epi32(src1, src2); \
			mask03 = _mm256_cmpeq_epi32(src1, src2); \
			mask02 = _mm256_or_si256(mask02, mask03); \
			mask02 = _mm256_andnot_si256(mask02, mask04); \
			dest   = _mm256_and_si256(mask02, mask); \
		}
		#define _mm257_blendv_epi32(src1, src2, mask) \
			(__m256i) _mm256_blendv_ps((__m256) src1, (__m256) src2, (__m256) mask);

		/* Maximum preparing */
		_mm257_cmplt_epi32(ymmx12, ymmx08, mask01);

		if (_mm256_testz_si256(mask01, mask01) == 1) {
			continue;
		}

		_mm257_cmplt_epi32_mask(mask01, ymmx26, ymmx08, mask00);
		_mm257_cmpeq_epi32_mask(mask01, ymmx26, ymmx08, mask01);
		_mm257_cmplt_epi32_mask(mask01, ymmx25, ymmx12, mask01);
		mask00 = _mm256_or_si256(mask00, mask01);
		/* */
		ymmx26 = _mm257_blendv_epi32(ymmx26, ymmx08, mask00);
		ymmx25 = _mm257_blendv_epi32(ymmx25, ymmx12, mask00);
		#ifdef DYN_CORE_N
		ymmx24 = _mm257_blendv_epi32(ymmx24, ymmx16, mask00);
		#endif
	}

	/*
	 *
	 * State logic
	 *
	 */
	uint result_noloop;
	uint result_inloop;
	uint result_newite;
	uint result_newlop;
	uint result_endlop;
	uint result_diflop;
	uint current_width;
	uint current_zeros;
	#ifdef DYN_CORE_N
	uint current_sizes;
	#endif
	uint *array_zeros;
	uint *array_width;
	#ifdef DYN_CORE_N
	uint *array_sizes;
	#endif

	// Using l to cut names
	l = level;
	// Initialiing
	current_width = 0;
	current_zeros = 0;
	#ifdef DYN_CORE_N
	current_sizes = 0x7FFFFFFF;
	#endif
	// Converting ymmx to array of ints
	array_zeros = (uint *) &ymmx26;
	array_width = (uint *) &ymmx25;
	#ifdef DYN_CORE_N
	array_sizes = (uint *) &ymmx24;
	#endif

	for (i = 0; i < 8; ++i)
	{
		if (array_zeros[i] > current_zeros) {
			current_width = array_width[i];
			current_zeros = array_zeros[i];
		}
		#ifdef DYN_CORE_N
		if (array_zeros[i] == current_zeros && array_sizes[i] < current_sizes) {
			current_sizes = array_sizes[i];
		}
		#endif
	}

	// New loop
	result_inloop = (current_zeros >= avx2_window);

	if (!result_inloop) {
		//
		avx2_current_width[l] = current_width;
		// New loop again
		result_inloop = (avx2_current_width[l] > 0) & (current_zeros > avx2_current_width[l]);
	}
	// New no loop
	result_noloop = !result_inloop;
	// New different loop
	result_diflop = avx2_previous_width[l] != avx2_current_width[l];
	// Array in loop counter
	if (result_diflop || avx2_current_fight[l] == avx2_current_width[l]) {
		avx2_current_fight[l] = 0;
	}
	// New iteration
	result_newite = result_inloop && (avx2_current_width[l] == 1 || avx2_current_fight[l] == 0);
	avx2_current_fight[l] += result_inloop;
	// New loop
	result_newlop = result_newite & (avx2_previous_width[l] != avx2_current_width[l]);
	// New end-new loop
	result_endlop = ((avx2_previous_width[l] != avx2_current_width[l]) && avx2_previous_width[l] != 0);

	i = (index + avx2_current_width[l]) % avx2_window;
	k = (index);

	if (result_newlop) {
		#ifdef DYN_CORE_N
		//avx2_previous_sizes[l] = current_sizes - size;
		avx2_previous_sizes[l] = avx2_current_width[l];
		#else
		avx2_previous_sizes[l] = avx2_current_width[l];
		#endif
		avx2_previous_width[l] = avx2_current_width[l];
	}

	if (result_noloop) {
		avx2_current_fight[l] = 0;
		avx2_previous_width[l] = 0;
	}
	// Level result
	avx2_current_resul[l] = 0;
	avx2_current_resul[l] -= (!result_inloop) & result_endlop;	// -1 = end lopp
	avx2_current_resul[l] += result_inloop;						//  1 = in loop
	avx2_current_resul[l] += result_newite;						//  2 = new iteration
	avx2_current_resul[l] += result_newlop;						//  3 = new loop
	avx2_current_resul[l] += result_newlop & result_endlop;		//  4 = end and new loop
	// Cleaning
	p_samps[index] = sample;
	p_sizes[index] = size;

	if (index == 0) {
		index = avx2_window;
	}
	index = index - 1;
	//
	avx2_current_index[level] = index;
}
