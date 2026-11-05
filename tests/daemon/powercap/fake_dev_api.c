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
 * fake_dev_api.c -- see fake_dev_api.h
 *
 * Built twice: with -DFAKE_GPU_SIDE for test_gpu_plugin and -DFAKE_CPU_SIDE
 * for test_cpu_plugin (each plugin binary only gets the half it links to).
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include <common/config.h>
#include <common/hardware/topology.h>
#include <common/states.h>

#include "fake_dev_api.h"

fake_gpus_t fgpu;
fake_cpus_t fcpu;
uint32_t fake_stored[NUM_DOMAINS][FD_MAX];
int fake_stored_calls;

/* ------------------------------------ callbacks into powercap.c/_mgt.c */

state_t powercap_set_stored_device_value(uint domain, uint device_id, uint32_t v)
{
    if (domain >= NUM_DOMAINS || device_id >= FD_MAX)
        return EAR_ERROR;
    fake_stored[domain][device_id] = v;
    fake_stored_calls++;
    return EAR_SUCCESS;
}

void powercap_update_all_device_storage(uint domain, uint32_t v)
{
    for (int i = 0; i < FD_MAX; i++)
        fake_stored[domain][i] = v;
}

/* Same contract as powercap_mgt.c: f holds dom_size entries */
void pmgt_get_app_req_freq(uint domain, ulong *f, uint dom_size)
{
    if (domain == DOMAIN_CPU) {
        for (int i = 0; i < MAX_CPUS_SUPPORTED && i < dom_size; i++)
            f[i] = fcpu.target_khz;
    } else if (domain == DOMAIN_GPU) {
        for (int i = 0; i < MAX_GPUS_SUPPORTED && i < dom_size; i++)
            f[i] = i < FD_MAX ? fgpu.target_khz[i] : 0;
    }
}

#ifdef FAKE_GPU_SIDE
#include <management/gpu/gpu.h>
#include <metrics/gpu/gpu.h>

void fake_gpus_default(uint n)
{
    memset(&fgpu, 0, sizeof(fgpu));
    memset(fake_stored, 0, sizeof(fake_stored));
    fake_stored_calls = 0;
    fgpu.n            = n;
    for (int i = 0; i < FD_MAX; i++) {
        fgpu.min_w[i] = 100;
        fgpu.max_w[i] = 400;
    }
    ulong f[8] = {1410000, 1245000, 1080000, 915000, 750000, 585000, 420000, 210000};
    memcpy(fgpu.freqs, f, sizeof(f));
    fgpu.nfreqs = 8;
    fgpu.init_ret = EAR_SUCCESS;
    for (int i = 0; i < FD_MAX; i++)
        fgpu.clock_khz[i] = fgpu.freqs[0];
}

void fake_gpus_freqs(uint n, ulong max_khz, ulong min_khz)
{
    if (n > 128)
        n = 128;
    for (uint i = 0; i < n; i++)
        fgpu.freqs[i] = max_khz - (ulong) ((double) (max_khz - min_khz) * i / (n - 1));
    fgpu.nfreqs = n;
    for (int i = 0; i < FD_MAX; i++)
        fgpu.clock_khz[i] = fgpu.freqs[0];
}

void fake_gpus_model(void)
{
    if (fgpu.w_per_mhz <= 0)
        return;
    for (uint i = 0; i < fgpu.n; i++) {
        fgpu.reading[i].freq_gpu = fgpu.clock_khz[i];
        fgpu.reading[i].power_w  = fgpu.idle_w;
        if (fgpu.reading[i].util_gpu > 0)
            fgpu.reading[i].power_w += fgpu.w_per_mhz * (double) fgpu.clock_khz[i] / 1000.0;
    }
}

state_t mgt_gpu_freq_limit_get_current(ctx_t *c, ulong *f)
{
    for (uint i = 0; i < fgpu.n; i++)
        f[i] = fgpu.clock_khz[i];
    return EAR_SUCCESS;
}

state_t mgt_gpu_freq_limit_set(ctx_t *c, ulong *f)
{
    fgpu.clock_set_calls++;
    for (uint i = 0; i < fgpu.n; i++)
        fgpu.clock_khz[i] = f[i];
    fake_gpus_model();
    return EAR_SUCCESS;
}

void mgt_gpu_load(int force_api)
{
}
state_t mgt_gpu_init(ctx_t *c)
{
    return fgpu.init_ret;
}
state_t mgt_gpu_count_devices(ctx_t *c, uint *n)
{
    *n = fgpu.n;
    return EAR_SUCCESS;
}
state_t mgt_gpu_power_data_alloc(uint32_t **d)
{
    *d = calloc(FD_MAX, sizeof(uint32_t));
    return EAR_SUCCESS;
}
state_t mgt_gpu_freq_data_alloc(ulong **d)
{
    *d = calloc(FD_MAX, sizeof(ulong));
    return EAR_SUCCESS;
}
state_t mgt_gpu_power_cap_get_rank(ctx_t *c, uint32_t *wmin, uint32_t *wmax)
{
    for (uint i = 0; i < fgpu.n; i++) {
        wmin[i] = fgpu.min_w[i];
        wmax[i] = fgpu.max_w[i];
    }
    return EAR_SUCCESS;
}
state_t mgt_gpu_power_cap_set(ctx_t *c, uint32_t *w)
{
    fgpu.set_calls++;
    for (uint i = 0; i < fgpu.n; i++)
        fgpu.hw_cap[i] = w[i];
    return EAR_SUCCESS;
}
state_t mgt_gpu_power_cap_get_current(ctx_t *c, uint32_t *w)
{
    for (uint i = 0; i < fgpu.n; i++)
        w[i] = fgpu.hw_cap[i];
    return EAR_SUCCESS;
}

static const ulong *freq_rows[FD_MAX];
static uint freq_lens[FD_MAX];
state_t mgt_gpu_freq_get_available(ctx_t *c, const ulong ***list, const uint **len)
{
    for (int i = 0; i < FD_MAX; i++) {
        freq_rows[i] = fgpu.freqs;
        freq_lens[i] = fgpu.nfreqs;
    }
    *list = freq_rows;
    *len  = freq_lens;
    return EAR_SUCCESS;
}

void gpu_load(int options)
{
}
void gpu_data_alloc(gpu_t **d)
{
    *d = calloc(FD_MAX, sizeof(gpu_t));
}
state_t gpu_read(gpu_t *d)
{
    fake_gpus_model();
    memcpy(d, fgpu.reading, sizeof(gpu_t) * FD_MAX);
    return EAR_SUCCESS;
}
state_t gpu_read_raw(gpu_t *d)
{
    return gpu_read(d);
}
/* The real diff averages two samples; the fake reading already is the average */
void gpu_data_diff(gpu_t *d2, gpu_t *d1, gpu_t *dd)
{
    memcpy(dd, d2, sizeof(gpu_t) * FD_MAX);
}
void gpu_data_copy(gpu_t *dst, gpu_t *src)
{
    memcpy(dst, src, sizeof(gpu_t) * FD_MAX);
}
#endif /* FAKE_GPU_SIDE */

#ifdef FAKE_CPU_SIDE
#include <management/cpufreq/cpufreq.h>
#include <management/cpupow/cpupow.h>
#include <metrics/energy_cpu/energy_cpu.h>

#include "fake_monitor.h"

ulong fake_cpu_pstate_khz(uint idx)
{
    return 3000000 - 100000 * idx;
}

void fake_cpus_default(void)
{
    memset(&fcpu, 0, sizeof(fcpu));
    memset(fake_stored, 0, sizeof(fake_stored));
    fake_stored_calls = 0;
    fcpu.sockets      = 2;
    fcpu.cpus         = 16;
    for (int i = 0; i < FD_MAX; i++) {
        fcpu.pkg_tdp[i]  = 200;
        fcpu.dram_tdp[i] = 40;
    }
    fcpu.npstates    = 21;
    fcpu.pkg_set_ret = fcpu.dram_set_ret = EAR_SUCCESS;
    fcpu.target_khz  = fake_cpu_pstate_khz(5); /* 2.5 GHz */
    for (int i = 0; i < FD_MAX * 2; i++)
        fcpu.cur_idx[i] = 5;
    for (int i = 0; i < FD_MAX; i++)
        fcpu.pack_power_w[i] = 100;
}

static double pkg_power(int s)
{
    if (fcpu.w_per_pstate <= 0)
        return fcpu.pack_power_w[s];
    int per = fcpu.cpus / fcpu.sockets;
    double idx = 0;
    for (int c = s * per; c < (s + 1) * per; c++)
        idx += fcpu.cur_idx[c];
    idx /= per;
    return fcpu.p0_power_w - fcpu.w_per_pstate * idx;
}

state_t topology_init(topology_t *t)
{
    memset(t, 0, sizeof(*t));
    t->socket_count = fcpu.sockets;
    t->cpu_count    = fcpu.cpus;
    t->core_count   = fcpu.cpus;
    return EAR_SUCCESS;
}

/* energy_cpu: NUM_PACKS (2) counters per socket, laid out as EAR does:
 * [0..S) DRAM, [S..2S) PKG, in nJ as energy_cpu_compute_power expects */
static ullong energy_nj[FD_MAX * 2];
static long long last_read_ns = 0;

state_t energy_cpu_load(topology_t *tp, uint eard)
{
    return EAR_SUCCESS;
}
state_t energy_cpu_init(ctx_t *c)
{
    return EAR_SUCCESS;
}
state_t energy_cpu_data_alloc(ctx_t *c, ullong **v, uint *count)
{
    *v = calloc(FD_MAX * 2, sizeof(ullong));
    if (count)
        *count = fcpu.sockets * 2;
    return EAR_SUCCESS;
}
static void energy_advance(void)
{
    long long now = fake_clock_ns;
    double dt     = (double) (now - last_read_ns) / 1e9;
    if (last_read_ns == 0)
        dt = 0;
    for (int i = 0; i < fcpu.sockets; i++) {
        energy_nj[i] += (ullong) (fcpu.dram_power_w[i] * dt * 1e9);
        energy_nj[fcpu.sockets + i] += (ullong) (pkg_power(i) * dt * 1e9);
    }
    last_read_ns = now;
}
state_t energy_cpu_read(ctx_t *c, ullong *v)
{
    energy_advance();
    memcpy(v, energy_nj, sizeof(ullong) * fcpu.sockets * 2);
    return EAR_SUCCESS;
}
state_t energy_cpu_read_copy(ctx_t *c, ullong *v2, ullong *v1, ullong *vD)
{
    energy_advance();
    for (int i = 0; i < fcpu.sockets * 2; i++) {
        v2[i] = energy_nj[i];
        vD[i] = v2[i] - v1[i];
        v1[i] = v2[i];
    }
    return EAR_SUCCESS;
}
state_t energy_cpu_data_diff(ctx_t *c, ullong *start, ullong *end, ullong *result)
{
    for (int i = 0; i < fcpu.sockets * 2; i++)
        result[i] = end[i] - start[i];
    return EAR_SUCCESS;
}
state_t energy_cpu_data_copy(ctx_t *c, ullong *dst, ullong *src)
{
    memcpy(dst, src, sizeof(ullong) * fcpu.sockets * 2);
    return EAR_SUCCESS;
}
double energy_cpu_compute_power(double energy, double elapsed_time)
{
    return elapsed_time > 0 ? (energy / 1e9) / elapsed_time : 0;
}

/* management/cpufreq/frequency.h (the old API dvfs.c uses) */
state_t frequency_init(uint eard)
{
    return EAR_SUCCESS;
}
uint frequency_get_num_pstates()
{
    return fcpu.npstates;
}
uint frequency_freq_to_pstate(ulong khz)
{
    for (uint i = 0; i < fcpu.npstates; i++)
        if (fake_cpu_pstate_khz(i) <= khz)
            return i;
    return fcpu.npstates - 1;
}
ulong frequency_get_cpufreq_list(uint cpus, ulong *f)
{
    for (uint i = 0; i < cpus && i < FD_MAX * 2; i++)
        f[i] = fake_cpu_pstate_khz(fcpu.cur_idx[i]);
    return f[0];
}
ulong frequency_set_with_list(uint cpus, ulong *f)
{
    fcpu.freq_set_calls++;
    for (uint i = 0; i < cpus && i < FD_MAX * 2; i++)
        fcpu.cur_idx[i] = frequency_freq_to_pstate(f[i]);
    return f[0];
}
state_t frequency_set_userspace_governor_all_cpus()
{
    fcpu.governor_calls++;
    return EAR_SUCCESS;
}
void verbose_frequencies(int cpus, ulong *f)
{
}
ulong *frequency_get_freq_rank_list()
{
    static ulong l[64];
    for (uint i = 0; i < fcpu.npstates && i < 64; i++)
        l[i] = fake_cpu_pstate_khz(i);
    return l;
}

void mgt_cpupow_load(topology_t *tp, int eard)
{
}
int mgt_cpupow_count_devices(int domain)
{
    return fcpu.sockets;
}
state_t mgt_cpupow_limits_get(int domain, uint32_t *wmin, uint32_t *wmax)
{
    for (int i = 0; i < fcpu.sockets; i++) {
        wmin[i] = (domain == CPUPOW_SOCKET) ? fcpu.pkg_min : fcpu.dram_min;
        wmax[i] = (domain == CPUPOW_SOCKET) ? fcpu.pkg_max : fcpu.dram_max;
    }
    return EAR_SUCCESS;
}
state_t mgt_cpupow_tdp_get(int domain, uint32_t *w)
{
    for (int i = 0; i < fcpu.sockets; i++)
        w[i] = domain == CPUPOW_SOCKET ? fcpu.pkg_tdp[i] : fcpu.dram_tdp[i];
    return EAR_SUCCESS;
}

int mgt_cpupow_powercap_is_capable(int domain)
{
    return !(domain == CPUPOW_DRAM && fcpu.dram_not_capable);
}
state_t mgt_cpupow_powercap_set(int domain, uint32_t *w)
{
    if (domain == CPUPOW_SOCKET) {
        fcpu.pkg_set_calls++;
        if (fcpu.pkg_set_ret != EAR_SUCCESS)
            return fcpu.pkg_set_ret;
        memcpy(fcpu.hw_pkg, w, sizeof(uint32_t) * fcpu.sockets);
    } else {
        fcpu.dram_set_calls++;
        if (fcpu.dram_not_capable)
            return EAR_ERROR;
        if (fcpu.dram_set_ret != EAR_SUCCESS)
            return fcpu.dram_set_ret;
        memcpy(fcpu.hw_dram, w, sizeof(uint32_t) * fcpu.sockets);
    }
    return EAR_SUCCESS;
}
state_t mgt_cpupow_powercap_get(int domain, uint32_t *w)
{
    memcpy(w, domain == CPUPOW_SOCKET ? fcpu.hw_pkg : fcpu.hw_dram, sizeof(uint32_t) * fcpu.sockets);
    return EAR_SUCCESS;
}
state_t mgt_cpupow_powercap_reset(int domain, int mode)
{
    uint32_t *hw = domain == CPUPOW_SOCKET ? fcpu.hw_pkg : fcpu.hw_dram;
    for (int i = 0; i < fcpu.sockets; i++)
        hw[i] = POWERCAP_DISABLE;
    if (domain == CPUPOW_SOCKET)
        fcpu.pkg_reset_calls++;
    else
        fcpu.dram_reset_calls++;
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_load(topology_t *tp, int eard)
{
    return EAR_SUCCESS;
}
state_t mgt_cpufreq_init(ctx_t *c)
{
    return EAR_SUCCESS;
}
/* Exactly sized, so an over-read of the list is visible under ASan */
void mgt_cpufreq_data_alloc(pstate_t **list, uint **idx)
{
    if (list)
        *list = calloc(fcpu.npstates > (uint) fcpu.cpus ? fcpu.npstates : (uint) fcpu.cpus, sizeof(pstate_t));
    if (idx)
        *idx = calloc(fcpu.cpus, sizeof(uint));
}
static pstate_t *available = NULL;
state_t mgt_cpufreq_get_available_list(ctx_t *c, const pstate_t **list, uint *count)
{
    if (available == NULL) {
        available = calloc(fcpu.npstates, sizeof(pstate_t));
        for (uint i = 0; i < fcpu.npstates; i++) {
            available[i].idx = i;
            available[i].khz = fake_cpu_pstate_khz(i);
        }
    }
    /* cpu_generic.c passes a pointer to its own freshly allocated pointer */
    *list  = available;
    *count = fcpu.npstates;
    return EAR_SUCCESS;
}
state_t mgt_cpufreq_get_current_list(ctx_t *c, pstate_t *list)
{
    for (int i = 0; i < fcpu.cpus; i++) {
        list[i].idx = fcpu.cur_idx[i];
        list[i].khz = fake_cpu_pstate_khz(fcpu.cur_idx[i]);
    }
    return EAR_SUCCESS;
}
state_t mgt_cpufreq_get_index(ctx_t *c, ullong khz, uint *idx, uint closest)
{
    for (uint i = 0; i < fcpu.npstates; i++) {
        if (fake_cpu_pstate_khz(i) <= khz) {
            *idx = i;
            return EAR_SUCCESS;
        }
    }
    *idx = fcpu.npstates - 1;
    return EAR_SUCCESS;
}

ulong frequency_pstate_to_freq(uint i)
{
    return fake_cpu_pstate_khz(i);
}

/* metrics/cpufreq: effective frequency (APERF/MPERF), per CPU */
#include <metrics/cpufreq/cpufreq.h>
void cpufreq_load(topology_t *tp, int options)
{
}
void cpufreq_data_alloc(cpufreq_t **f, ulong **freqs)
{
    if (f)
        *f = calloc(fcpu.cpus, sizeof(cpufreq_t));
    if (freqs)
        *freqs = calloc(fcpu.cpus, sizeof(ulong));
}
state_t cpufreq_read(cpufreq_t *ef)
{
    return EAR_SUCCESS;
}
state_t cpufreq_read_copy(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average)
{
    ulong sum = 0;
    for (int i = 0; i < fcpu.cpus; i++) {
        freqs[i] = fake_cpu_pstate_khz(fcpu.eff_set ? fcpu.eff_idx[i] : fcpu.cur_idx[i]);
        sum += freqs[i];
    }
    if (average)
        *average = fcpu.cpus ? sum / fcpu.cpus : 0;
    return EAR_SUCCESS;
}
#endif /* FAKE_CPU_SIDE */
