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
 * fake_dev_api.h -- stubbed device APIs for the plugin unit tests.
 *
 * gpu.c talks to management/gpu (caps, limits, frequency list) and metrics/gpu
 * (utilization, power, frequency readings); cpu_generic.c talks to
 * management/cpupow (RAPL caps, TDP), management/cpufreq (P-states) and
 * metrics/energy_cpu (package energy). Both call back into powercap.c
 * (per-device storage) and powercap_mgt.c (requested frequencies). All of
 * that is replaced here by plain arrays the tests set and inspect.
 */
#ifndef FAKE_DEV_API_H
#define FAKE_DEV_API_H

#include <pthread.h>

#include <common/config.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <metrics/gpu/gpu.h>

#define FD_MAX 16

/* -------------------------------------------------------------- GPUs */
typedef struct fake_gpus {
    uint n;
    uint32_t min_w[FD_MAX], max_w[FD_MAX];
    uint32_t hw_cap[FD_MAX];  /* what mgt_gpu_power_cap_set() last wrote */
    int set_calls;
    ulong freqs[128];         /* available frequencies, kHz, descending */
    uint nfreqs;
    gpu_t reading[FD_MAX];    /* what gpu_read() returns (used as the diff too) */
    ulong target_khz[FD_MAX]; /* pmgt_get_app_req_freq(DOMAIN_GPU) */
    state_t init_ret;
    ulong clock_khz[FD_MAX];  /* current clock limit (mgt_gpu_freq_limit_*) */
    int clock_set_calls;
    /* Optional closed loop: when w_per_mhz > 0 a busy GPU draws
     * idle_w + w_per_mhz * clock_MHz, an idle one idle_w */
    double idle_w, w_per_mhz;
} fake_gpus_t;
extern fake_gpus_t fgpu;
void fake_gpus_default(uint n);
/* Replaces the frequency list with n evenly spaced steps from max to min (kHz) */
void fake_gpus_freqs(uint n, ulong max_khz, ulong min_khz);
/* Refreshes reading[].power_w from the closed-loop model (no-op without one) */
void fake_gpus_model(void);

/* --------------------------------------------------------------- CPUs */
typedef struct fake_cpus {
    int sockets, cpus;
    uint32_t pkg_tdp[FD_MAX], dram_tdp[FD_MAX];
    uint32_t hw_pkg[FD_MAX], hw_dram[FD_MAX]; /* last powercap_set per domain */
    int pkg_set_calls, dram_set_calls, pkg_reset_calls, dram_reset_calls;
    state_t pkg_set_ret, dram_set_ret;
    uint npstates;
    uint cur_idx[FD_MAX * 2];  /* current P-state index per CPU (requested, cpufreq driver) */
    int eff_set;               /* effective frequency (APERF/MPERF) differs from cur_idx */
    uint eff_idx[FD_MAX * 2];  /* effective P-state index per CPU when eff_set */
    ulong target_khz;          /* pmgt_get_app_req_freq(DOMAIN_CPU), all CPUs */
    double pack_power_w[FD_MAX]; /* PKG power per socket (W) */
    double dram_power_w[FD_MAX]; /* DRAM power per socket (W) */
    /* Optional closed loop: when w_per_pstate > 0, PKG power per socket is
     * p0_power_w - w_per_pstate * (average P-state index of its CPUs) */
    double p0_power_w, w_per_pstate;
    int freq_set_calls;
    int governor_calls;
    uint32_t pkg_min, pkg_max, dram_min, dram_max; /* RAPL POWER_INFO range (0 = unknown) */
    int dram_not_capable;                          /* no DRAM power limit (AMD) */
} fake_cpus_t;
extern fake_cpus_t fcpu;
void fake_cpus_default(void);
ulong fake_cpu_pstate_khz(uint idx); /* 3.0 GHz - 100 MHz * idx */

/* ------------------------------------ callbacks into powercap.c */
extern uint32_t fake_stored[NUM_DOMAINS][FD_MAX];
extern int fake_stored_calls;

#endif
