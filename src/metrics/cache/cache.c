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

#include <common/output/debug.h>
#include <metrics/cache/archs/dummy.h>
#include <metrics/cache/archs/perf.h>
#include <metrics/cache/cache.h>
#include <pthread.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static double line_size;
static cache_ops_t ops;
static uint load;

void cache_load(topology_t *tp, int force_api)
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (load) {
        goto out;
    }
    if (API_IS(force_api, API_DUMMY)) {
        goto dummy;
    }
    cache_perf_load(tp, &ops);
dummy:
    cache_dummy_load(tp, &ops);
    // Getting cache line, is used to get the bandwidth
    line_size = (double) tp->cache_line_size;
    load      = 1;
out:
    pthread_mutex_unlock(&lock);
    return;
}

void cache_get_info(apinfo_t *info)
{
    info->layer       = "CACHE";
    info->api         = API_NONE;
    info->devs_count  = 0;
    info->scope       = SCOPE_PROCESS;
    info->granularity = GRANULARITY_PROCESS;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t cache_init(ctx_t *c)
{
    while (pthread_mutex_trylock(&lock))
        ;
    state_t s = ops.init();
    pthread_mutex_unlock(&lock);
    return s;
}

state_t cache_dispose(ctx_t *c)
{
    load = 0;
    return ops.dispose();
}

state_t cache_read(ctx_t *c, cache_t *ca)
{
    return ops.read(ca);
}

state_t cache_read_diff(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
    state_t s;
    if (state_fail(s = cache_read(c, ca2))) {
        return s;
    }
    cache_data_diff(ca2, ca1, caD, gbs);
    return s;
}

state_t cache_read_copy(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
    state_t s;
    if (state_fail(s = cache_read_diff(c, ca2, ca1, caD, gbs))) {
        return s;
    }
    cache_data_copy(ca1, ca2);
    return s;
}

static void level_diff(cache_level_t *l2, cache_level_t *l1, cache_level_t *lD)
{
    lD->hits      = l2->hits - l1->hits;
    lD->misses    = l2->misses - l1->misses;
    lD->accesses  = l2->accesses - l1->accesses;
    lD->lines_in  = l2->lines_in - l1->lines_in;
    lD->lines_out = l2->lines_out - l1->lines_out;
    lD->lines_in  = (lD->lines_in) ? lD->lines_in : lD->misses;
    lD->hit_rate  = (lD->accesses) ? ((double) lD->hits) / ((double) lD->accesses) : 0.0;
    lD->miss_rate = (lD->accesses) ? ((double) lD->misses) / ((double) lD->accesses) : 0.0;
}

static cache_level_t *get_offset(void *dst_addr, void *src_addr, void *src_lv_addr)
{
    return (cache_level_t *) (dst_addr + (src_lv_addr - src_addr));
}

void cache_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
    double secs = 0.0;

    if (ops.data_diff != NULL) {
        return ops.data_diff(ca2, ca1, caD, gbs);
    }
    memset(caD, 0, sizeof(cache_t));
    secs     = timestamp_fdiff(&ca2->time, &ca1->time, TIME_SECS, TIME_MSECS);
    caD->ll  = get_offset(caD, ca2, ca2->ll);
    caD->lbw = get_offset(caD, ca2, ca2->lbw);
    level_diff(&ca2->l1d, &ca1->l1d, &caD->l1d);
    level_diff(&ca2->l2, &ca1->l2, &caD->l2);
    level_diff(&ca2->l3, &ca1->l3, &caD->l3);
    if (gbs != NULL) {
        *gbs = (double) (caD->lbw->lines_in + caD->lbw->lines_out);
        *gbs = *gbs / secs;
        *gbs = *gbs * ((double) line_size);
        *gbs = *gbs / ((double) 1E9);
    }
#if SHOW_DEBUGS
    cache_data_print(caD, *gbs, fderr);
#endif
}

void cache_data_copy(cache_t *dst, cache_t *src)
{
    memcpy(dst, src, sizeof(cache_t));
    dst->ll  = get_offset(dst, src, src->ll);
    dst->lbw = get_offset(dst, src, src->lbw);
}

void cache_data_print(cache_t *ca, double gbs, int fd)
{
    char buffer[SZ_BUFFER];
    cache_data_tostr(ca, gbs, buffer, SZ_BUFFER);
    dprintf(fd, "%s", buffer);
}

void cache_data_tostr(cache_t *ca, double gbs, char *buffer, size_t length)
{
    snprintf(buffer, length,
    "L1D: %10llu hits, %10llu misses, %10llu accesses, %10llu lines_in, %10llu lines_out, %0.2lf hit rate, %0.2lf miss rate\n"
    "L2 : %10llu hits, %10llu misses, %10llu accesses, %10llu lines_in, %10llu lines_out, %0.2lf hit rate, %0.2lf miss rate\n"
    "L3 : %10llu hits, %10llu misses, %10llu accesses, %10llu lines_in, %10llu lines_out, %0.2lf hit rate, %0.2lf miss rate\n"
    "LL : %10llu hits, %10llu misses, %10llu accesses, %10llu lines_in, %10llu lines_out, %0.2lf hit rate, %0.2lf miss rate\n"
    "LBW: %10llu hits, %10llu misses, %10llu accesses, %10llu lines_in, %10llu lines_out, %0.2lf hit rate, %0.2lf miss rate\n"
    "CAS: %0.2lf GB/s\n",
    ca->l1d.hits , ca->l1d.misses , ca->l1d.accesses , ca->l1d.lines_in , ca->l1d.lines_out , ca->l1d.hit_rate , ca->l1d.miss_rate ,
    ca->l2.hits  , ca->l2.misses  , ca->l2.accesses  , ca->l2.lines_in  , ca->l2.lines_out  , ca->l2.hit_rate  , ca->l2.miss_rate  ,
    ca->l3.hits  , ca->l3.misses  , ca->l3.accesses  , ca->l3.lines_in  , ca->l3.lines_out  , ca->l3.hit_rate  , ca->l3.miss_rate  ,
    ca->ll->hits , ca->ll->misses , ca->ll->accesses , ca->ll->lines_in , ca->ll->lines_out , ca->ll->hit_rate , ca->ll->miss_rate ,
    ca->lbw->hits, ca->lbw->misses, ca->lbw->accesses, ca->lbw->lines_in, ca->lbw->lines_out, ca->lbw->hit_rate, ca->lbw->miss_rate,
    gbs);
}

void cache_internals_print(int fd)
{
    char buffer[1024];
    cache_internals_tostr(buffer, 1024);
    dprintf(fd, "%s", buffer);
}

void cache_internals_tostr(char *buffer, int length)
{
    ops.internals_tostr(buffer, length);
}

#if TEST
#include <common/utils/stress.h>

static topology_t tp;
static apinfo_t info;
static cache_t c1;
static cache_t c2;
static cache_t cD;
static double gbs;

int main(int argc, char *argv[])
{
    topology_init(&tp);

    cache_load(&tp, API_FREE);
    stress_alloc();

    cache_get_info(&info);
    apinfo_tostr(&info);
    printf("%s\n", info.api_str);

    cache_internals_print(fderr);

    cache_init(no_ctx);
    cache_read(no_ctx, &c1);
    // sleep(1);
    stress_bandwidth(1000);
    cache_read_diff(no_ctx, &c2, &c1, &cD, &gbs);

    return 0;
}
#endif