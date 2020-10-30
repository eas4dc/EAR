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

/** Given two doubles a and b, checks if they are equal within a margin of th.*/
unsigned int equal_with_th(double a, double b, double th);

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
*   This corrects end-begin when there has been an overflow.*/
long long llong_diff_overflow(long long begin, long long end);
