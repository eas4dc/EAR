/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* clang-format off */
// #define SHOW_DEBUGS 1
#include <stdlib.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/cache/cache.h>
#include <metrics/cache/archs/demo.h>
#include <metrics/cache/archs/dummy.h>
#include <metrics/cache/archs/perf.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static double          line_size;
static cache_ops_t     ops;
static apinfo_t        info;

void cache_load(topology_t *tp, int options)
{
    while (pthread_mutex_trylock(&lock));
    if (info.api != API_NONE) {
        goto out;
    }
    if (API_IS(options, API_DUMMY)) {
        goto dummy;
    }
    cache_perf_load(tp, &ops, options);
    cache_demo_load(tp, &ops, options);
dummy:
    cache_dummy_load(tp, &ops, options);
    cache_get_info(&info);
    // Getting cache line, is used to get the bandwidth
    line_size = (double) tp->cache_line_size;
out:
    pthread_mutex_unlock(&lock);
    return;
}

void cache_unload()
{
    while (pthread_mutex_trylock(&lock));
    if (info.api != API_NONE) {
        ops.unload();
        memset(&ops, 0, sizeof(cache_ops_t));
        memset(&info, 0, sizeof(apinfo_t));
    }
    pthread_mutex_unlock(&lock);
}

state_t cache_update(uint option, void *value)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
    s = ops.update(option, value);
    pthread_mutex_unlock(&lock);
    return s;
}

void cache_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    info->layer       = "CACHE";
    info->api         = API_NONE;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t cache_read(cache_t *ca)
{
    state_t s;
    memset(ca, 0, sizeof(cache_t)*info.devs_count);
    while (pthread_mutex_trylock(&lock));
    s = ops.read(ca);
    pthread_mutex_unlock(&lock);
    return s;
}

state_t cache_read_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
    state_t s;
    if (state_fail(s = cache_read(ca2))) {
        return s;
    }
    cache_data_diff(ca2, ca1, caD, gbs);
    return s;
}

state_t cache_read_copy(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
    state_t s;
    if (state_fail(s = cache_read_diff(ca2, ca1, caD, gbs))) {
        return s;
    }
    cache_data_copy(ca1, ca2);
    return s;
}

static void level_diff(cache_level_t *l2, cache_level_t *l1, cache_level_t *lD)
{
    lD->hits      = overflow_zeros_u64(l2->hits     , l1->hits     );
    lD->misses    = overflow_zeros_u64(l2->misses   , l1->misses   );
    lD->accesses  = overflow_zeros_u64(l2->accesses , l1->accesses );
    lD->lines_in  = overflow_zeros_u64(l2->lines_in , l1->lines_in );
    lD->lines_out = overflow_zeros_u64(l2->lines_out, l1->lines_out);
    lD->lines_in  = (lD->lines_in)? lD->lines_in: lD->misses;
    lD->hit_rate  = (lD->accesses && lD->accesses > lD->hits  )?
                    ((double) lD->hits  ) / ((double) lD->accesses): 0.0;
    lD->miss_rate = (lD->accesses && lD->accesses > lD->misses)?
                    ((double) lD->misses) / ((double) lD->accesses): 0.0;
}

static cache_level_t *get_offset(void *dst_addr, void *src_addr, void *src_lv_addr)
{
    return (src_lv_addr != NULL)? (cache_level_t *) (dst_addr + (src_lv_addr - src_addr)): NULL;
}

void cache_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
    double gbs_tot = 0.0;
    double secs = 0.0;
    int i;

    memset(caD, 0, sizeof(cache_t)*info.devs_count);
    if (ops.data_diff != NULL) {
        return ops.data_diff(ca2, ca1, caD, gbs);
    }
    for (i = 0; i < info.devs_count; ++i) {
        // L2 by default
        caD[i].ll  = (cache_level_t *) &caD[i].l2;
        caD[i].lbw = (cache_level_t *) &caD[i].l2;
        if (ca2[i].pid == 0 || ca1[i].pid == 0) {
            continue;
        }
        caD[i].pid = ca1[i].pid;
        caD[i].ll  = get_offset(&caD[i], &ca2[i], ca2[i].ll);
        caD[i].lbw = get_offset(&caD[i], &ca2[i], ca2[i].lbw);
        level_diff(&ca2[i].l1d, &ca1[i].l1d, &caD[i].l1d);
        level_diff(&ca2[i].l2 , &ca1[i].l2 , &caD[i].l2 );
        level_diff(&ca2[i].l3 , &ca1[i].l3 , &caD[i].l3 );
        secs = timestamp_fdiff(&ca2[i].time, &ca1[i].time, TIME_SECS, TIME_MSECS);
        secs = (secs > 0.0)? secs: 1.0;
        caD[i].bw_gbs = (double) (caD[i].lbw->lines_in + caD[i].lbw->lines_out);
        caD[i].bw_gbs = (caD[i].bw_gbs / secs) * line_size;
        caD[i].bw_gbs = (caD[i].bw_gbs / ((double) 1E9));
        gbs_tot += caD[i].bw_gbs;
    }
    for (i = 0; i < info.devs_count; ++i) {
        caD[i].bw_ratio = (gbs_tot > 0.0)? caD[i].bw_gbs / gbs_tot: 0.0;
    }
    if (gbs != NULL) {
        *gbs = gbs_tot;
    }
    #if SHOW_DEBUGS
    cache_data_print(caD, *gbs, fderr);
    #endif
}

void cache_data_alloc(cache_t **ca)
{
    if (ca != NULL) {
        *ca = calloc(info.devs_count, sizeof(cache_t));
    }
}

void cache_data_free(cache_t **ca)
{
    if (ca != NULL) {
        free(*ca);
        *ca = NULL;
    }
}

void cache_data_copy(cache_t *dst, cache_t *src)
{
    memcpy(dst, src, sizeof(cache_t) * info.devs_count);
    dst->ll  = get_offset(dst, src, src->ll);
    dst->lbw = get_offset(dst, src, src->lbw);
}

void cache_data_print(cache_t *ca, double gbs, int fd)
{
    char buffer[8192];
    cache_data_tostr(ca, gbs, buffer, sizeof(buffer));
    dprintf(fd, "%s", buffer);
}

char *cache_data_tostr(cache_t *ca, double gbs, char *buffer, size_t length)
{
    int i, b, w;

    buffer[0] = '\0';
    for (i = b = 0; i < info.devs_count && length > 0; ++i) {
        if (ca[i].pid == 0) {
            continue;
        }
        w = snprintf(&buffer[b], length-1,
        "d%d l1d: %10llu hits, %10llu misses, %10llu accesses, %0.2lf miss rate\n"
        "d%d l2 : %10llu hits, %10llu misses, %10llu accesses, %0.2lf miss rate\n"
        "d%d l3 : %10llu hits, %10llu misses, %10llu accesses, %0.2lf miss rate\n"
        "d%d lbw: %10.2lf GB/s, %10.2lf ratio\n",
        i, ca[i].l1d.hits , ca[i].l1d.misses , ca[i].l1d.accesses , ca[i].l1d.miss_rate,
        i, ca[i].l2.hits  , ca[i].l2.misses  , ca[i].l2.accesses  , ca[i].l2.miss_rate,
        i, ca[i].l3.hits  , ca[i].l3.misses  , ca[i].l3.accesses  , ca[i].l3.miss_rate,
        i, ca[i].bw_gbs   , ca[i].bw_ratio);
        w = (w < (length-1))? w: length;
        b += w, length -= w;
    }
    if (length  > 0 && b > 0) {
        w = snprintf(&buffer[b], length-1, "dtotal: %10.2lf GB/s\n", gbs);
        w = (w < (length-1))? w: length;
        b += w, length -= w;
    }
    if (length == 0 && b > 0) {
        buffer[b-1] = '\n';
    }
    return buffer;
}

void cache_internals_print(int fd)
{
    char buffer[4096];
    cache_internals_tostr(buffer, sizeof (buffer));
    dprintf(fd, "%s", buffer);
}

void cache_internals_tostr(char *buffer, int length)
{
    ops.internals_tostr(buffer, length);
}
