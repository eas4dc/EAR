#include <library/dynais/dynais.h>
#include <library/dynais/avx512/dynais_core.h>

// General indexes.
extern ushort _levels;
extern ushort _window;
extern ushort _topmos;

// Circular buffers
extern ushort *circ_samps[MAX_LEVELS];
extern ushort *circ_zeros[MAX_LEVELS];
extern ushort *circ_sizes[MAX_LEVELS];
extern ushort *circ_indxs[MAX_LEVELS];
extern ushort *circ_accus[MAX_LEVELS];

// Current and previous data
extern  short cur_resul[MAX_LEVELS];
extern ushort cur_width[MAX_LEVELS];
extern ushort cur_index[MAX_LEVELS];
extern ushort cur_fight[MAX_LEVELS];
extern ushort old_sizes[MAX_LEVELS];
extern ushort old_width[MAX_LEVELS];

// Static replicas
extern __m512i zmmx31; // Ones
extern __m512i zmmx30; //
extern __m512i zmmx29; // Shifts

//
//
// Dynamic Application Iterative Structure Detection (DynAIS)
//
//

#ifdef DYN_CORE_N
void dynais_core_n(ushort sample, ushort size, ushort level)
#else
void dynais_core_0(ushort sample, ushort size, ushort level)
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

	// Basura
	unsigned short *samp;
	unsigned short *zero;
	unsigned short *indx;
	unsigned short *sizz;
	#ifdef DYN_CORE_N
	unsigned short *accu;
	#endif

	//  signed short resul;
	unsigned short index;
	unsigned short i, k;

	index = cur_index[level];

	samp = circ_samps[level];
	sizz = circ_sizes[level];
	zero = circ_zeros[level];
	indx = circ_indxs[level];
	#ifdef DYN_CORE_N
	accu = circ_accus[level];
	#endif

	samp[index] = 0;
	sizz[index] = 0;
	#ifdef DYN_CORE_N
	accu[index] = 0;
	#endif

	// Statics
	zmmx28 = _mm512_set1_epi16(sample);
	zmmx27 = _mm512_set1_epi16(size);
	zmmx26 = _mm512_setzero_si512();
	zmmx25 = _mm512_set1_epi16(0xFFFF);

	/* Outsiders */
	zero[_window] = zero[0];
	indx[_window] = indx[0];
	#ifdef DYN_CORE_N
	accu[_window] = accu[0];
	#endif

	/* Main iteration */
	for (k = 0, i = 32; k < _window; k += 32, i += 32)
	{
    	zmmx00 = _mm512_load_si512((__m512i *) &samp[k]);
    	zmmx04 = _mm512_load_si512((__m512i *) &sizz[k]);
    	zmmx08 = _mm512_load_si512((__m512i *) &zero[k]);
    	zmmx12 = _mm512_load_si512((__m512i *) &indx[k]);
    	#ifdef DYN_CORE_N
    	zmmx16 = _mm512_load_si512((__m512i *) &accu[k]);
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
		zmmx08 = _mm512_mask_set1_epi16(zmmx08, 0x80000000, zero[i]); // C3
		zmmx12 = _mm512_mask_set1_epi16(zmmx12, 0x80000000, indx[i]); // D3
		#ifdef DYN_CORE_N
		zmmx16 = _mm512_mask_set1_epi16(zmmx16, 0x80000000, accu[i]); // D3
		#endif

		/* */
		zmmx08 = _mm512_maskz_adds_epu16(mask00, zmmx08, zmmx31);
		#ifdef DYN_CORE_N
		zmmx16 = _mm512_maskz_adds_epu16(mask00, zmmx16, zmmx04);
		#endif

		/* Data storing */
		_mm512_store_si512((__m512i *) &zero[k], zmmx08);
		_mm512_store_si512((__m512i *) &indx[k], zmmx12);
		#ifdef DYN_CORE_N
		_mm512_store_si512((__m512i *) &accu[k], zmmx16);
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

	ushort res_nol;
	ushort res_inl;
	ushort res_ite;
	ushort res_new;
	ushort res_end;
	ushort res_dif;
	ushort cur_zeros;
	ushort l = level;

	// Maximum zero
	zmmx00 = _mm512_srli_epi32(zmmx26, 0x10);
	zmmx04 = _mm512_mask_set1_epi16(zmmx26, 0xAAAAAAAA, 0);
	zmmx08 = _mm512_max_epu16(zmmx00, zmmx04);
	mask01 = _mm512_reduce_max_epu32(zmmx08);
	cur_zeros = (ushort) mask01;

	// New loop
	res_inl = (cur_zeros >= _window);

	if (!res_inl)
	{
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
		cur_width[l] = (ushort) mask00;

		// New loop again
		res_inl = (cur_width[l] > 0) & (cur_zeros > cur_width[l]);
	}

	// New no loop
	res_nol = !res_inl;

	// New different loop
	res_dif = old_width[l] != cur_width[l];

	// Array in loop counter
	if (res_dif || cur_fight[l] == cur_width[l]) {
		cur_fight[l] = 0;
	}

	// New iteration
	res_ite = res_inl && (cur_width[l] == 1 || cur_fight[l] == 0);
	
	cur_fight[l] += res_inl;

	// New loop
	res_new = res_ite & (old_width[l] != cur_width[l]);

	// New end-new loop
	res_end = ((old_width[l] != cur_width[l]) && old_width[l] != 0);

	i = (index + cur_width[l]) % _window;
	k = index;

	if (res_new)
	{
		// Minimum index size
		#ifdef DYN_CORE_N
		zmmx24 = _mm512_maskz_srli_epi16(mask02, zmmx24, 0);
		mask02 = _mm512_reduce_max_epu32(zmmx24);

		if ((unsigned int) mask02 > 65535) {
			mask02 = mask02 >> 16;
		}
		#endif
		#ifdef DYN_CORE_N
		old_sizes[l] = (unsigned short) mask02 - size;
		#else
		old_sizes[l] = cur_width[l];
		#endif
		old_width[l] = cur_width[l];
	}

	if (res_nol)
	{
		cur_fight[l] = 0;
		old_width[l] = 0;
	}

	// Level result
	cur_resul[l] = 0;
	cur_resul[l] -= (!res_inl) & res_end;	// -1 = end lopp
	cur_resul[l] += res_inl;				// 1 = in loop
	cur_resul[l] += res_ite;				// 2 = new iteration
	cur_resul[l] += res_new;				// 3 = new loop
	cur_resul[l] += res_new & res_end;	// 4 = end and new loop

	// Cleaning
	samp[index] = sample;
	sizz[index] = size;

	if (index == 0) index = _window;
	index = index - 1;
	
	cur_index[level] = index;
	// Linea causante de problemas, investigar de donde ha salido	
	// cur_resul[level] = resul;
}
