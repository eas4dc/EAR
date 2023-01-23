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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <stdio.h>
#include <unistd.h>
#include <common/system/time.h>
#include <common/output/debug.h>

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
	stamp  = (ullong) (ts->tv_sec * 1000000000);
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
	ullong initsec = 0, initnsec = 0;
	ullong endsec = 0 , endnsec = 0;
	ullong stamp = 0;
  if ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec == ts2->tv_nsec)) return (ullong)0;

	initsec  = (ullong)ts1->tv_sec;
	initnsec = (ullong)ts1->tv_nsec;
	endsec   = (ullong)ts2->tv_sec;
	endnsec  = (ullong)ts2->tv_nsec;
	
	if (endnsec < initnsec){
		endsec--;
		endnsec += 1E9;
	}
        #if 0
	endnsec  /= time_unit;
	initnsec /= time_unit;

	stamp = ((endsec - initsec) * (1E9 / time_unit));
	if (endnsec < initnsec) {
        return stamp;
        #endif
        stamp = ((endsec - initsec)) * 1E9 + (endnsec - initnsec);
        return stamp / time_unit;
        #if 0
    }
    return stamp + (endnsec - initnsec);
    #endif

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
	snprintf(buffer, size, "TS->SECS %ld TS->NSECS %ld", ts->tv_sec, ts->tv_nsec);
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
