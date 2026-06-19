/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/math_operations.h>
#include <common/output/debug.h>
#include <common/utils/overhead.h>
#include <metrics/bandwidth/archs/amd17.h>
#include <metrics/bandwidth/archs/amd17df.h>
#include <metrics/bandwidth/archs/amd19.h>
#include <metrics/bandwidth/archs/dummy.h>
#include <metrics/bandwidth/archs/eard.h>
#include <metrics/bandwidth/archs/intel106.h>
#include <metrics/bandwidth/archs/intel143.h>
#include <metrics/bandwidth/archs/intel63.h>
#include <metrics/bandwidth/archs/likwid.h>
#include <metrics/bandwidth/archs/perf.h>
#include <metrics/common/apis.h>
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static double line_size;
static bwidth_ops_t ops;
static apinfo_t info;
static uint oid; // Overhead calculation

void bwidth_load(topology_t *tp, int options)
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (info.api != API_NONE) {
        goto leave;
    }
    if (API_IS(options, API_DUMMY)) {
        goto dummy;
    }
    bwidth_amd19_load(tp, &ops, options);
    bwidth_amd17_load(tp, &ops, options);
    bwidth_amd17df_load(tp, &ops, options);
    bwidth_intel63_load(tp, &ops, options);
    bwidth_intel106_load(tp, &ops, options);
    bwidth_intel143_load(tp, &ops, options);
    bwidth_perf_load(tp, &ops, options);
    bwidth_likwid_load(tp, &ops, options);
    bwidth_eard_load(tp, &ops, options);
dummy:
    bwidth_dummy_load(tp, &ops, options);
    // Saving some additional data
    bwidth_get_info(&info);
    line_size = (double) tp->cache_line_size;
    overhead_subscribe("metrics/bandwidth", &oid);
leave:
    pthread_mutex_unlock(&lock);
}

void bwidth_unload()
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (info.api != API_NONE) {
        ops.unload();
        memset(&ops, 0, sizeof(bwidth_ops_t));
        memset(&info, 0, sizeof(apinfo_t));
    }
    pthread_mutex_unlock(&lock);
}

void bwidth_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    info->layer = "BANDWIDTH";
    info->api   = API_NONE;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t bwidth_read(bwidth_t *bws)
{
    overhead_start(oid);
    bwidth_data_null(bws);
    state_t s = ops.read(bws);
    overhead_stop(oid);
    return s;
}

state_t bwidth_read_diff(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs)
{
    state_t s;
    if (state_fail(s = bwidth_read(b2))) {
        // if read fails, b1 is copies in b2, which produces a 0.
        bwidth_data_copy(b2, b1);
    }
    bwidth_data_diff(b2, b1, bD, cas, gbs);
    return s;
}

state_t bwidth_read_copy(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs)
{
    state_t s;
    if (state_fail(s = bwidth_read_diff(b2, b1, bD, cas, gbs))) {
        return s;
    }
    bwidth_data_copy(b1, b2);
    return s;
}

void bwidth_data_diff(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs)
{
    ullong tcas = 0LLU; // Total CAS
    ullong diff = 0LLU;
    ullong time = 0LLU;
    double tgbs = 0.0; // Total GB/s
    double secs = 0.0;
    int i;

    if (cas != NULL) {
        *cas = 0LLU;
    }
    if (gbs != NULL) {
        *gbs = 0.0;
    }
    // Ms to seconds (for decimal
    time = timestamp_diff(&b2[info.devs_count - 1].time, &b1[info.devs_count - 1].time, TIME_MSECS);
    debug("devs_count %d", info.devs_count);
    debug("timestamp B2 %ld_%ld", b2[info.devs_count - 1].time.tv_sec, b2[info.devs_count - 1].time.tv_nsec);
    debug("timestamp B1 %ld_%ld", b1[info.devs_count - 1].time.tv_sec, b1[info.devs_count - 1].time.tv_nsec);
    // If no time, no metrics
    if (time == 0LLU) {
        debug("Time is 0");
        return;
    }
    secs = (double) time;
    secs = secs / 1000.0;
    debug("secs %lf", secs);
    if (bD != NULL) {
        bD[info.devs_count - 1].secs = secs;
    }
    // Computing differences. 64 bit APIs:
    // - PERF: it converts all registers widths into 64 bit registers.
    // - BYPASS: because calls cache which is a PERF API.
    // - LIKWID: doubt.
    // Meanwhile this API is not fully updated to get the registers width, we
    // will convert to 0 the result if the values of b1 are greater than b2.
    for (i = 0; i < info.devs_count - 1; ++i) {
        diff = overflow_zeros_u64(b2[i].cas, b1[i].cas);
        tcas += diff;
        if (bD != NULL) {
            bD[i].cas = diff;
        }
        debug("DEV%02d/%u: %014llu - %014llu = %llu", i, info.devs_count - 2, b2[i].cas, b1[i].cas, diff);
    }
    tgbs = bwidth_help_castogbs(tcas, secs);
    debug("CAS: %llu in %0.4lf secs", tcas, secs);
    debug("GBS: %0.2lf", tgbs);
    if (gbs != NULL) {
        *gbs = tgbs;
    }
    if (cas != NULL) {
        *cas = tcas;
    }
}

void bwidth_data_accum(bwidth_t *bA, bwidth_t *bD, ullong *cas, double *gbs)
{
    ullong tcas = 0LLU; // Total CAS
    int i;

    for (i = 0; i < info.devs_count - 1; ++i) {
        if (bD != NULL) {
            bA[i].cas += bD[i].cas;
        }
        tcas += bA[i].cas;
    }
    if (bD != NULL) {
        bA[info.devs_count - 1].secs += bD[info.devs_count - 1].secs;
    }
    if (gbs != NULL) {
        *gbs = bwidth_help_castogbs(tcas, bA[info.devs_count - 1].secs);
    }
    if (cas != NULL) {
        *cas = tcas;
    }
    debug("CAS: %llu in %0.2lf secs", tcas, bA[info.devs_count - 1].secs);
}

void bwidth_data_alloc(bwidth_t **b)
{
    *b = calloc(info.devs_count, sizeof(bwidth_t));
}

void bwidth_data_free(bwidth_t **b)
{
    free(*b);
    *b = NULL;
}

void bwidth_data_null(bwidth_t *bws)
{
    memset(bws, 0, info.devs_count * sizeof(bwidth_t));
}

void bwidth_data_copy(bwidth_t *dst, bwidth_t *src)
{
    memcpy(dst, src, info.devs_count * sizeof(bwidth_t));
}

void bwidth_data_print(ullong cas, double gbs, int fd)
{
    char buffer[4096];
    bwidth_data_tostr(cas, gbs, buffer, sizeof(buffer));
    dprintf(fd, "%s", buffer);
}

char *bwidth_data_tostr(ullong cas, double gbs, char *buffer, size_t length)
{
#if 1
    snprintf(buffer, length, "%0.2lf GB/s (%llu CAS)\n", gbs, cas);
#else
    size_t accum = 0;
    size_t added = 0;
    uint i;
    for (i = 0; i < info.devs_count && length > 0; ++i) {
        added  = snprintf(&buffer[accum], length, "IMC%u: %llu\n", i, b[i].cas);
        length = length - added;
        accum  = accum + added;
    }
    snprintf(&buffer[accum], length, "Total: %lf GB/s\n", gbs);
#endif
    return buffer;
}

double bwidth_help_castogbs(ullong cas, double secs)
{
    double gbs = 0.0;
    // CAS to CAS/s
    gbs = (((double) cas) / secs);
    // CAS/s to BYTES/s
    if (ops.castob != NULL)
        gbs = ops.castob(gbs);
    else
        gbs = gbs * line_size;
    // BYTES/s to GB/s
    gbs = (gbs / ((double) 1E9));
    return gbs;
}

double bwidth_help_castotpi(ullong cas, ullong instructions)
{
    if (ops.castob != NULL) {
        return ops.castob((double) cas) / ((double) instructions);
    }
    return (((double) cas) * line_size) / ((double) instructions);
}