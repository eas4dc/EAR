/*********************************************************************
 * Copyright (c) 2026 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

/*
 * fake_hw_mgt.h -- everything powercap_mgt.c needs below it, faked:
 *
 *  - four in-process domain plugins (node, cpu, dram, gpu) that replace the
 *    .so files symplug_open() would load. Each records what the pmgt layer
 *    asks of it, and returns a status the test scripts;
 *  - the hardware discovery powercap_mgt.c performs at init (topology, GPU
 *    API and count, CPU/DRAM/GPU TDPs, the load average, GPU raw readings).
 *
 * A plugin is "loaded" when fake_plug[d].loaded is set and the matching
 * EAR_POWERCAP_POLICY_<DOMAIN> env var names "fake_<domain>".
 */
#ifndef FAKE_HW_MGT_H
#define FAKE_HW_MGT_H

#include <common/config.h>
#include <common/system/monitor.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <metrics/gpu/gpu.h>

#define FAKE_MAX_DEV 16

typedef struct fake_plugin {
    /* configuration */
    int loaded;
    domain_settings_t settings;
    domain_status_t status;      /* returned by get_powercap_status; current_pc is overwritten with cur */
    uint32_t dev_caps[FAKE_MAX_DEV]; /* returned by get_powercap_value */
    state_t set_ret;
    int no_strategy;             /* do not export get_powercap_strategy */
    int owns_dram;               /* export set/get_dram_powercap_value (cpu_generic caps DRAM) */
    uint strategy;
    /* observations */
    uint32_t cur;                /* last node/domain level limit */
    int set_calls;
    uint last_level;
    uint32_t last_limits[FAKE_MAX_DEV];
    ulong last_util[FAKE_MAX_DEV];
    int reset_calls, relax_calls, burst_calls, new_util_calls, status_calls, enable_calls, disable_calls;
    ulong new_util[FAKE_MAX_DEV];
    uint last_set_status;
    int app_freq_calls;
    ulong app_freq[FAKE_MAX_DEV];
    uint last_pc_mode;
} fake_plugin_t;

typedef struct fake_hw {
    int sockets, cpus, cores;
    int gpus;
    uint gpu_api;               /* API_NVML, API_DUMMY, ... */
    uint32_t pkg_tdp[FAKE_MAX_DEV];
    uint32_t dram_tdp[FAKE_MAX_DEV];
    uint32_t gpu_tdp[FAKE_MAX_DEV];
    uint32_t gpu_min[FAKE_MAX_DEV];
    uint32_t pkg_min[FAKE_MAX_DEV];  /* RAPL POWER_INFO minimum (0 = unknown) */
    uint32_t dram_min[FAKE_MAX_DEV];
    float load;                 /* loadavg() first value */
    gpu_t gpu_raw[FAKE_MAX_DEV];/* what gpu_read_raw() returns */
    uint policy_pstate;
    uint32_t dram_cap[FAKE_MAX_DEV]; /* last mgt_cpupow_powercap_set(CPUPOW_DRAM) */
    uint32_t stored_dram[FAKE_MAX_DEV]; /* powercap_set_stored_device_value(DOMAIN_DRAM) */
    int dram_not_capable;               /* CPU without a DRAM power limit (AMD) */
} fake_hw_t;

extern fake_plugin_t fake_plug[NUM_DOMAINS];
extern fake_hw_t fake_hw;

/* Default rig: 2 sockets (PKG TDP 200 W, DRAM TDP 40 W), 4 NVML GPUs (TDP 400 W,
 * min 100 W), 16 CPUs, cpu+gpu plugins loaded with cpu_generic/gpu settings. */
void fake_hw_default(int num_gpus);
/* Apply the plugin env vars for whatever fake_plug[].loaded says */
void fake_hw_export_env(void);

ulong fake_pstate_freq(uint p);

#endif
