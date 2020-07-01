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

#ifndef EAR_COMMON_TIME_H
#define EAR_COMMON_TIME_H

#include <time.h>
#include <common/types/generic.h>

// Time units
#define TIME_SECS	1000000000
#define TIME_MSECS	1000000
#define TIME_USECS	1000
#define TIME_NSECS	1

typedef struct timespec timestamp;
typedef struct timespec timestamp_t;

/* Generic monotonic timestamp
 *	- Builded with the fast or precise versions under the hood. Without
 *	  considering any of its properties.
 *	- The monotonic timestamp is used to measure the elapsed time. Its value is
 *	  related with the system boot time, not to the system date.
 */
void timestamp_get(timestamp *ts);

/* Returns a monotonic timestamp
 * 	- precision: ns
 * 	- resolution: 1ns
 */
void timestamp_getprecise(timestamp *ts);

/* Returns a monotonic timestamp
 * 	- precision: ns
 * 	- resolution: 1-4ms
 * 	- Performance: 5x faster than 'time_gettimestamp_precise'
 */
void timestamp_getfast(timestamp *ts);

/* Realtime timestamp
 *	- Used to get the system time (or date) in timestamp format.
 */
void timestamp_getreal(timestamp *ts);

/* Converts timestamp_t format to ullong in the selected time units. */
ullong timestamp_convert(timestamp *ts, ullong time_unit);

/* Computes the difference between two timestamp_t values (ts2-ts1) and converts to the
 * selected time units. time_unit=1 means ns. */
ullong timestamp_diff(timestamp *ts2, timestamp *ts1, ullong time_unit);

/* A combination of getfast and convert. */
ullong timestamp_getconvert(ullong time_unit);

#endif //EAR_COMMON_TIME_H
