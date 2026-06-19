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
#include <string.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/common/apis.h>
#include <metrics/proc/proc.h>
#include <metrics/proc/archs/dummy.h>
#include <metrics/proc/archs/stat_file.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static proc_ops_t ops;
static apinfo_t info;

void proc_load(topology_t *tp, int options)
{
    while (pthread_mutex_trylock(&lock));
    if (info.api != API_NONE) {
        goto leave;
    }
    if (API_IS(options, API_DUMMY)) {
        goto dummy;
    }
    proc_stat_file_load(tp, &ops, options);
dummy:
    proc_dummy_load(tp, &ops, options);
    proc_get_info(&info);
leave:
    pthread_mutex_unlock(&lock);
}

void proc_unload()
{
    while (pthread_mutex_trylock(&lock));
    if (info.api != API_NONE) {
        ops.unload();
        memset(&ops, 0, sizeof(proc_ops_t));
        memset(&info, 0, sizeof(apinfo_t));
    }
    pthread_mutex_unlock(&lock);
}

state_t proc_update(uint option, void *value)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
    s = ops.update(option, value);
    pthread_mutex_unlock(&lock);
    return s;
}

void proc_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    info->layer = "PROC";
    info->api   = API_NONE;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t proc_read(proc_t *pr)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
    memset(pr, 0, sizeof(proc_t)*info.devs_count);
    s = ops.read(pr);
    pthread_mutex_unlock(&lock);
    return s;
}

// Helpers
state_t proc_read_diff(proc_t *pr2, proc_t *pr1, proc_t *prD)
{
    state_t s;
    if (state_fail(s = ops.read(pr2))) {
        return s;
    }
    proc_data_diff(pr2, pr1, prD);
    return s;
}

state_t proc_read_copy(proc_t *pr2, proc_t *pr1, proc_t *prD)
{
    state_t s;
    if (state_fail(s = proc_read_diff(pr2, pr1, prD))) {
        return s;
    }
    proc_data_copy(pr1, pr2);
    return s;
}

void proc_data_diff(proc_t *pr2, proc_t *pr1, proc_t *prD)
{
    int i;

    memset(prD, 0, sizeof(proc_t)*info.devs_count);
    for (i = 0; i < info.devs_count; ++i) {
        if (pr2[i].pid == 0 || pr1[i].pid == 0) {
            continue;
        }
        if ((prD[i].secs = timestamp_fdiff(&pr2[i].time, &pr1[i].time, TIME_SECS, TIME_MSECS)) == 0.0) {
            continue;
        }
        prD[i].pid   = pr1[i].pid;
        prD[i].utime = overflow_zeros_f64(pr2[i].utime, pr1[i].utime);
        prD[i].stime = overflow_zeros_f64(pr2[i].stime, pr1[i].stime);
        prD[i].cpu_util = (uint) (100.0 * ((prD[i].utime + prD[i].stime) / prD[i].secs));
        prD[i].cpu_util = (prD[i].cpu_util > 100) ? 100 : prD[i].cpu_util;
    }
    #if SHOW_DEBUGS
    proc_data_print(prD, fderr);
    #endif
}

void proc_data_reduce(proc_t *prD, proc_t *prA)
{
    int i;

    memset(prA, 0, sizeof(proc_t));
    for (i = 0; i < info.devs_count; ++i) {
        if (prD[i].pid == 0) {
            continue;
        }
        prA->pid    = 1;
        prA->secs  += prD[i].secs;
        prA->utime += prD[i].utime;
        prA->stime += prD[i].stime;
    }
    prA->cpu_util = (uint) (100.0 * ((prA->utime + prA->stime) / prA->secs));
    prA->cpu_util = (prA->cpu_util > 100) ? 100: prA->cpu_util;
    printf("UTIL %u\n", prA->cpu_util);
}

void proc_data_alloc(proc_t **pr)
{
    if (pr != NULL) {
        *pr = calloc(info.devs_count, sizeof(proc_t));
    }
}

void proc_data_free(proc_t **pr)
{
    if (pr != NULL) {
        free(*pr);
        *pr = NULL;
    }
}

void proc_data_copy(proc_t *dst, proc_t *src)
{
    memcpy(dst, src, sizeof(proc_t)*info.devs_count);
}

void proc_data_print(proc_t *pr, int fd)
{
    char buffer[4096] = {0};
    proc_data_tostr(pr, buffer, SZ_BUFFER);
    dprintf(fd, "%s", buffer);
}

char *proc_data_tostr(proc_t *pr, char *buffer, size_t length)
{
    int i, b, w;
    buffer[0] = '\0';
    for (i = b = 0; i < info.devs_count && length > 0; ++i) {
        if (pr[i].pid == 0) {
            continue;
        }
        w = snprintf(&buffer[b], length-1, "util %u (%lf %lf) %lf\n",
                pr[i].cpu_util, pr[i].utime, pr[i].stime, pr[i].secs);
        w = (w < (length-1))? w: length;
        b += w, length -= w;
    }
    return buffer;
}
