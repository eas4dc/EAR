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

#include <unistd.h>
#include <common/system/time.h>
#include <common/utils/overhead.h>
#include <common/string_enhanced.h>

#define N_QUEUE 256

typedef struct ohdata_s {
    const char *description;
    timestamp_t time_start;
    timestamp_t time_stop;
    ullong call_count;
    ullong accum_us;
    uint active;
} ohdata_t;

static uint        init;
static clock_t     global_clock_start;
static timestamp_t global_stamp_start;
static ohdata_t    queue[N_QUEUE];
static uint        queue_last;

void overhead_suscribe(const char *description, uint *id)
{
    #if !ENABLE_OVERHEAD
    return; 
    #endif
    if (!init) {
        timestamp_get(&global_stamp_start);
        global_clock_start = clock();
        init = 1;
    }

    *id = queue_last;
    queue[*id].description = description;
    queue[*id].active = 1;
    queue_last++;
}

void overhead_start(uint id)
{
    #if !ENABLE_OVERHEAD
    return; 
    #endif
    timestamp_get(&queue[id].time_start);
    queue[id].call_count += 1;
}

void overhead_stop(uint id)
{
    #if !ENABLE_OVERHEAD
    return; 
    #endif
    timestamp_get(&queue[id].time_stop);
    queue[id].accum_us += timestamp_diff(&queue[id].time_stop, &queue[id].time_start, TIME_USECS);
}

void overhead_report()
{
    #if !ENABLE_OVERHEAD
    return; 
    #endif
    double global_stamp_secs;
    double global_clock_secs;
    double percent_clock;
    double percent_stamp;
    double percall_msecs;
    double accum_msecs;
    double accum_secs;
    int id;

    global_stamp_secs = ((double) timestamp_diffnow(&global_stamp_start, TIME_USECS)) / 1000000.0;
    global_clock_secs = ((double) (clock() - global_clock_start)) / CLOCKS_PER_SEC;

    tprintf_init(fderr, STR_MODE_DEF, "10 14 14 14 14 32");
    tprintf("#calls||time accum||time/#calls||%%cpu time||%%wall time||description");
    tprintf("------||----------||-----------||---------||----------||-----------");

    for(id = 0; id < queue_last; ++id)
    {
        accum_msecs   = ((double) queue[id].accum_us) / 1000.0;
        percall_msecs = accum_msecs / ((double) queue[id].call_count);
        accum_secs    = accum_msecs / 1000.0;
        percent_clock = (accum_secs / global_clock_secs)*100.0;
        percent_stamp = (accum_secs / global_stamp_secs)*100.0;

        if (percent_clock > 100.0) {
            percent_clock = 0.0;
        }

        tprintf("%llu||%0.3lf s||%0.3lf ms||%0.2lf %%||%0.2lf %%||%s", queue[id].call_count,
                accum_secs, percall_msecs, percent_clock, percent_stamp, queue[id].description);
    }
}

#if 0
int main(int argc, char *argv[])
{
    uint id1;
    uint id2;
    uint id3;
    int i;

    overhead_suscribe("CPUFreq read", &id1);
    overhead_suscribe("PIPE communication", &id2);
    overhead_suscribe("GPU read", &id3);

    for (i = 0; i < 2; ++i) {
        overhead_start(id1);
        sleep(1);
        overhead_stop(id1);
        overhead_start(id2);
        sleep(2);
        overhead_stop(id2);
        overhead_start(id3);
        sleep(1);
        overhead_stop(id3);
    }

    overhead_report();

    return 0;
}
#endif
