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
#include <metrics/cpi/archs/dummy.h>
#include <metrics/cpi/archs/perf.h>
#include <metrics/cpi/cpi.h>
#include <pthread.h>
#include <string.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static cpi_ops_t ops;
static apinfo_t info;

void cpi_load(topology_t *tp, int force_api)
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (info.api != API_NONE) {
        goto leave;
    }
    if (API_IS(force_api, API_DUMMY)) {
        goto dummy;
    }
    cpi_perf_load(tp, &ops);
dummy:
    cpi_dummy_load(tp, &ops);
    // Bandiwdth wants to know more about the loaded API. This is safe because at
    // this point all API's have their devices counter.
    cpi_get_info(&info);
leave:
    pthread_mutex_unlock(&lock);
}

void cpi_get_api(uint *api_in)
{
    apinfo_t info;
    cpi_get_info(&info);
    *api_in = info.api;
}

void cpi_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    info->layer       = "CPI";
    info->api         = API_NONE;
    info->devs_count  = 1;
    info->scope       = SCOPE_PROCESS;
    info->granularity = GRANULARITY_PROCESS;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t cpi_init(ctx_t *c)
{
    while (pthread_mutex_trylock(&lock))
        ;
    state_t s = ops.init();
    pthread_mutex_unlock(&lock);
    return s;
}

state_t cpi_dispose(ctx_t *c)
{
    return ops.dispose();
}

state_t cpi_read(ctx_t *c, cpi_t *ci)
{
    return ops.read(ci);
}

// Helpers
state_t cpi_read_diff(ctx_t *c, cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi)
{
    state_t s;
    if (state_fail(s = ops.read(ci2))) {
        return s;
    }
    cpi_data_diff(ci2, ci1, ciD, cpi);
    return s;
}

state_t cpi_read_copy(ctx_t *c, cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi)
{
    state_t s;
    if (state_fail(s = cpi_read_diff(c, ci2, ci1, ciD, cpi))) {
        return s;
    }
    cpi_data_copy(ci1, ci2);
    return s;
}

void cpi_data_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi)
{
    static double cpi_copy = 0.0;

    memset(ciD, 0, sizeof(cpi_t));
    ciD->instructions        = overflow_zeros_u64(ci2->instructions, ci1->instructions);
    ciD->cycles              = overflow_zeros_u64(ci2->cycles, ci1->cycles);
    ciD->stalls.fetch_decode = overflow_zeros_u64(ci2->stalls.fetch_decode, ci1->stalls.fetch_decode);
    ciD->stalls.resources    = overflow_zeros_u64(ci2->stalls.resources, ci1->stalls.resources);
    ciD->stalls.memory       = overflow_zeros_u64(ci2->stalls.memory, ci1->stalls.memory);

#if SHOW_DEBUGS
    cpi_data_print(ciD, 0.0, fderr);
#endif
    // If instructions is 0, we convert them to 1
    if (ciD->instructions == 0 || ciD->cycles == 0) {
        ciD->instructions = (ciD->instructions > 0) ? ciD->instructions : 1;
        ciD->cycles       = (ciD->cycles > 0) ? ciD->cycles : 1;
        // And if CPI is not NULL, we return the last valid CPI
        if (cpi != NULL) {
            *cpi = cpi_copy;
        }
    } else if (cpi != NULL) {
        *cpi = ((double) ciD->cycles) / ((double) ciD->instructions);
        // Saving valid CPI
        cpi_copy = *cpi;
    }
}

void cpi_data_copy(cpi_t *dst, cpi_t *src)
{
    memcpy(dst, src, sizeof(cpi_t));
}

void cpi_data_print(cpi_t *ci, double cpi, int fd)
{
    char buffer[SZ_BUFFER];
    cpi_data_tostr(ci, cpi, buffer, SZ_BUFFER);
    dprintf(fd, "%s", buffer);
}

void cpi_data_tostr(cpi_t *ci, double cpi, char *buffer, size_t length)
{
    snprintf(buffer, length,
             "Instructions      : %llu\n"
             "Cycles            : %llu (cpi: %0.2lf)\n"
             "Stalls (fetch-dec): %llu\n"
             "Stalls (resources): %llu\n"
             "Stalls (memory)   : %llu\n",
             ci->instructions, ci->cycles, cpi, ci->stalls.fetch_decode, ci->stalls.resources, ci->stalls.memory);
}

#if TEST
#include <time.h>

static topology_t tp;
static apinfo_t info;
static cpi_t c1;
static cpi_t c2;
static cpi_t cD;
static double gbs;

int main(int argc, char *argv[])
{
    topology_init(&tp);
    cpi_load(&tp, API_FREE);
    cpi_get_info(&info);

    cpi_init(no_ctx);
    apinfo_tostr(&info);
    printf("%s\n", info.api_str);
    cpi_read(no_ctx, &c1);
    sleep(1);
    cpi_read_diff(no_ctx, &c2, &c1, &cD, &gbs);

    return 0;
}
#endif