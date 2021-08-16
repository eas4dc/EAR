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
#include <library/dynais/avx512/dynais_core.h>

// General indexes.
extern ushort avx512_levels;
extern ushort avx512_window;
extern ushort avx512_topmos;
// Circular buffers
extern ushort *avx512_circular_samps[MAX_LEVELS];
extern ushort *avx512_circular_zeros[MAX_LEVELS];
extern ushort *avx512_circular_sizes[MAX_LEVELS];
extern ushort *avx512_circular_indxs[MAX_LEVELS];
extern ushort *avx512_circular_accus[MAX_LEVELS];
// Current data
extern  short avx512_current_resul[MAX_LEVELS];
extern ushort avx512_current_width[MAX_LEVELS];
extern ushort avx512_current_index[MAX_LEVELS];
extern ushort avx512_current_fight[MAX_LEVELS];
// Previous data
extern ushort avx512_previous_sizes[MAX_LEVELS];
extern ushort avx512_previous_width[MAX_LEVELS];
// Static replicas
extern __m512i zmmx31; // Ones
extern __m512i zmmx30; //
extern __m512i zmmx29; // Shifts

//
// Dynamic Application Iterative Structure Detection (DynAIS)
//
#ifdef DYN_CORE_N
void avx512_dynais_core_n(ushort sample, ushort size, ushort level)
#else
void avx512_dynais_core_0(ushort sample, ushort size, ushort level)
#endif
{
	__m512i zmmx00; // S
	__m512i zmmx04; // W
	__m512i zmmx08; // Z
	__m512i zmmx12; // I
	#ifdef DYN_CORE_N
	__m512i zmmx16; // A
	#endif
	__m512i zmmx28; // Replica S
	__m512i zmmx27; // Replica W
	__m512i zmmx26; // Maximum Z
	__m512i zmmx25; // Maximum I
	#ifdef DYN_CORE_N
	__m512i zmmx24; // Maximum A
	#endif
	__mmask32 mask00;
	__mmask32 mask01;
	__mmask32 mask02;
	//
	ushort *p_samps;
	ushort *p_zeros;
	ushort *p_indxs;
	ushort *p_sizes;
	#ifdef DYN_CORE_N
	ushort *p_accus;
	#endif
	ushort i, k, l;
	ushort index;

	index = avx512_current_index[level];
	//
	p_samps = avx512_circular_samps[level];
	p_sizes = avx512_circular_sizes[level];
	p_zeros = avx512_circular_zeros[level];
	p_indxs = avx512_circular_indxs[level];
	#ifdef DYN_CORE_N
	p_accus = avx512_circular_accus[level];
	#endif

	p_samps[index] = 0;
	p_sizes[index] = 0;
	#ifdef DYN_CORE_N
	p_accus[index] = 0;
	#endif

	// Statics
	zmmx28 = _mm512_set1_epi16(sample);
	zmmx27 = _mm512_set1_epi16(size);
	zmmx26 = _mm512_setzero_si512();
	zmmx25 = _mm512_set1_epi16(0xFFFF);
	/* Outsiders */
	p_zeros[avx512_window] = p_zeros[0];
	p_indxs[avx512_window] = p_indxs[0];
	#ifdef DYN_CORE_N
	p_accus[avx512_window] = p_accus[0];
	#endif

	/* Main iteration */
	for (k = 0, i = 32; k < avx512_window; k += 32, i += 32)
	{
    	zmmx00 = _mm512_load_si512((__m512i *) &p_samps[k]);
    	zmmx04 = _mm512_load_si512((__m512i *) &p_sizes[k]);
    	zmmx08 = _mm512_load_si512((__m512i *) &p_zeros[k]);
    	zmmx12 = _mm512_load_si512((__m512i *) &p_indxs[k]);
    	#ifdef DYN_CORE_N
    	zmmx16 = _mm512_load_si512((__m512i *) &p_accus[k]);
    	#endif
		/* Circular buffer processing */
		mask00 = _mm512_cmp_epu16_mask(zmmx00, zmmx28, _MM_CMPINT_EQ);
		mask00 = _mm512_mask_cmp_epu16_mask(mask00, zmmx04, zmmx27, _MM_CMPINT_EQ);
		/* */
		zmmx08 = _mm512_permutexvar_epi16(zmmx29, zmmx08);
		zmmx12 = _mm512_permutexvar_epi16(zmmx29, zmmx12);
		#ifdef DYN_CORE_N
		zmmx16 = _mm512_permutexvar_epi16(zmmx29, zmmx16);
		#endif
		/* */
		zmmx08 = _mm512_mask_set1_epi16(zmmx08, 0x80000000, p_zeros[i]); // C3
		zmmx12 = _mm512_mask_set1_epi16(zmmx12, 0x80000000, p_indxs[i]); // D3
		#ifdef DYN_CORE_N
		zmmx16 = _mm512_mask_set1_epi16(zmmx16, 0x80000000, p_accus[i]); // D3
		#endif
		/* */
		zmmx08 = _mm512_maskz_adds_epu16(mask00, zmmx08, zmmx31);
		#ifdef DYN_CORE_N
		zmmx16 = _mm512_maskz_adds_epu16(mask00, zmmx16, zmmx04);
		#endif
		/* Data storing */
		_mm512_store_si512((__m512i *) &p_zeros[k], zmmx08);
		_mm512_store_si512((__m512i *) &p_indxs[k], zmmx12);
		#ifdef DYN_CORE_N
		_mm512_store_si512((__m512i *) &p_accus[k], zmmx16);
		#endif
		/* Maximum preparing */
		mask01 = _mm512_cmp_epu16_mask(zmmx12, zmmx08, _MM_CMPINT_LT);

		if (mask01 == 0) {
			continue;
		}
		mask00 = _mm512_mask_cmp_epu16_mask(mask01, zmmx26, zmmx08, _MM_CMPINT_LT);
		mask01 = _mm512_mask_cmp_epu16_mask(mask01, zmmx26, zmmx08, _MM_CMPINT_EQ);
		mask01 = _mm512_mask_cmp_epu16_mask(mask01, zmmx25, zmmx12, _MM_CMPINT_LT);
		mask00 = mask00 | mask01;
		/* */
		zmmx26 = _mm512_mask_mov_epi16(zmmx26, mask00, zmmx08);
		zmmx25 = _mm512_mask_mov_epi16(zmmx25, mask00, zmmx12);
		#ifdef DYN_CORE_N
		zmmx24 = _mm512_setzero_si512();
		zmmx24 = _mm512_mask_mov_epi16(zmmx24, mask00, zmmx16);
		#endif
	}

	/*
	 *
	 * State logic
	 *
	 */
	ushort result_noloop;
	ushort result_inloop;
	ushort result_newite;
	ushort result_newlop;
	ushort result_endlop;
	ushort result_diflop;
	ushort current_zeros;

	// Using l to cut names
	l = level;
	// Maximum zero
	zmmx00 = _mm512_srli_epi32(zmmx26, 0x10);
	zmmx04 = _mm512_mask_set1_epi16(zmmx26, 0xAAAAAAAA, 0);
	zmmx08 = _mm512_max_epu16(zmmx00, zmmx04);
	mask01 = _mm512_reduce_max_epu32(zmmx08);
	current_zeros = (ushort) mask01;
	// New loop
	result_inloop = (current_zeros >= avx512_window);

	if (!result_inloop) {
		// Minimum index
		zmmx08 = _mm512_set1_epi16(mask01);
		mask02 = _mm512_cmp_epu16_mask(zmmx08, zmmx26, _MM_CMPINT_EQ);
		zmmx25 = _mm512_maskz_srli_epi16(mask02, zmmx25, 0);
		mask00 = _mm512_reduce_max_epu32(zmmx25);
		// Shift to 16-bits
		if ((unsigned int) mask00 > 65535) {
			mask00 = mask00 >> 16;
		}
		// Array width
		avx512_current_width[l] = (ushort) mask00;
		// New loop again
		result_inloop = (avx512_current_width[l] > 0) & (current_zeros > avx512_current_width[l]);
	}
	// New no loop
	result_noloop = !result_inloop;
	// New different loop
	result_diflop = avx512_previous_width[l] != avx512_current_width[l];
	// Array in loop counter
	if (result_diflop || avx512_current_fight[l] == avx512_current_width[l]) {
		avx512_current_fight[l] = 0;
	}
	// New iteration
	result_newite = result_inloop && (avx512_current_width[l] == 1 || avx512_current_fight[l] == 0);
	//
	avx512_current_fight[l] += result_inloop;
	// New loop
	result_newlop = result_newite & (avx512_previous_width[l] != avx512_current_width[l]);
	// New end-new loop
	result_endlop = ((avx512_previous_width[l] != avx512_current_width[l]) && avx512_previous_width[l] != 0);
	//
	i = (index + avx512_current_width[l]) % avx512_window;
	k = (index);

	if (result_newlop) {
		// Minimum index size
		#ifdef DYN_CORE_N
		zmmx24 = _mm512_maskz_srli_epi16(mask02, zmmx24, 0);
		mask02 = _mm512_reduce_max_epu32(zmmx24);

		if ((unsigned int) mask02 > 65535) {
			mask02 = mask02 >> 16;
		}
		#endif
		#ifdef DYN_CORE_N
		avx512_previous_sizes[l] = (ushort) mask02 - size;
		#else
		avx512_previous_sizes[l] = avx512_current_width[l];
		#endif
		avx512_previous_width[l] = avx512_current_width[l];
	}
	if (result_noloop) {
		avx512_current_fight[l] = 0;
		avx512_previous_width[l] = 0;
	}
	// Level result
	avx512_current_resul[l]  = 0;
	avx512_current_resul[l] -= (!result_inloop) & result_endlop;	// -1 = end lopp
	avx512_current_resul[l] += result_inloop;						//  1 = in loop
	avx512_current_resul[l] += result_newite;						//  2 = new iteration
	avx512_current_resul[l] += result_newlop;						//  3 = new loop
	avx512_current_resul[l] += result_newlop & result_endlop;		//  4 = end and new loop
	// Cleaning
	p_samps[index] = sample;
	p_sizes[index] = size;
	//
	if (index == 0) {
		index = avx512_window;
	}
	index = index - 1;
	//
	avx512_current_index[level] = index;
}
