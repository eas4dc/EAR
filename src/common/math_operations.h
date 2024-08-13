/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_MATH_OPS_H
#define COMMON_MATH_OPS_H

#include <stdlib.h>
#include <limits.h>

#include <common/types/generic.h>

#define B_TO_KB        1E3
#define B_TO_MB        1E6
#define B_TO_GB        1E9
#define KB_TO_MB(n)    (n / 1000)
#define KB_TO_HB(n)    (n / 100000) //Hundred of megahertz
#define MB_TO_KB(n)    (n * 1000) 
#define HB_TO_KB(n)    (n * 100000)
#define KHZ_TO_MHZ(n)  KB_TO_MB(n)
#define KHZ_TO_HHZ(n)  KB_TO_HB(n)
#define MHZ_TO_KHZ(n)  MB_TO_KB(n)
#define HHZ_TO_KHZ(n)  HB_TO_KB(n)
#define KHZ_TO_GHZ(n)  (n / 1000000)

/* Overflow functions:
 * 	zeros: returns zero on overflow
 * 	magic: returns (max - v1 + v2) on overflow
 * 	mixed: like magic but returns zero if v1 isn't over the threshold (MAX >> bits)
 */

#define MAXBITS32      0x00000000FFFFFFFF
#define MAXBITS48      0x0000FFFFFFFFFFFF
#define MAXBITS64      0xFFFFFFFFFFFFFFFF

#define overflow_zeros(type, suffix) \
    type overflow_zeros_ ##suffix (type v2, type v1)
#define overflow_magic(type, suffix) \
    type overflow_magic_ ##suffix (type v2, type v1, type max)
#define overflow_mixed(type, suffix) \
    type overflow_mixed_ ##suffix (type v2, type v1, type max, uint bits)

overflow_zeros(double, f64);
overflow_zeros(ullong, u64);
overflow_mixed(ullong, u64);
overflow_magic(ullong, u64);
overflow_magic(uint, u32);

/* Round functions:
 *  ceil: given an integer number, rounds a value up to a specific digit.
 *        Ex: given the value 2345 and digit 3, it returns 2400.
 *  floor: pending.
 */

#define ceil_magic(type, suffix) \
    type ceil_magic_ ##suffix (type value, uint digits)

ceil_magic(ullong, u64);
ceil_magic(  uint, u32);

/** Given two doubles a and b, checks whether they are equal within a margin of th.*/
uint equal_with_th(double a, double b, double th);

uint equal_with_th_ul(ulong a,ulong b,double th);

/** Given two unsigned long's, one before and one after overflow, returns the
 *   value added to the first to obtain the second. This corrects end-begin when there 
 *   has been an overflow. */
unsigned long ulong_diff_overflow(unsigned long begin, unsigned long end);

/** Given two unsigned long's, one before and one after overflow, returns the
 *   value added to the first to obtain the second. This corrects end-begin when there 
 *   has been an overflow. */
unsigned long long ullong_diff_overflow(unsigned long long begin, unsigned long long end);

/** Given two long long's, one before and one after overflow, returns the
 *   value added to the first to obtain the second. 
 *   This corrects end-begin when there has been an overflow.
 */
long long llong_diff_overflow(long long begin, long long end);

/** Given a generic 2d array ('len' x 1) 'arr' and a function 'fn_ptr' (which
 * will convert the input type for its own computation), returns a new generic
 * array where each entry is the result of applying 'fn_ptr' to each entry of 'arr',
 * respectively.
 * This function will dynamically allocate memory space for the resulting array, so
 * it's recommended to call ear_math_free_gen_arr after using the resulting array.
 */
void** ear_math_apply(void **arr, size_t len, void* (*fn_ptr)(void*));

/** Given two vectors 'a' and 'b' of size 'n', returns its cosine similarity, which is
 *                  A·B
 *  cos(phi) =  ------------
 *              ||A||*||B||
 */
double ear_math_cosine_similarity(double *a, double *b, size_t n);

double ear_math_cosine_sim_uint(uint *a, uint *b, size_t n);

typedef struct mean_sd {
    double mean;
    double sd;
    double mag;
} mean_sd_t;

mean_sd_t ear_math_mean_sd(const double data[], size_t n);

/** Given a vector 'data' with 'n' values, returns the mean. */
double ear_math_mean(const double data[], size_t n);

/** Given a vector 'data' with 'n' values and its mean 'mean', returns the
 * standard deviation of the data. */
double ear_math_standard_deviation(const double data[], size_t n, double mean);

double ear_math_exp(double x);

double ear_math_scale(double src_range_min, double src_range_max, double n);

float ear_math_roundf(float n);

double ear_math_median(const double data[], size_t n);

#endif
