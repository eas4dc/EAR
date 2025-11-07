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
#include <common/states.h>
#include <common/system/symplug.h>
#include <metrics/common/rsmi.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef RSMI_BASE
#define RSMI_PATH ""
#else
#define RSMI_PATH RSMI_BASE
#endif
#define RSMI_LIB "librocm_smi64.so"
#define RSMI_N   21

#define ccv(f)                                                                                                         \
    if (f != RSMI_STATUS_SUCCESS) {                                                                                    \
        debug(#f ": error");                                                                                           \
    }

// https://docs.amd.com/bundle/rocm_smi_lib.5.0/page/globals_func.html
static const char *rsmi_names[] = {
    "rsmi_init",
    "rsmi_num_monitor_devices",
    "rsmi_dev_serial_number_get",
    "rsmi_dev_energy_count_get",
    "rsmi_dev_power_ave_get",
    "rsmi_dev_power_cap_get",
    "rsmi_dev_power_cap_default_get",
    "rsmi_dev_clk_range_set",
    "rsmi_dev_power_cap_set",
    "rsmi_dev_temp_metric_get",
    "rsmi_dev_gpu_clk_freq_get",
    "rsmi_dev_od_clk_info_set",
    "rsmi_dev_clk_range_set",
    "rsmi_utilization_count_get",
    "rsmi_dev_busy_percent_get",
    "rsmi_dev_memory_busy_percent_get",
    "rsmi_dev_memory_total_get",
    "rsmi_dev_memory_usage_get",
    "rsmi_dev_pci_bandwidth_get",
    "rsmi_compute_process_info_get",
    "rsmi_compute_process_gpus_get",
};

// Nomenclatures:
// SYS  -> system (GPU, i think stream processors)
// DF   -> data fabric
// DCEF -> display controller engine
// SOC  -> system on a chip
// MEM  -> memory
// sclk -> system clock
// mclk -> memory clock
// fclk -> ¿df clock? It seems

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static rsmi_t rsmi;
static int ok;

static state_t library_load()
{
#define _open_test(path)                                                                                               \
    debug("Openning %s", path);                                                                                        \
    if (state_ok(plug_open(path, (void **) &rsmi, rsmi_names, RSMI_N, RTLD_NOW | RTLD_LOCAL))) {                       \
        return EAR_SUCCESS;                                                                                            \
    } else {                                                                                                           \
        debug("Failed: %s", state_msg);                                                                                \
    }

#define open_test(path)                                                                                                \
    _open_test(path RSMI_LIB);                                                                                         \
    _open_test(path RSMI_LIB ".1");

    _open_test(getenv("HACK_RSMI_FILE"));
    open_test(RSMI_PATH "/targets/x86_64-linux/lib/");
    open_test(RSMI_PATH "/lib64/");
    open_test(RSMI_PATH "/lib/");
    open_test("/usr/lib/targets/x86_64-linux/lib/");
    open_test("/usr/lib64/");
    open_test("/usr/lib/");

    return_msg(EAR_ERROR, "library not found");
}

static state_t library_init()
{
    uint devs;
    if (rsmi.init(0) != RSMI_STATUS_SUCCESS) {
        debug("rsmi.init(0) failed");
        return EAR_ERROR;
    }
    if (rsmi.devs_count(&devs) != RSMI_STATUS_SUCCESS) {
        debug("rsmi.devs_count(&devs) failed");
        return EAR_ERROR;
    }
    if (devs == 0) {
        return_msg(EAR_ERROR, "no devices detected");
    }
    return EAR_SUCCESS;
}

static void library_destroy()
{
    dlclose(&rsmi);
    memset(&rsmi, 0, sizeof(rsmi_t));
}

state_t rsmi_open(rsmi_t *rsmi_in)
{
#ifndef RSMI_BASE
    return EAR_ERROR;
#endif
    state_t s = EAR_SUCCESS;
    while (pthread_mutex_trylock(&lock))
        ;
    if (ok) {
        goto fini;
    }
    if (state_fail(s = library_load())) {
        goto fini;
    }
    if (state_fail(s = library_init())) {
        library_destroy();
        goto fini;
    }
    ok = 1;
fini:
    if (ok && rsmi_in != NULL) {
        memcpy(rsmi_in, &rsmi, sizeof(rsmi_t));
    }
    pthread_mutex_unlock(&lock);
#if 0
    rsmi_util_t util[1024];
    rsmi_freqs_t freqs;
    ullong ullong1;
    ullong ullong2;
    float float1;
    uint uint1;

    rsmi.init(0);
    if (rsmi.devs_count(&uint1) != RSMI_STATUS_SUCCESS) {}
    debug("#Devs %u", uint1);
    ccv(rsmi.get_power(0, 0, &ullong1));
    debug("#Power %llu", ullong1);
    ccv(rsmi.get_power(0, 1, &ullong1));
    debug("#Power %llu", ullong1);
    if (rsmi.get_energy(0, &ullong1, &float1, &ullong2) != RSMI_STATUS_SUCCESS) {}
    debug("#Energy %llu %f %llu", ullong1, float1, ullong2);
    if (rsmi.get_temperature(0, RSMI_TEMP_TYPE_EDGE, RSMI_TEMP_CURRENT, &ullong1) != RSMI_STATUS_SUCCESS) {}
    debug("#Temperature1 %llu", ullong1);
    if (rsmi.get_temperature(0, RSMI_TEMP_TYPE_JUNCTION, RSMI_TEMP_CURRENT, &ullong1) != RSMI_STATUS_SUCCESS) {}
    debug("#Temperature2 %llu", ullong1);
    util[0].type = RSMI_COARSE_GRAIN_GFX_ACTIVITY;
    if (rsmi.get_utilization(0, util, 1024, &ullong1) != RSMI_STATUS_SUCCESS) {}
    debug("#Utilization %lu (%u)", util[0].value, util[0].type);
    if (rsmi.get_clock(0, RSMI_CLK_TYPE_SYS, &freqs) != RSMI_STATUS_SUCCESS) {}
    debug("#Clock %lu", freqs.frequency[0]);
#endif
    return s;
}

state_t rsmi_close()
{
    return EAR_SUCCESS;
}

int rsmi_is_privileged()
{
    // Provisional
    return (getuid() == 0);
}

#if 0
int main(int argc, char *argv[])
{
    rsmi_open(NULL);
    return 0;
}
#endif
