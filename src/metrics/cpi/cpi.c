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
//#define SHOW_DEBUGS 1
#include <string.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/common/apis.h>
#include <metrics/cpi/cpi.h>
#include <metrics/cpi/archs/perf.h>
#include <metrics/cpi/archs/dummy.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static cpi_ops_t ops;
static apinfo_t info;

void cpi_load(topology_t *tp, int options)
{
    while (pthread_mutex_trylock(&lock));
    if (info.api != API_NONE) {
        goto leave;
    }
    if (API_IS(options, API_DUMMY)) {
        goto dummy;
    }
    cpi_perf_load(tp, &ops, options);
dummy:
    cpi_dummy_load(tp, &ops, options);
    // Bandwidth wants to know more about the loaded API. This is safe because at
    // this point all API's have their devices counter.
    cpi_get_info(&info);
leave:
    pthread_mutex_unlock(&lock);
}

void cpi_unload()
{
    while (pthread_mutex_trylock(&lock));
    if (info.api != API_NONE) {
        ops.unload();
        memset(&ops, 0, sizeof(cpi_ops_t));
        memset(&info, 0, sizeof(apinfo_t));
    }
    pthread_mutex_unlock(&lock);
}

state_t cpi_update(uint option, void *value)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
    s = ops.update(option, value);
    pthread_mutex_unlock(&lock);
    return s;
}

void cpi_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    info->layer = "CPI";
    info->api   = API_NONE;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t cpi_read(cpi_t *ci)
{
    state_t s;
    memset(ci, 0, sizeof(cpi_t)*info.devs_count);
    while (pthread_mutex_trylock(&lock));
    s = ops.read(ci);
    pthread_mutex_unlock(&lock);
    return s;
}

// Helpers
state_t cpi_read_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpis_avg)
{
    state_t s;
    if (state_fail(s = ops.read(ci2))) {
        return s;
    }
    cpi_data_diff(ci2, ci1, ciD, cpis_avg);
    return s;
}

state_t cpi_read_copy(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpis_avg)
{
    state_t s;
    if (state_fail(s = cpi_read_diff(ci2, ci1, ciD, cpis_avg))) {
        return s;
    }
    cpi_data_copy(ci1, ci2);
    return s;
}

void cpi_data_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpis_avg)
{
    double aux = 0.0;
    int i;

    memset(ciD, 0, sizeof(cpi_t)*info.devs_count);
    for (i = 0; i < info.devs_count; ++i) {
        if (ci2[i].pid == 0 || ci1[i].pid == 0) {
            continue;
        }
        ciD[i].pid                 = ci1[i].pid;
        ciD[i].instructions        = overflow_zeros_u64(ci2[i].instructions       , ci1[i].instructions       );
        ciD[i].cycles              = overflow_zeros_u64(ci2[i].cycles             , ci1[i].cycles             );
        ciD[i].stalls.fetch_decode = overflow_zeros_u64(ci2[i].stalls.fetch_decode, ci1[i].stalls.fetch_decode);
        ciD[i].stalls.resources    = overflow_zeros_u64(ci2[i].stalls.resources   , ci1[i].stalls.resources   );
        ciD[i].stalls.memory       = overflow_zeros_u64(ci2[i].stalls.memory      , ci1[i].stalls.memory      );
        if (ciD[i].instructions != 0 && ciD[i].cycles != 0) {
            ciD[i].cpi = ((double) ciD[i].cycles) / ((double) ciD[i].instructions);
        }
        aux += ciD[i].cpi;
    }
    aux = aux / (double) info.devs_count;
    if (cpis_avg != NULL) {
        *cpis_avg = aux;
    }
    #if SHOW_DEBUGS
    cpi_data_print(ciD, aux, fderr);
    #endif
}

void cpi_data_alloc(cpi_t **ci)
{
    if (ci != NULL) {
        *ci = calloc(info.devs_count, sizeof(cpi_t));
    }
}

void cpi_data_free(cpi_t **ci)
{
    if (ci != NULL) {
        free(*ci);
        *ci = NULL;
    }
}

void cpi_data_copy(cpi_t *dst, cpi_t *src)
{
    memcpy(dst, src, sizeof(cpi_t) * info.devs_count);
}

void cpi_data_print(cpi_t *ci, double cpis_avg, int fd)
{
    char buffer[SZ_BUFFER]={0};
    cpi_data_tostr(ci, cpis_avg, buffer, SZ_BUFFER);
    dprintf(fd, "%s", buffer);
}

char *cpi_data_tostr(cpi_t *ci, double cpis_avg, char *buffer, size_t length)
{
    int i, b, w;
    buffer[0] = '\0';
    for (i = b = w = 0; i < info.devs_count && length > 0; ++i) {
        if (ci[i].pid == 0) {
            continue;
        }
        w = snprintf(&buffer[b], length-1,
            "dev%d "
            "%014llu ins, %014llu cycles, %0.2lf cpi, "
            "%llu-%llu-%llu stalls\n", i,
            ci[i].instructions, ci[i].cycles, ci[i].cpi,
            ci[i].stalls.fetch_decode,
            ci[i].stalls.resources,
            ci[i].stalls.memory);
        w = (w < (length-1))? w: length;
        b += w, length -= w;
    }
    return buffer;
}

#if TEST
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static topology_t tp;
static apinfo_t   info;
static cpi_t     *t1;
static cpi_t     *t2;
static cpi_t     *tD;
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
    cpi_load(&tp, SCOPE_JOB | API_FREE);
    cpi_get_info(&info);
    apinfo_tostr(&info);
    dprintf(STDOUT_FILENO, "%d: Loaded %s (%s:%s), with %u devices and %d fds\n",
        getpid(), info.api_str, info.scope_str, info.granularity_str, info.devs_count, count_fds());
    cpi_data_alloc(&t1);
    cpi_data_alloc(&t2);
    cpi_data_alloc(&tD);
reread:
    cpi_read(t1);
    sleep(2);
    for (i = j = 0; i < 1000000; ++i) {
        j += 1;
    }
    dprintf(STDOUT_FILENO, "%d: Printing...\n", getpid());
    cpi_read_diff(t2, t1, tD, &tA);
    cpi_read(t2);
    cpi_data_print(t1, 0.0, STDOUT_FILENO);
    cpi_data_print(t2, 0.0, STDOUT_FILENO);
    cpi_data_print(tD, 0.0, STDOUT_FILENO);
    dprintf(STDOUT_FILENO, "--------------------------\n");
    cpi_data_copy(t1, t2);
    #if 1
    if (k++ == 2 || k == 10) {
        cpi_update(UPD_PID_ADD, (void *) 1703051);
    } else if (k == 6 || k == 14){
        cpi_update(UPD_PID_REMOVE, (void *) 1703051);
        //cache_update(UPD_PIDS_CLEAN, NULL);
    }
    #endif
    goto reread;
    return 0;
}
#endif
