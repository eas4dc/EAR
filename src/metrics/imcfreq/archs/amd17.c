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

#define _GNU_SOURCE
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <metrics/common/hsmp.h>
#include <metrics/imcfreq/archs/amd17.h>

static uint sockets_count;

IMCFREQ_F_LOAD(amd17)
{
    // Already loaded
    if (state_fail(hsmp_open(tp_in, HSMP_RD))) {
        return;
    }
    sockets_count = tp_in->socket_count;
    // Bypassing
    apis_set(ops->unload, imcfreq_amd17_unload);
    apis_set(ops->get_info, imcfreq_amd17_get_info);
    apis_set(ops->read, imcfreq_amd17_read);
    apis_set(ops->data_diff, imcfreq_amd17_data_diff);
    debug("AMD17 loaded full API");
}

IMCFREQ_F_UNLOAD(amd17)
{
    hsmp_close();
}

IMCFREQ_F_GET_INFO(amd17)
{
    info->api         = API_AMD17;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = sockets_count;
}

static state_t read_current_freqs(ullong *freqs_khz)
{
    uint args[3] = {0, 0, -1};
    state_t s;
    uint i;

    for (i = 0; i < sockets_count; ++i) {
        // Function ReadCurrentFclkMemclk (0x0f). 0 arguments, 2 answers.
        // args[0] = 0;
        // args[1] = 0;
        if (state_fail(s = hsmp_send(i, HSMP_GET_FCLK_MCLK, &args[2], &args[0]))) {
            return s;
        }
        if (args[0] == 0 || args[0] == -1) {
            return_msg(EAR_ERROR, "Incorrect result when asking for frequency by HSMP");
        }
        freqs_khz[i] = ((ullong) args[0]) * 1000LLU;
        debug("SOCKET%d: %llu KHz", i, freqs_khz[i]);
    }
    return EAR_SUCCESS;
}

IMCFREQ_F_READ(amd17)
{
    ullong freqs_khz[8]; // Up to 8 sockets
    state_t s;
    int cpu;

    if (state_fail(s = read_current_freqs(freqs_khz))) {
        return s;
    }
    timestamp_getfast(&list[0].time);
    // Iterating per socket.
    for (cpu = 0; cpu < sockets_count; ++cpu) {
        debug("AMD17 read %llu", list[cpu].khz);
        list[cpu].freq = (ulong) freqs_khz[cpu];
    }
    return EAR_SUCCESS;
}

IMCFREQ_F_DATA_DIFF(amd17)
{
    ulong aux1 = 0;
    ulong aux2 = 0;
    uint cpu;
    debug("imcfreq_amd17_data_diff %u devices", sockets_count);
    for (cpu = 0; cpu < sockets_count; ++cpu) {
        // aux1 = (l2[cpu].freq + l1[cpu].freq) / 2LU;
        debug("IMCFREQ [%d] %lu", cpu, l2[cpu].freq);
        aux1 = l2[cpu].freq;
        if (ldiff != NULL) {
            ldiff[cpu] = aux1;
        }
        aux2 += aux1;
    }
    if (freq_avg != NULL) {
        *freq_avg = aux2;
        if (sockets_count > 0) {
            *freq_avg = *freq_avg / (ulong) sockets_count;
        }
    }
}