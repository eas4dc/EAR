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

#ifndef _MB_NOT_PRIV_H_
#define _MB_NOT_PRIV_H_

unsigned long long uncore_ullong_diff_overflow(unsigned long long begin, unsigned long long end);

/** Calculates diff=end-begin, with vectors of N elements */
void diff_uncores(unsigned long long * diff,unsigned long long *end,unsigned long long  *begin,int N); 

/** Copies DEST=SRC */
void copy_uncores(unsigned long long * DEST,unsigned long long * SRC,int N);

int uncore_are_frozen(unsigned long long * DEST,int N);
void print_uncores(unsigned long long * DEST,int N);

#endif
