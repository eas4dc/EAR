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
#include <string.h>
#include <stdio.h>
#include <common/math_operations.h>

unsigned long long uncore_ullong_diff_overflow(unsigned long long begin, unsigned long long end)
{
    unsigned long long max_64 = ULLONG_MAX;
    unsigned long long max_48 = 281474976710656; //2^48
    unsigned long long ret = 0;

    if (begin < max_48 && end < max_48) {
        ret = max_48 - begin + end;
    } else {
        ret = max_64 - begin + end;
    }
    return ret;
}

/** Calculates diff=end-begin, with vectors of N elements */
void diff_uncores(unsigned long long * diff,unsigned long long *end,unsigned long long  *begin,int N)
{
	int i;
	for (i=0;i<N;i++){
		if (end[i]<begin[i]){
			diff[i]=uncore_ullong_diff_overflow(begin[i],end[i]);
		}else{
			diff[i]=end[i]-begin[i];
		}
	}
}

/** Copies DEST=SRC */
void copy_uncores(unsigned long long * DEST,unsigned long long * SRC,int N)
{
	memcpy((void *)DEST, (void *)SRC, N*sizeof(unsigned long long));
}

int uncore_are_frozen(unsigned long long * DEST,int N)
{
	int i,frozen=1;
	for (i=0;i<N;i++){
		if (DEST[i]>0){ 
			frozen=0;	
			break;
		}
	}
	return frozen;
}

void print_uncores(unsigned long long * DEST,int N)
{
  int i,frozen=1;
  for (i=0;i<N;i++){
		fprintf(stdout,"Counter %d= %llu \t",i,DEST[i]);
	}
	fprintf(stdout,"\n");
}

