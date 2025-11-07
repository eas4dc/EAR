/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <common/math_operations.h>
#include <common/system/time.h>
#include <common/output/debug.h>
#include <common/math_operations.h>

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
    ullong stamp = 0;
    stamp        = (ullong) (ts->tv_sec * 1000000000);
    stamp += (ullong) (ts->tv_nsec);
    stamp /= time_unit;
    return stamp;
}

ullong timestamp_getconvert(ullong time_unit)
{
    timestamp_t ts;
    timestamp_getfast(&ts);
    return timestamp_convert(&ts, time_unit);
}

ullong timestamp_diff(timestamp *ts2, timestamp *ts1, ullong time_unit)
{
    #define __1E9LLU 1000000000LLU
    ullong ts2_nsec = ts2->tv_nsec;
    ullong ts1_nsec = ts1->tv_nsec;
    ullong ts2_sec  = ts2->tv_sec;
    ullong ts1_sec  = ts1->tv_sec;
    if (ts1_sec > ts2_sec || ts2_nsec >= __1E9LLU || ts1_nsec >= __1E9LLU) {
        return 0LLU;
    }
    if (ts1_sec == ts2_sec && ts1_nsec >= ts2_nsec) {
        return 0LLU;
    }
    ts2_sec  = ts2_sec - ts1_sec - (ts1_nsec > ts2_nsec);
    ts2_nsec = overflow_magic_u64(ts2_nsec, ts1_nsec, __1E9LLU);
    return ((ts2_sec * __1E9LLU) + ts2_nsec) / time_unit;
}

double timestamp_fdiff(timestamp *ts2, timestamp *ts1, ullong diff_unit, ullong prec_unit)
{
    ullong idiff = timestamp_diff(ts2, ts1, prec_unit);
    double fdiff = ((double) idiff) / (((double) diff_unit) / ((double) prec_unit));
    return fdiff;
}

ullong timestamp_diffnow(timestamp *ts1, ullong time_unit)
{
    timestamp_t time_now;
    timestamp_get(&time_now);
    return timestamp_diff(&time_now, ts1, time_unit);
}

void timestamp_revert(timestamp *ts, ullong time, ullong time_unit)
{
    ullong aux_ns = time * time_unit;
    ts->tv_sec    = aux_ns / 1000000000;
    ts->tv_nsec   = aux_ns - (ts->tv_sec * 1000000000);
}

void timestamp_print(timestamp *ts, int fd)
{
    char buffer[1024];
    timestamp_tostr(ts, buffer, sizeof(buffer));
    dprintf(fd, "%s", buffer);
}

void timestamp_tostr(timestamp *ts, char *buffer, size_t size)
{
    snprintf(buffer, size, "Timestamp: %ld s, %ld ns\n", ts->tv_sec, ts->tv_nsec);
}

ullong timeval_convert(struct timeval *ts, ullong time_unit)
{
    ullong nanos = (((ullong) ts->tv_sec) * 1E9) + (((ullong) ts->tv_usec) * 1E3);
    return (nanos / time_unit);
}

struct timeval timeval_create(ullong time, ullong time_unit)
{
    struct timeval ts;
    ullong nanos = time * time_unit;
    ullong nosec = nanos - ((nanos / TIME_SECS) * TIME_SECS);
    ts.tv_sec    = nanos / TIME_SECS;
    ts.tv_usec   = nosec / TIME_USECS;
    return ts;
}
