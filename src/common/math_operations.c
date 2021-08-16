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

#include <limits.h>
#include <math.h>
#include <string.h>
#include <common/math_operations.h>

uint equal_with_th_ul(ulong a,ulong b,double th)
{
    int eq = 1;
    if (a == b) return eq;
    if (a > b) {
        // if (a < (b * (1 + th))) eq = 1;
        // else eq = 0;
        if (a * (1 - th) > b) eq = 0;
    } else {
        // if ((a * (1 + th)) > b) eq = 1;
        // else eq = 0;
        if (b * (1 - th) > a) eq = 0;
    }
    return eq;

}
unsigned int equal_with_th(double a, double b, double th)
{
    int eq = 1;
    if (a == b) return eq;
    if (a > b) {
        // if (a < (b * (1 + th))) eq = 1;
        // else eq = 0;
        if (a * (1 - th) > b) eq = 0;
    } else {
        // if ((a * (1 + th)) > b) eq = 1;
        // else eq = 0;
        if (b * (1 - th) > a) eq = 0;
    }
    return eq;
}

//Unsigned long can be 16, 32 or 64 bits
//a should be the old number, b the one after the overflow
unsigned long ulong_diff_overflow(unsigned long begin, unsigned long end)
{
    unsigned long max_16 = USHRT_MAX;
    unsigned long max_32 = ULONG_MAX;
    unsigned long max_48 = 281474976710656; //2^48
    unsigned long max_64 = ULLONG_MAX;

    unsigned long ret = 0;

    if (begin < max_16 && end < max_16)
    {
        ret = max_16 - begin + end;
    }
    else if (begin < max_32 && end < max_32 && max_32 < max_64)
    {
        ret = max_32 - begin + end;
    }
    if (begin < max_48 && end < max_48)
    {
        ret = max_48 - begin + end;
    }
    else
    {
        ret = max_64 - begin + end;
    }
    return ret;
}

//Unsigned long can be 32 or 64 bits
//a should be the old number, b the one after the overflow
unsigned long long ullong_diff_overflow(unsigned long long begin, unsigned long long end)
{
    unsigned long long max_16 = USHRT_MAX;
    unsigned long long max_32 = ULONG_MAX;
    unsigned long long max_64 = ULLONG_MAX;
    //max_48 was obtained empirically via monitoring the power results and checking
    //when the overflow occured. The same can be said about max_DRAM (~46 bits)
    unsigned long long max_DRAM = 0X3B9ACABB5CB6;
    unsigned long long max_48 = 0xEE6B23392C68; 

    unsigned long long ret = 0;

    if (begin < max_16 && end < max_16)
    {
        ret = max_16 - begin + end;
    }
    else if (begin < max_32 && end < max_32 && max_32 < max_64) //some architectures have the same size for ul and ull
    {
        ret = max_32 - begin + end;
    }
    else if (begin < max_DRAM && end < max_DRAM)
    {
        ret = max_DRAM - begin + end;
    }
    else if (begin < max_48 && end < max_48)
    {
        ret = max_48 - begin + end;
    }
    else
    {
        ret = max_64 - begin + end;
    }
    return ret;
}

long long llong_diff_overflow(long long begin, long long end)
{
    long long max_64=LLONG_MAX;
    long long ret;
    ret=max_64 - begin + end;
    return ret;
}

void** ear_math_apply(void **arr, size_t len, void* (*fn_ptr)(void*)){
    void** res = malloc(sizeof(void*)*len);
    for (size_t i = 0; i < len; i++) {
        res[i] = (*fn_ptr)(arr[i]);
    }
    return res;
}

void ear_math_free_gen_array(void **arr) {
    free(arr);
}

double ear_math_cosine_similarity(double *a, double *b, size_t n) {
    double dot_product = 0, magnitude_a = 0, magnitude_b = 0;
    for (int i = 0; i < n; i++) {
        dot_product += a[i] * b[i];
        magnitude_a += pow(a[i], 2);
        magnitude_b += pow(b[i], 2);
    }
    return dot_product / (sqrt(magnitude_a) * sqrt(magnitude_b));
}

double ear_math_cosine_sim_uint(uint *a, uint *b, size_t n){
    uint dot_product = 0, magnitude_a = 0, magnitude_b = 0;
    for (int i = 0; i < n; i++) {
        dot_product += a[i] * b[i];
        magnitude_a += pow(a[i], 2);
        magnitude_b += pow(b[i], 2);
    }
    return (double)dot_product / (double)(sqrt(magnitude_a) * sqrt(magnitude_b));
}

mean_sd_t ear_math_mean_sd(const double data[], size_t n){
    mean_sd_t result;
    result.mean = result.sd = 0.0;
    if (n == 0)
        return result;
    double sum, sq_sum;
    sum = sq_sum = 0;
    for (int i = 0; i < n; i++){
        sum += data[i];
        sq_sum += pow(data[i], 2);
    }
    result.mean = sum / n;
    result.sd = sqrt(sq_sum / n - pow(result.mean, 2));
    result.mag = sqrt(sq_sum);
    return result;
}

double ear_math_mean(const double data[], size_t n) {
    double m = 0;
    for (size_t i = 0; i < n; i++) {
        m += data[i];
    }
    return m / n;
}

double ear_math_standard_deviation(const double data[], size_t n, double mean){
    double sum = 0;
    for (size_t i = 0; i < n; i++) {
        sum += (pow(data[i] - mean, 2));
    }
    return sqrt(sum / (n - 1));
}

double ear_math_exp(double x){
    return exp(x);
}

double ear_math_scale(double src_range_min, double src_range_max, double n){
    return ((n - src_range_min)/(src_range_max - src_range_min)); // *(target_range_max - target_range_min) + target_range_min;
}

float ear_math_roundf(float n){
    return roundf(n);
}

int cmp_fnc(const void *p1, const void *p2){
    return (int)(*(double*)p1 - *(double*)p2);
}

double ear_math_median(const double data[], size_t n){
    double *copied = calloc(n, sizeof(double));
    memcpy(copied, data, n * sizeof(double));

    qsort(copied, n, sizeof(double), cmp_fnc);

    double median;
    if (n % 2 != 0){
        median = copied[(n-1)/2];
    }else{
        median = (copied[(n-2)/2] + copied[n/2]) / 2;
    }

    free(copied);

    return median;
}
