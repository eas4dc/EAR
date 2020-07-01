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

unsigned int equal_with_th(double a, double b, double th)
{
	int eq;
	if (a > b) {
		if (a < (b * (1 + th))) eq = 1;
		else eq = 0;
	} else {
		if ((a * (1 + th)) > b) eq = 1;
		else eq = 0;
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

