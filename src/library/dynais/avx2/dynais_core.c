#include <library/dynais/dynais.h>
#include <library/dynais/avx2/dynais_core.h>

// General indexes.
extern uint _levels;
extern uint _window;
extern uint _topmos;

// Circular buffers
extern uint *circ_samps[MAX_LEVELS];
extern uint *circ_zeros[MAX_LEVELS];
extern uint *circ_sizes[MAX_LEVELS];
extern uint *circ_indxs[MAX_LEVELS];
extern uint *circ_accus[MAX_LEVELS];

// Current and previous data
extern  int cur_resul[MAX_LEVELS];
extern uint cur_width[MAX_LEVELS];
extern uint cur_index[MAX_LEVELS];
extern uint cur_fight[MAX_LEVELS];
extern uint old_sizes[MAX_LEVELS];
extern uint old_width[MAX_LEVELS];

// Static replicas
extern __m256i zmmx31; // Ones
extern __m256i zmmx30; //
extern __m256i zmmx29; // Shifts

//
//
// Dynamic Application Iterative Structure Detection (DynAIS)
//
//

#ifdef DYN_CORE_N
void dynais_core_n(uint sample, uint size, uint level)
#else
void dynais_core_0(uint sample, uint size, uint level)
#endif
{
	__m256i zmmx00; // S
	__m256i zmmx04; // W
	__m256i zmmx08; // Z
	__m256i zmmx12; // I
	#ifdef DYN_CORE_N
	__m256i zmmx16; // A
	#endif
	__m256i zmmx28; // Replica S
	__m256i zmmx27; // Replica W
	__m256i zmmx26; // Maximum Z
	__m256i zmmx25; // Maximum I
	#ifdef DYN_CORE_N
	__m256i zmmx24; // Maximum A
	#endif

	__m256i mask00;
	__m256i mask01;
	__m256i mask02; // Macro aux
	__m256i mask03; // Macro aux
	__m256i mask04; // Macro aux (0xFF)

	// Basura
	uint *samp;
	uint *zero;
	uint *indx;
	uint *sizz;
	#ifdef DYN_CORE_N
	uint *accu;
	#endif

	uint index;
	uint i, k;

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
	zmmx28 = _mm256_set1_epi32(sample);
	zmmx27 = _mm256_set1_epi32(size);
	zmmx26 = _mm256_setzero_si256();
	zmmx25 = _mm256_set1_epi32(0x7FFFFFFF);
	#ifdef DYN_CORE_N
	zmmx24 = _mm256_set1_epi32(0x7FFFFFFF);
	#endif
	mask04 = _mm256_set1_epi32(0xFFFFFFFF);

	/* Outsiders */
	zero[_window] = zero[0];
	indx[_window] = indx[0];
	#ifdef DYN_CORE_N
	accu[_window] = accu[0];
	#endif

	/* Main iteration */
	for (k = 0, i = 8; k < _window; k += 8, i += 8)
	{
		zmmx00 = _mm256_load_si256((__m256i *) &samp[k]);
		zmmx04 = _mm256_load_si256((__m256i *) &sizz[k]);
		zmmx08 = _mm256_load_si256((__m256i *) &zero[k]);
		zmmx12 = _mm256_load_si256((__m256i *) &indx[k]);
		#ifdef DYN_CORE_N
		zmmx16 = _mm256_load_si256((__m256i *) &accu[k]);
		#endif

		/* Circular buffer processing */
		mask00 = _mm256_cmpeq_epi32(zmmx00, zmmx28);
		mask01 = _mm256_cmpeq_epi32(zmmx04, zmmx27);
		mask00 = _mm256_and_si256  (mask00, mask01);

		/* */
		zmmx08 = _mm256_permutevar8x32_epi32(zmmx08, zmmx29);
		zmmx12 = _mm256_permutevar8x32_epi32(zmmx12, zmmx29);
		#ifdef DYN_CORE_N
		zmmx16 = _mm256_permutevar8x32_epi32(zmmx16, zmmx29);
		#endif

		/* */
		zmmx08 = _mm256_insert_epi32(zmmx08, zero[i], 7);
		zmmx12 = _mm256_insert_epi32(zmmx12, indx[i], 7);
		#ifdef DYN_CORE_N
		zmmx16 = _mm256_insert_epi32(zmmx16, accu[i], 7);
		#endif

		zmmx08 = _mm256_add_epi32(zmmx08, zmmx31);
		zmmx08 = _mm256_and_si256(zmmx08, mask00);
		#ifdef DYN_CORE_N
		zmmx16 = _mm256_add_epi32(zmmx16, zmmx04);
		zmmx16 = _mm256_and_si256(zmmx16, mask00);
		#endif

		/* Data storing */
		_mm256_store_si256((__m256i *) &zero[k], zmmx08);
		_mm256_store_si256((__m256i *) &indx[k], zmmx12);
		#ifdef DYN_CORE_N
		_mm256_store_si256((__m256i *) &accu[k], zmmx16);
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
		_mm257_cmplt_epi32(zmmx12, zmmx08, mask01);

		if (_mm256_testz_si256(mask01, mask01) == 1) {
			continue;
		}

		_mm257_cmplt_epi32_mask(mask01, zmmx26, zmmx08, mask00);
		_mm257_cmpeq_epi32_mask(mask01, zmmx26, zmmx08, mask01);
		_mm257_cmplt_epi32_mask(mask01, zmmx25, zmmx12, mask01);
		mask00 = _mm256_or_si256(mask00, mask01);

		/* */
		zmmx26 = _mm257_blendv_epi32(zmmx26, zmmx08, mask00);
		zmmx25 = _mm257_blendv_epi32(zmmx25, zmmx12, mask00);
		#ifdef DYN_CORE_N
		zmmx24 = _mm257_blendv_epi32(zmmx24, zmmx16, mask00);
		#endif
	}

	uint l = level;
	uint cur_width_aux;
	uint cur_sizes_aux;
	uint cur_zeros_aux;
	uint res_nol;
	uint res_inl;
	uint res_ite;
	uint res_new;
	uint res_end;
	uint res_dif;
	uint *pz;
	uint *pw;
	uint *ps;

	pz = (uint *) &zmmx26;
	pw = (uint *) &zmmx25;
	#ifdef DYN_CORE_N
	ps = (uint *) &zmmx24;
	#endif
	cur_width_aux = 0;
	cur_zeros_aux = 0;
	#ifdef DYN_CORE_N
	cur_sizes_aux = 0x7FFFFFFF;
	#endif

	for (i = 0; i < 8; ++i)
	{
		if (pz[i] > cur_zeros_aux)
		{
			cur_width_aux = pw[i];
			cur_zeros_aux = pz[i];
		}
		#ifdef DYN_CORE_N
		if (pz[i] == cur_zeros_aux && ps[i] < cur_sizes_aux)
		{
			cur_sizes_aux = ps[i];
		}
		#endif
	}

	// New loop
	res_inl = (cur_zeros_aux >= _window);

	if (!res_inl)
	{
		//
		cur_width[l] = cur_width_aux;

		// New loop again
		res_inl = (cur_width[l] > 0) & (cur_zeros_aux > cur_width[l]);
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
		#ifdef DYN_CORE_N
		//old_sizes[l] = cur_sizes_aux - size;
		old_sizes[l] = cur_width[l];
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
}
