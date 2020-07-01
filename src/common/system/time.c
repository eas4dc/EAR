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

#include <stdio.h>
#include <unistd.h>
#include <common/system/time.h>

void timestamp_get(timestamp *ts)
{
	timestamp_getfast(ts);
}

void timestamp_getprecise(timestamp *ts)
{
	clock_gettime(CLOCK_MONOTONIC, ts);
}

void timestamp_getfast(timestamp *ts)
{
	clock_gettime(CLOCK_MONOTONIC_COARSE, ts);
}

void timestamp_getreal(timestamp *ts)
{
	clock_gettime(CLOCK_REALTIME_COARSE, ts);
}

ullong timestamp_convert(timestamp *ts, ullong time_unit)
{
	ullong stamp;
	stamp  = (ullong) (ts->tv_sec * 1000000000);
	stamp += (ullong) (ts->tv_nsec);
	stamp /= time_unit;
	return stamp;
}

ullong timestamp_diff(timestamp *ts2, timestamp *ts1, ullong time_unit)
{
	ullong stamp;

	if (ts2->tv_nsec < ts1->tv_nsec) {
		ts2->tv_sec   = ts2->tv_sec - 1;
		ts2->tv_nsec += 1000000000;
	}

	stamp  = (ullong) ((ts2->tv_sec - ts1->tv_sec) * 1000000000);
	stamp += (ullong) ((ts2->tv_nsec - ts1->tv_nsec));
	stamp /= time_unit;

	return stamp;
}

ullong timestamp_getconvert(ullong time_unit)
{
	timestamp_t ts;
	timestamp_getfast(&ts);
	return timestamp_convert(&ts, time_unit);
}
