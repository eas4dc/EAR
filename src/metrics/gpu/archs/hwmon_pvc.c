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
#include <common/system/monitor.h>
#include <common/system/symplug.h>
#include <dlfcn.h>
#include <metrics/common/hwmon.h>
#include <metrics/gpu/archs/hwmon_pvc.h>
#include <pthread.h>
#include <stdlib.h>

static hwmon_t *chips;
static uint chips_count;
static uint items_count;

static void close_all()
{
    if (chips != NULL) {
        hwmon_close(&chips);
        chips_count = 0;
        items_count = 0;
        chips       = NULL;
    }
}

GPU_F_LOAD(hwmon_pvc)
{
    if (state_fail(hwmon_open("i915", "energy", NULL, &chips, &chips_count))) {
        debug("Failed: %s", state_msg);
        return;
    }
    debug("chips_count: %u", chips_count);
    if ((items_count = hwmon_count_items(chips, "input", NULL)) == 0) {
        close_all();
        return;
    }
    debug("items_count: %u", items_count);
    apis_pif(ops->unload, gpu_hwmon_pvc_unload, 1);
    apis_pif(ops->get_info, gpu_hwmon_pvc_get_info, 1);
    apis_pif(ops->topology_get, gpu_hwmon_pvc_topology_get, 1);
    apis_pif(ops->read, gpu_hwmon_pvc_read, 1);
    apis_pif(ops->read_raw, gpu_hwmon_pvc_read_raw, 1);
    debug("Loaded HWMON_PVC");
}

GPU_F_UNLOAD(hwmon_pvc)
{
    close_all();
}

GPU_F_GET_INFO(hwmon_pvc)
{
    info->api         = API_HWMON;
    info->devs_count  = items_count;
    info->granularity = GRANULARITY_PERIPHERAL;
    info->scope       = SCOPE_NODE;
}

GPU_F_TOPOLOGY_GET(hwmon_pvc)
{
    int i;

    tp->devs_count = items_count;
    tp->devs       = calloc(items_count, sizeof(gpu_devs_t));

    for (i = 0; i < items_count; ++i) {
        tp->devs[i].index            = i;
        tp->devs[i].index_device     = -1;
        tp->devs[i].is_readable      = 1;
        tp->devs[i].is_subdevice     = 0;
        tp->devs[i].has_subdevices   = 0;
        tp->devs[i].subdevices_count = 0;
        tp->devs[i].serial           = (ullong) i;
    }
}

GPU_F_READ(hwmon_pvc)
{
    static uint samples = 0;
    timestamp_t time;
    hwmon_t *chip;
    int c = 0;

    hwmon_read(chips);
    timestamp_getfast(&time);
    memset(d, 0, sizeof(gpu_t) * items_count);

    while ((chip = hwmon_iter_chips(chips)) != NULL) {
        d[c].energy_j = atof(chip->devs->input) / 1000000.0;
        debug("D%d: %lf J", c, d[c].energy_j);
        d[c].util_gpu = 100 * samples;
        d[c].util_mem = 100 * samples;
        d[c].freq_gpu = 1600000 * samples;
        d[c].freq_mem = 1000000 * samples;
        d[c].time     = time;
        d[c].samples  = samples;
        d[c].working  = 1;
        d[c].correct  = 1;
        ++c;
    }
    debug("----");
    return EAR_SUCCESS;
}

GPU_F_READ_RAW(hwmon_pvc)
{
    return gpu_hwmon_pvc_read(d);
}

#if TEST
static gpu_ops_t ops;
static gpu_t d1[4];
static gpu_t d2[4];
static gpu_t dD[4];

int main(int argc, char **argv)
{
    gpu_load(0);
    gpu_read(d1);
    sleep(1);
    gpu_read_diff(d2, d1, dD);
    return 0;
}
#endif
