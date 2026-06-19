/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
// #define SHOW_DEBUGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <common/system/time.h>
#include <common/output/debug.h>
#include <metrics/flops/flops.h>
#include <metrics/flops/archs/perf.h>
#include <metrics/flops/archs/dummy.h>

// 0       1       2      3      4      5      6      7      8
// CPU-SP, CPU-DP, 128SP, 128DP, 256SP, 256DP, 512HP, 512SP, 512DP
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static flops_ops_t ops;
static apinfo_t info;

void flops_load(topology_t *tp, int options)
{
    while (pthread_mutex_trylock(&lock));
    if (info.api != API_NONE) {
        goto leave;
    }
    if (API_IS(options, API_DUMMY)) {
        goto dummy;
    }
    flops_perf_load(tp, &ops, options);
dummy:
    flops_dummy_load(tp, &ops, options);
    flops_get_info(&info);
leave:
    pthread_mutex_unlock(&lock);
}

void flops_unload()
{
    while (pthread_mutex_trylock(&lock));
    if (info.api != API_NONE) {
        ops.unload();
        memset(&ops, 0, sizeof(flops_ops_t));
        memset(&info, 0, sizeof(apinfo_t));
    }
    pthread_mutex_unlock(&lock);
}

state_t flops_update(uint option, void *value)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
    s = ops.update(option, value);
    pthread_mutex_unlock(&lock);
    return s;
}

void flops_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    info->layer = "FLOPS";
    info->api   = API_NONE;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t flops_read(flops_t *fl)
{
    memset(fl, 0, sizeof(flops_t)*info.devs_count);
    return ops.read(fl);
}

// Helpers
state_t flops_read_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gflops_tot)
{
    state_t s;
    if (state_ok(s = flops_read(fl2))) {
        flops_data_diff(fl2, fl1, flD, gflops_tot);
    }
    return s;
}

state_t flops_read_copy(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gflops_tot)
{
    state_t s;
    if (state_ok(s = flops_read_diff(fl2, fl1, flD, gflops_tot))) {
        flops_data_copy(fl1, fl2);
    }
    return s;
}

void flops_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gflops_tot)
{
    return ops.data_diff(fl2, fl1, flD, gflops_tot);
}

void flops_data_alloc(flops_t **fl)
{
    if (fl != NULL) {
        *fl = calloc(info.devs_count, sizeof(flops_t));
    }
}

void flops_data_free(flops_t **fl)
{
    if (fl != NULL) {
        free(*fl);
        *fl = NULL;
    }
}

void flops_data_copy(flops_t *dst, flops_t *src)
{
    memcpy(dst, src, sizeof(flops_t) * info.devs_count);
}

void flops_data_print(flops_t *flD, double gflops_tot, int fd)
{
    char buffer[SZ_BUFFER];
    flops_data_tostr(flD, gflops_tot, buffer, SZ_BUFFER);
    dprintf(fd, "%s", buffer);
}

char *flops_data_tostr(flops_t *flD, double gflops_tot, char *buffer, size_t length)
{
    int added = 0;
    int pivot = 0;
    int i;
    buffer[0] = '\0';
    for (i = 0; i < info.devs_count && length > 0; ++i) {
        if (flD[i].pid == 0) {
            continue;
        }
        added = snprintf(&buffer[pivot], length-1, "d%d "
                     "[%8llu %8llu] "
                     "[%8llu %8llu] "
                     "[%8llu %8llu] "
                     "[%8llu %8llu] "
                     "(%0.2lf GF/s in %0.2lf s)\n", i,
                     flD[i].f64 , flD[i].d64,
                     flD[i].f128, flD[i].d128,
                     flD[i].f256, flD[i].d256,
                     flD[i].f512, flD[i].d512,
                     flD[i].gflops, flD[i].secs);
        added = (added < (length-1))? added: length;
        pivot += added, length -= added;
    }
    return buffer;
}

void flops_data_accum(flops_t *flA, flops_t *flD, double *gflops_tot)
{
    double gflops = 0.0;
    int i;

    for (i = 0; i < info.devs_count; ++i) {
        if (flD != NULL) {
            // flAccum
            flA[i].f64 += flD[i].f64;
            flA[i].d64 += flD[i].d64;
            flA[i].f128 += flD[i].f128;
            flA[i].d128 += flD[i].d128;
            flA[i].f256 += flD[i].f256;
            flA[i].d256 += flD[i].d256;
            flA[i].f512 += flD[i].f512;
            flA[i].d512 += flD[i].d512;
            flA[i].secs += flD[i].secs;
            // flA[i].gflops += flD[i].gflops;
        }
        // Individual counters are already weigthed
        if (flA[i].secs > 0.0) {
            flA[i].gflops = (flA[i].f64 + flA[i].d64 + flA[i].f128 + flA[i].d128 + flA[i].f256 + flA[i].d256 +
                             flA[i].f512 + flA[i].d512) /
                            flA[i].secs;
            flA[i].gflops /= ((double) 1E9);
        }
        gflops += flA[i].gflops;
#if SHOW_DEBUGS
        dprintf(debug_channel, "DEV%d accum ", i);
        dprintf(debug_channel, "[%14lld %14lld] ", flA[i].f64, flA[i].d64);
        dprintf(debug_channel, "[%14lld %14lld] ", flA[i].f128, flA[i].d128);
        dprintf(debug_channel, "[%14lld %14lld] ", flA[i].f256, flA[i].d256);
        dprintf(debug_channel, "[%14lld %14lld] ", flA[i].f512, flA[i].d512);
        dprintf(debug_channel, "(%0.2lf GF/s in %0.2lf s)\n", flA[i].gflops, flA[i].secs);
#endif
    }
    dprintf(debug_channel, "DEVS accum %0.2lf GF/s\n", gflops);
    if (gflops_tot != NULL) {
        *gflops_tot = gflops;
    }
}

void flops_internals_print(int fd)
{
    char buffer[1024];
    flops_internals_tostr(buffer, 1024);
    dprintf(fd, "%s", buffer);
}

void flops_internals_tostr(char *buffer, int length)
{
    ops.internals_tostr(buffer, length);
}

/* If SCOPE_JOB be careful!!! it iterates out of range!!! */
ullong *flops_help_toold(flops_t *flD, ullong *flops)
{
    int i;

    for (i = 0; i < info.devs_count; i += 8) {
        flops[i + 0] = flD[i + 0].f64;
        flops[i + 4] = flD[i + 4].d64;
        flops[i + 1] = flD[i + 1].f128;
        flops[i + 5] = flD[i + 5].d128;
        flops[i + 2] = flD[i + 2].f256;
        flops[i + 6] = flD[i + 6].d256;
        flops[i + 3] = flD[i + 3].f512;
        flops[i + 7] = flD[i + 7].d512;
        debug("flops[%d]: %llu", i + 0, flops[i + 0]);
        debug("flops[%d]: %llu", i + 1, flops[i + 1]);
        debug("flops[%d]: %llu", i + 2, flops[i + 2]);
        debug("flops[%d]: %llu", i + 3, flops[i + 3]);
        debug("flops[%d]: %llu", i + 4, flops[i + 4]);
        debug("flops[%d]: %llu", i + 5, flops[i + 5]);
        debug("flops[%d]: %llu", i + 6, flops[i + 6]);
        debug("flops[%d]: %llu", i + 7, flops[i + 7]);
    }
    return flops;
}

#if TEST
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static topology_t tp;
static apinfo_t   info;
static flops_t   *t1;
static flops_t   *t2;
static flops_t   *tD;
static double     tA;
static pid_t      pid;
static int        forked;

static int count_fds()
{
    int dummy_fd = open("/dev/null", O_RDONLY);
    close(dummy_fd);
    return dummy_fd;
}

int main(int argc, char *argv[])
{
    int i, j, k = 0;

    topology_init(&tp);
reload:
    dprintf(STDOUT_FILENO, "%d: Loading... (%d fds)\n", getpid(), count_fds());
    flops_load(&tp, SCOPE_JOB | API_FREE);
    flops_get_info(&info);
    apinfo_tostr(&info);
    dprintf(STDOUT_FILENO, "%d: Loaded %s (%s:%s), with %u devices and %d fds\n",
        getpid(), info.api_str, info.scope_str, info.granularity_str, info.devs_count, count_fds());
    flops_data_alloc(&t1);
    flops_data_alloc(&t2);
    flops_data_alloc(&tD);
reread:
    flops_read(t1);
    sleep(2);
    for (i = j = 0; i < 1000000; ++i) {
        j += 1;
    }
    flops_data_print(t1, 0.0, STDOUT_FILENO);
    flops_read_diff(t2, t1, tD, &tA);
    flops_data_print(t2, 0.0, STDOUT_FILENO);
    flops_data_print(tD, 0.0, STDOUT_FILENO);
    flops_data_copy(t1, t2);
    dprintf(STDOUT_FILENO, "%d: Printing...\n", getpid());
    #if 1
    if (k++ == 2 || k == 10) {
        flops_update(UPD_PID_ADD, (void *) 1383557);
    } else if (k == 6 || k == 14){
        flops_update(UPD_PID_REMOVE, (void *) 1383557);
        //cache_update(UPD_PIDS_CLEAN, NULL);
    }
    #endif
    goto reread;
    return 0;
}
#endif
