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
#include <common/output/verbose.h>
#include <metrics/common/apis.h>
#include <metrics/imcfreq/archs/amd17.h>
#include <metrics/imcfreq/archs/dummy.h>
#include <metrics/imcfreq/archs/eard.h>
#include <metrics/imcfreq/archs/intel63.h>
#include <metrics/imcfreq/imcfreq.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static apinfo_t info;
static imcfreq_ops_t ops;

void imcfreq_load(topology_t *tp, int options)
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (info.api != API_NONE) {
        goto done;
    }
    if (API_IS(options, API_DUMMY)) {
        goto dummy;
    }
    imcfreq_intel63_load(tp, &ops, options);
    imcfreq_amd17_load(tp, &ops, options);
    imcfreq_eard_load(tp, &ops, options);
dummy:
    imcfreq_dummy_load(tp, &ops, options);
    imcfreq_get_info(&info);
done:
    pthread_mutex_unlock(&lock);
}

void imcfreq_unload()
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (ops.unload != NULL) {
        ops.unload();
        memset(&ops, 0, sizeof(imcfreq_ops_t));
        memset(&info, 0, sizeof(apinfo_t));
    }
    pthread_mutex_unlock(&lock);
}

void imcfreq_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    info->layer = "IMCFREQ";
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t imcfreq_read(imcfreq_t *i)
{
    if (i == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    }
    preturn(ops.read, i);
}

state_t imcfreq_read_diff(imcfreq_t *i2, imcfreq_t *i1, ulong *freqs, ulong *average)
{
    state_t s;
    if (state_fail(s = imcfreq_read(i2))) {
        return s;
    }
    imcfreq_data_diff(i2, i1, freqs, average);
    return s;
}

state_t imcfreq_read_copy(imcfreq_t *i2, imcfreq_t *i1, ulong *freqs, ulong *average)
{
    state_t s;
    if (state_fail(s = imcfreq_read_diff(i2, i1, freqs, average))) {
        return s;
    }
    imcfreq_data_copy(i1, i2);
    return s;
}

void imcfreq_data_diff(imcfreq_t *i2, imcfreq_t *i1, ulong *freq_list, ulong *average)
{
    ulong time;
    ulong freq;
    ulong aux1; // Adds frequencies
    ulong aux2; // Counts valid devices
    int cpu;

    if (i2 == NULL || i1 == NULL) {
        return_msg(, Generr.input_null);
    }
    if (ops.data_diff != NULL) {
        return ops.data_diff(i2, i1, freq_list, average);
    }
    if (freq_list != NULL) {
        memset((void *) freq_list, 0, sizeof(ulong) * info.devs_count);
    }
    if (average != NULL) {
        *average = 0LU;
    }
    time = (ulong) timestamp_diff(&i2[0].time, &i1[0].time, TIME_MSECS);
    //
    for (cpu = 0, aux1 = aux2 = 0LU; cpu < info.devs_count; ++cpu) {
        if (freq_list != NULL) {
            freq_list[cpu] = 0LU;
        }
        if (i2[cpu].error || i1[cpu].error || time == 0) {
            continue;
        }
        //
        freq = (i2[cpu].freq - i1[cpu].freq) / time;
        aux1 += freq;
        aux2 += 1;
        //
        if (freq_list != NULL) {
            freq_list[cpu] = freq;
        }
    }
    if (average != NULL) {
        *average = 0LU;
        if (aux2 > 0LU) {
            *average = aux1 / aux2;
        }
    }
}

void imcfreq_data_alloc(imcfreq_t **i, ulong **freq_list)
{
    if (i != NULL) {
        if ((*i = (imcfreq_t *) calloc(info.devs_count, sizeof(imcfreq_t))) == NULL) {
            return_msg(, strerror(errno));
        }
    }
    if (freq_list != NULL) {
        if ((*freq_list = (ulong *) calloc(info.devs_count, sizeof(ulong))) == NULL) {
            return_msg(, strerror(errno));
        }
    }
}

void imcfreq_data_free(imcfreq_t **i, ulong **freq_list)
{
    if (i != NULL && *i != NULL) {
        free(*i);
        *i = NULL;
    }
    if (freq_list != NULL && *freq_list != NULL) {
        free(*freq_list);
        *freq_list = NULL;
    }
}

void imcfreq_data_copy(imcfreq_t *i2, imcfreq_t *i1)
{
    memcpy(i2, i1, sizeof(imcfreq_t) * info.devs_count);
}

void imcfreq_data_print(ulong *freq_list, ulong *average, int fd)
{
    char buffer[SZ_BUFFER];
    imcfreq_data_tostr(freq_list, average, buffer, SZ_BUFFER);
    dprintf(fd, "%s", buffer);
}

char *imcfreq_data_tostr(ulong *freq_list, ulong *average, char *buffer, size_t length)
{
    double freq_ghz;
    int accum = 0;
    int i;

    for (i = 0; freq_list != NULL && i < info.devs_count; ++i) {
        freq_ghz = ((double) freq_list[i]) / 1000000.0;
        accum += sprintf(&buffer[accum], "%0.1lf ", freq_ghz);
    }
    if (average != NULL) {
        freq_ghz = ((double) *average) / 1000000.0;
        accum += sprintf(&buffer[accum], "!%0.1lf ", freq_ghz);
    }
    return buffer;
}