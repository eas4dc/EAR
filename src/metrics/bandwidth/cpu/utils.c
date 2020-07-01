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

