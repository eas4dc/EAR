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
#include <data_center_monitor/plugins/management.h>
#include <data_center_monitor/plugins/metrics.h>
#include <string.h>

// Metrics
static metrics_read_t mr1;
static metrics_read_t mr2;
static metrics_diff_t mrD;
// Information
static int devs_count;
static const ulong **freq_list; // KHz
static ulong *freq_list_set;
static const uint *freq_list_count;
static ulong *pcap_list_max; // W
static ulong *pcap_list_min;
static ulong *pcap_list_set;
static int *list_doing;
static uint *list_accum;
static int flopped;
static strtable_t table;

declr_up_get_tag()
{
    *tag       = "test_gpu";
    *tags_deps = "<!metrics+!management";
}

declr_up_action_init(_metrics)
{
    metrics_data_alloc(&mr1, &mr2, &mrD);
    return NULL;
}

declr_up_action_init(_test_gpu)
{
    char **args = (char **) data;
    uint devs_count_met;
    uint devs_count_mgt;
    int i;

    // Counting devices
    gpu_count_devices(no_ctx, &devs_count_met);
    mgt_gpu_count_devices(no_ctx, &devs_count_mgt);
    // If number of GPU devices differ
    if (devs_count_met != devs_count_mgt) {
        return "[D] Number of metrics and management GPUs differ";
    }
    devs_count = devs_count_mgt;
    // Allocating space
    mgt_gpu_data_alloc(&pcap_list_min);
    mgt_gpu_data_alloc(&pcap_list_max);
    mgt_gpu_data_alloc(&pcap_list_set);
    mgt_gpu_data_alloc(&freq_list_set);
    mgt_gpu_freq_get_available(no_ctx, &freq_list, &freq_list_count);
    mgt_gpu_power_cap_get_rank(no_ctx, pcap_list_min, pcap_list_max);
    mgt_gpu_power_cap_get_rank(no_ctx, pcap_list_min, pcap_list_set);
    //
    for (i = 0; i < devs_count; ++i) {
        freq_list_set[i] = freq_list[i][0];
        fprintf(stderr, "GPU%d: %lu-%lu KHz, %lu-%lu W\n", i, freq_list[i][0], freq_list[i][freq_list_count[i] - 1],
                pcap_list_max[i], pcap_list_min[i]);
    }
    //
    list_doing = calloc(devs_count, sizeof(uint));
    list_accum = calloc(devs_count, sizeof(uint));
    // Configuration
    if (args == NULL || !rantoa(args[0], (uint *) list_doing, devs_count)) {
        // If the list is empty, we are selecting the GPU0.
        list_doing[0] = 1;
    }
    // Configuring strings
    return NULL;
}

static int test_powercap()
{
    int dev, reset = 0;

    // Is time to reset?
    for (dev = 0; dev < devs_count; ++dev) {
        if (list_doing[dev] && pcap_list_set[dev] <= pcap_list_min[dev]) {
            reset = 1;
            continue;
        }
    }
    // Calculating next power cap
    for (dev = 0; dev < devs_count; ++dev) {
        if (!list_doing[dev]) {
            continue;
        }
        if (reset) {
            debug("GPU%d: re-setting to %lu W", dev, freq_list[dev][0]);
            pcap_list_set[dev] = (long) pcap_list_max[dev];
            mgt_gpu_power_cap_reset_dev(no_ctx, dev);
            list_accum[dev] = 0;
            continue;
        }
        pcap_list_set[dev] = (long) pcap_list_max[dev] - list_accum[dev];
        debug("GPU%d: setting to %lu W", dev, pcap_list_set[dev]);
        mgt_gpu_power_cap_set_dev(no_ctx, pcap_list_set[dev], dev);
        // Next time
        list_accum[dev] += 20;
    }
    return reset;
}

static int test_frequency()
{
    int dev, reset = 0;
    uint ten;

    // Is time to reset?
    for (dev = 0; dev < devs_count; ++dev) {
        if (list_doing[dev] && list_accum[dev] >= freq_list_count[dev]) {
            reset = 1;
            continue;
        }
    }
    // Calculating next frequency
    for (dev = 0; dev < devs_count; ++dev) {
        if (!list_doing[dev]) {
            continue;
        }
        if (reset) {
            debug("GPU%d: re-setting to PS%u (%lu KHz)", dev, 0, freq_list[dev][0]);
            freq_list_set[dev] = freq_list[dev][0];
            mgt_gpu_freq_limit_reset_dev(no_ctx, dev);
            list_accum[dev] = 0;
            continue;
        }
        freq_list_set[dev] = freq_list[dev][list_accum[dev]];
        debug("GPU%d: setting to PS%u (%lu KHz)", dev, list_accum[dev], freq_list_set[dev]);
        if (state_fail(mgt_gpu_freq_limit_set_dev(no_ctx, freq_list_set[dev], dev))) {
            debug("GPU%d error while changing frequency: %s", dev, state_msg);
        }
        // Next time
        if ((ten = freq_list_count[dev] / 10U) == 0U) {
            ten = 1;
        }
        list_accum[dev] += ten;
    }
    return reset;
}

char *test_select()
{
    static int flip_flop = 0;
    if (!flip_flop) {
        flip_flop = test_frequency();
        flopped   = flip_flop;
    } else {
        flip_flop = !test_powercap();
        flopped   = !flip_flop;
    }
    return NULL;
}

static char *print_table(int init)
{
    if (init) {
        tprintf_init2(&table, fdout, STR_MODE_DEF, "6 4 7 6 3 10 10 10 10 10 10 6 6");
    }
    if (init) {
        printf("--------- | ------------ | -------------------------------------------------------------------- | "
               "--------------\n");
    }
    if (init) {
        printf("General   | Capping      | Metrics                                                              | "
               "System Metrics\n");
    }
    printf("--------- | ------------ | -------------------------------------------------------------------- | "
           "--------------\n");
    tprintf2(&table, "Step||GPU|||Freq.||Power|||R||GPU Freq.||MEM Freq.||GPU Util.||MEM Util.||GPU Temp.||MEM "
                     "Temp.||Power|||Power");
    tprintf2(&table, "----||---|||-----||-----|||-||---------||---------||---------||---------||---------||---------||-"
                     "----|||-----");
    return NULL;
}

declr_up_action_periodic(_metrics)
{
    mets_t *mets = (mets_t *) data;
    int dev;

    if (mr1.samples == 0) {
        metrics_data_copy(&mr1, &mets->mr);
        return print_table(1);
    }
    if (flopped) {
        print_table(0);
        flopped = 0;
    }
    metrics_data_copy(&mr2, &mets->mr);
    metrics_data_diff(&mr2, &mr1, &mrD);
    metrics_data_copy(&mr1, &mr2);

    for (dev = 0; dev < devs_count; ++dev) {
        char *set_mhz = "";
        char *set_wat = "";
        if (freq_list_set[dev] != freq_list[dev][0])
            set_mhz = "*";
        if (pcap_list_set[dev] != pcap_list_max[dev])
            set_wat = "*";

        tprintf2(&table, "%llu||%d|||%4lu%s||%lu%s|||%u||%lu||%lu||%lu||%lu||%lu||%lu||%lu|||%lu", mr2.samples, dev,
                 freq_list_set[dev] / 1000LU, set_mhz, pcap_list_set[dev], set_wat, mrD.gpu_diff[dev].working,
                 mrD.gpu_diff[dev].freq_gpu / 1000LU, mrD.gpu_diff[dev].freq_mem / 1000LU, mrD.gpu_diff[dev].util_gpu,
                 mrD.gpu_diff[dev].util_mem, mrD.gpu_diff[dev].temp_gpu, mrD.gpu_diff[dev].temp_mem,
                 (ulong) mrD.gpu_diff[dev].power_w, mrD.nod_avrg);
    }
    return NULL;
}

declr_up_action_periodic(_test_gpu)
{
    return test_select();
}
