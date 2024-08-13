/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_COMMON_TIME_H
#define EAR_COMMON_TIME_H

#include <time.h>
#include <sys/time.h>
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

/* Computes the difference between two timestamp_t values (ts2-ts1) and converts
 * to the selected time units. Use the TIME_* macros to select the unit. */
ullong timestamp_diff(timestamp *ts2, timestamp *ts1, ullong time_unit);

/* Float diff. Precision unit is used to compute the decimals. */
double timestamp_fdiff(timestamp *ts2, timestamp *ts1, ullong time_unit, ullong prec_unit);

/* Gets the current time and diffs between now and ts1. */
ullong timestamp_diffnow(timestamp *ts1, ullong time_unit);

/* Converts a time to timestamp */
void timestamp_revert(timestamp *ts, ullong time, ullong time_unit);

void timestamp_print(timestamp *ts, int fd);

void timestamp_tostr(timestamp *ts, char *buffer, size_t size);

// Helpers
/* Converts a timeval to a 64 bits time value in `time_unit` units. */
ullong timeval_convert(struct timeval *ts, ullong time_unit);

/* Converts a 64 bits time value in `time_unit` units in a timeval. */
struct timeval timeval_create(ullong time, ullong time_unit);

#endif //EAR_COMMON_TIME_H
