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

/* A combination of getfast and convert. */
ullong timestamp_getconvert(ullong time_unit);

/* Computes the difference between two timestamp_t values (ts2-ts1) and converts to the
 * selected time units. time_unit=1 means ns. */
ullong timestamp_diff(timestamp *ts2, timestamp *ts1, ullong time_unit);

/* Gets the current time and diffs between now and ts1. */
ullong timestamp_diffnow(timestamp *ts1, ullong time_unit);

/* Converts a time to timestamp */
void timestamp_revert(timestamp *ts, ullong *tr, ullong time_unit);

void timestamp_to_str(timestamp *ts,char *txt,uint size);

void print_timestamp(timestamp *ts);

#endif //EAR_COMMON_TIME_H
