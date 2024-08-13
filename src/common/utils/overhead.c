/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <unistd.h>
#include <common/system/time.h>
#include <common/utils/overhead.h>
#include <common/utils/strtable.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>

#define N_QUEUE 256

typedef struct ohdata_s {
    const char *description;
    timestamp_t wall_start;
    timestamp_t wall_stop;
    double wall_accum; //us
    double wall_max; //us
    double wall_min; //us
    double wall_last;
    clock_t cpu_start;
    clock_t cpu_stop;
    double cpu_accum;
    double cpu_last;
    ullong call_count;
    uint active;
} ohdata_t;

static timestamp_t global_wall_start;
static clock_t     global_cpu_start;
static ohdata_t    queue[N_QUEUE];
static uint        queue_last;
static uint        init;
static strtable_t  st;

void overhead_getwall(timestamp *ts)
{
    timestamp_getprecise(ts);
}

void overhead_suscribe(const char *description, uint *id)
{
    #if !ENABLE_OVERHEAD
    return; 
    #endif
    if (!init) {
	// Formatting report table
	tprintf_init2(&st, verb_channel, STR_MODE_DEF, "10 14 14 14 14 14 14 14 14 14 32");
        // Times
	overhead_getwall(&global_wall_start);
        global_cpu_start = clock();
        init = 1;
    }

    *id = queue_last;
    queue[*id].description = description;
    queue[*id].call_count  = 0;
    queue[*id].active      = 1;
    queue_last++;
}

void overhead_start(uint id)
{
    #if !ENABLE_OVERHEAD
    return; 
    #endif
    queue[id].cpu_start = clock();
    overhead_getwall(&queue[id].wall_start);
}

void overhead_stop(uint id)
{
    double wall;
    double cpu;
    #if !ENABLE_OVERHEAD
    return; 
    #endif
    overhead_getwall(&queue[id].wall_stop);
    queue[id].cpu_stop = clock();
    // Differences
    wall = (double) timestamp_diff(&queue[id].wall_stop, &queue[id].wall_start, TIME_USECS);
    cpu  = (double) (queue[id].cpu_stop - queue[id].cpu_start);
    // Accums
    queue[id].call_count += 1;
    queue[id].wall_accum += wall;
    queue[id].wall_last   = wall;
    queue[id].cpu_accum  += cpu;
    queue[id].cpu_last    = cpu;
    // Min max
    if (queue[id].wall_min == 0)   queue[id].wall_min = wall;
    if (wall > queue[id].wall_max) queue[id].wall_max = wall;
    if (wall < queue[id].wall_min) queue[id].wall_min = wall;
}

void overhead_report(int print_header)
{
    #if !ENABLE_OVERHEAD
    return; 
    #endif
    double global_wall_secs;
    double global_cpu_secs;
    double wall_last_msecs;
    double wall_max_msecs;
    double wall_min_msecs;
    double wall_msecs;
    double wall_secs;
    double wall_call_msecs;
    double wall_percent;
    ATTR_UNUSED double cpu_last_msecs;
    double cpu_msecs;
    double cpu_secs;
    double cpu_call_msecs;
    double cpu_percent;
    int id;

    global_wall_secs = ((double) timestamp_diffnow(&global_wall_start, TIME_USECS)) / 1000000.0;
    global_cpu_secs  = ((double) (clock() - global_cpu_start)) / ((double) CLOCKS_PER_SEC);
        
    if (print_header) {
        overhead_print_header();
    }
    
    tprintf2(&st, "-||%0.3lf s||-||-||-||-||-|||%0.3lf s||-||-|||global",
	global_wall_secs, global_cpu_secs);

    for(id = 0; id < queue_last; ++id)
    {
        wall_msecs      = queue[id].wall_accum / 1000.0;
        wall_secs       = wall_msecs / 1000.0;
	wall_last_msecs = queue[id].wall_last / 1000.0;
	wall_max_msecs  = queue[id].wall_max / 1000.0;
	wall_min_msecs  = queue[id].wall_min / 1000.0;
        wall_call_msecs = 0.0;
        wall_percent    = (wall_secs / global_wall_secs)*100.0;
	cpu_secs        = queue[id].cpu_accum / ((double) CLOCKS_PER_SEC);
	cpu_msecs       = cpu_secs * 1000.0;
	cpu_last_msecs  = queue[id].cpu_last / ((double) CLOCKS_PER_SEC);
	cpu_call_msecs  = 0.0;
	cpu_percent     = (cpu_secs / global_cpu_secs)*100.0;

	if (queue[id].call_count) {
            wall_call_msecs = wall_msecs / ((double) queue[id].call_count);
            cpu_call_msecs  = cpu_msecs / ((double) queue[id].call_count);
	}
        //if (percent_clock > 100.0) {
        //    percent_clock = 0.0;
        //}

        tprintf2(&st, "%llu||%0.3lf s||%0.3lf ms||%0.3lf ms||%0.3lf ms||%0.3lf ms||%0.2lf %%|||%0.3lf s||%0.3lf ms||%0.2lf %%|||%s", queue[id].call_count,
                wall_secs, wall_call_msecs, wall_last_msecs, wall_max_msecs, wall_min_msecs, wall_percent, cpu_secs, cpu_call_msecs, cpu_percent, queue[id].description);
    }
}

void overhead_print_header()
{
    //cpu/#calls %%cpu
    tprintf2(&st, "#calls||wall accum||wall/#calls||wall last||wall max||wall min||%%wall time|||cpu accum||cpu/#calls||%%cpu time|||description");
    tprintf2(&st, "------||----------||-----------||---------||--------||--------||----------|||---------||----------||---------|||-----------");
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
