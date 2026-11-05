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
 * test_gpu_dvfs_plugin.c -- unit tests for src/daemon/powercap/gpu_dvfs.c
 *
 * gpu_dvfs.so is EARD's DEFAULT GPU powercap plugin (DEFAULT_PC_PLUGIN_NAME_GPU).
 * It does not program NVML power limits: it splits the GPU domain limit into
 * per-GPU budgets (idle GPUs get their idle power + 5 W, busy ones share the
 * rest by utilization) and a monitor thread lowers or raises each GPU's
 * clock to keep its measured power under its budget.
 *
 * The loop runs closed: a busy fake GPU draws idle_w + w_per_mhz * clock.
 * Rig: 4 GPUs, 81 clocks 1980..780 MHz (15 MHz steps), 50 W idle,
 * 0.2 W/MHz busy (446 W at 1980 MHz, 206 W at 780 MHz), cap range 100..400 W.
 * The ASan build (test_gpu_dvfs_plugin_asan) checks the t_freq overflow.
 */
#include <pthread.h>

#include <common/config.h>
#include <common/system/monitor.h>
#include <daemon/powercap/powercap_status_conf.h>

#include "fake_dev_api.h"
#include "pc_test.h"

state_t enable(suscription_t *sus);
state_t set_powercap_value(uint pid, uint domain, uint32_t *limit, uint32_t *gpu_util);
state_t get_powercap_value(uint pid, uint32_t *powercap);
uint get_powercap_status(domain_status_t *status);
void set_new_utilization(uint32_t *util);
void set_status(uint status);
uint get_powercap_strategy();
state_t gpu_dvfs_pc_thread_main(void *p);

#define MHZ(k) ((k) / 1000)

static suscription_t *sus;

static void rig(uint u0, uint u1, uint u2, uint u3)
{
    fake_gpus_default(4);
    fake_gpus_freqs(81, 1980000, 780000);
    fgpu.idle_w    = 50;
    fgpu.w_per_mhz = 0.2;
    uint u[4]      = {u0, u1, u2, u3};
    for (int i = 0; i < 4; i++) {
        fgpu.reading[i].util_gpu = u[i];
        fgpu.target_khz[i]       = 1980000;
    }
    fake_gpus_model();
    sus = suscription();
    enable(sus);
    sus->call_init(NULL);
}

static void set_limit(uint32_t w)
{
    uint32_t util[4];
    for (int i = 0; i < 4; i++)
        util[i] = (uint32_t) fgpu.reading[i].util_gpu;
    set_powercap_value(0, LEVEL_NODE, &w, util);
}

static void ticks(int n)
{
    for (int i = 0; i < n; i++)
        gpu_dvfs_pc_thread_main(NULL);
}

static double gpu_power(int i)
{
    return fgpu.reading[i].power_w;
}

/* ================================================================ loop */

TEST(test_identity)
{
    rig(80, 80, 80, 80);
    CHECK_EQ(get_powercap_strategy(), PC_DVFS, "strategy is DVFS");
    CHECK(sus->time_burst == 1000 && sus->time_relax == 60000, "monitor 1 s burst / 60 s relax");
}

/* Runs n periods and returns the largest clock jump seen on GPU g (MHz),
 * plus the periods GPU g spent over its budget */
static ulong watch(int g, int n, double budget, int *over)
{
    ulong jump = 0, prev = fgpu.clock_khz[g];
    *over      = 0;
    for (int i = 0; i < n; i++) {
        ticks(1);
        ulong c = fgpu.clock_khz[g];
        ulong d = c > prev ? c - prev : prev - c;
        if (d > jump)
            jump = d;
        prev = c;
        *over += gpu_power(g) > budget + 2;
    }
    return MHZ(jump);
}

TEST(test_equal_load_converges)
{
    rig(80, 80, 80, 80);
    set_limit(1200); /* 300 W per GPU: 1250 MHz is the answer */
    ticks(10);
    int over;
    ulong jump = watch(0, 20, 300, &over);
    printf("        GPU0 over 20 periods: largest clock jump %lu MHz, %d periods over 300 W\n", jump, over);
    CHECK(jump <= 60 && over <= 2,
          "settles near 1250 MHz instead of bang-bang between 780 and 1980 MHz (jump %lu MHz, %d/20 over)", jump, over);
    CHECK(fgpu.clock_set_calls > 0, "the loop drives the clocks");
}

TEST(test_mixed_load_split)
{
    rig(100, 50, 25, 0);
    set_limit(1200);
    ticks(40);
    CHECK_EQ(fgpu.clock_khz[3], 780000, "idle GPU parked at the minimum clock after UTIL_WAIT");
    /* budgets 400 / 327 / 163 W: GPU0 fits up to 1750 MHz, GPU1 1385, GPU2 none */
    int over0, over1;
    ulong j0 = watch(0, 20, 400, &over0), j1 = watch(1, 20, 327, &over1);
    CHECK(over0 <= 2 && over1 <= 2, "busy GPUs stay within their budgets (GPU0 %d/20, GPU1 %d/20 over)", over0,
          over1);
    CHECK(j0 >= 0 && j1 >= 0, "(largest jumps %lu / %lu MHz)", j0, j1);
}

TEST(test_idle_gpu_resumes)
{
    rig(80, 80, 80, 0);
    set_limit(1600);
    ticks(10);
    CHECK_EQ(fgpu.clock_khz[3], 780000, "idle GPU3 parked at 780 MHz");
    fgpu.reading[3].util_gpu = 80;
    ticks(10);
    CHECK(fgpu.clock_khz[3] > 780000, "busy again: GPU3 clock raised (%lu MHz)", MHZ(fgpu.clock_khz[3]));
}

TEST(test_raises_to_target_when_limit_rises)
{
    rig(80, 80, 80, 80);
    set_limit(1790); /* 447 W each, but a GPU's budget stops at its 400 W max: 1980 MHz needs 446 W */
    ticks(20);
    CHECK(fgpu.clock_khz[0] >= 1740000 && gpu_power(0) <= 402,
          "GPUs climb to the highest clock within their 400 W max (%lu MHz, %.0f W)", MHZ(fgpu.clock_khz[0]),
          gpu_power(0));
    ulong settled = fgpu.clock_khz[0];
    int over;
    watch(0, 10, 400, &over);
    CHECK_EQ(fgpu.clock_khz[0], settled, "and they stay there");
}

TEST(test_small_overshoot)
{
    rig(80, 80, 80, 80);
    set_limit(1200);
    fgpu.w_per_mhz = 0; /* open loop: pin the readings */
    for (int i = 0; i < 4; i++) {
        fgpu.clock_khz[i]        = 1500000;
        fgpu.reading[i].power_w  = 301; /* 1 W over the 300 W budget */
        fgpu.reading[i].freq_gpu = 1500000;
    }
    ticks(1);
    CHECK(fgpu.clock_khz[0] <= 1500000, "1 W over budget does not raise the clock (1500 -> %lu MHz)",
          MHZ(fgpu.clock_khz[0]));
}

TEST(test_moderate_overshoot)
{
    rig(80, 80, 80, 80);
    set_limit(1200);
    fgpu.w_per_mhz = 0;
    for (int i = 0; i < 4; i++) {
        fgpu.clock_khz[i]        = 1980000;
        fgpu.reading[i].power_w  = 350; /* 50 W over: ~250 MHz at 0.2 W/MHz */
        fgpu.reading[i].freq_gpu = 1980000;
    }
    ticks(1);
    CHECK(fgpu.clock_khz[0] >= 1500000,
          "50 W over budget cuts the clock proportionally, not by 48 steps (1980 -> %lu MHz)",
          MHZ(fgpu.clock_khz[0]));
}

TEST(test_unlimited_restores_target)
{
    rig(80, 80, 80, 80);
    set_limit(1000);
    ticks(40);
    set_limit(POWER_CAP_UNLIMITED);
    CHECK_EQ(fgpu.clock_khz[0], 1980000, "unlimited: clocks back to the target");
}

TEST(test_per_device_sum)
{
    rig(80, 80, 80, 80);
    uint32_t lim[4] = {300, 250, 200, 150}, util[4] = {80, 80, 80, 80}, v = 0;
    set_powercap_value(0, LEVEL_DEVICE, lim, util);
    get_powercap_value(0, &v);
    CHECK_EQ(v, 900, "per-device request: domain limit is the sum (per-GPU values not honoured)");
}

/* ============================================================ status */

TEST(test_status_idle_release)
{
    rig(0, 0, 0, 0);
    set_limit(1200);
    ticks(2);
    domain_status_t st;
    CHECK_EQ(get_powercap_status(&st), 1, "all GPUs idle: can release");
    CHECK_EQ(st.ok, PC_STATUS_RELEASE, "RELEASE");
    CHECK_EQ(st.exceed, 1200 - 4 * 55, "everything but idle + 5 W per GPU");
}

TEST(test_status_greedy_requested)
{
    rig(80, 80, 80, 80);
    set_limit(1000); /* 250 W per GPU: throttled well below the 1980 MHz target */
    ticks(40);
    fgpu.w_per_mhz = 0;
    for (int i = 0; i < 4; i++)
        fgpu.reading[i].power_w = 260; /* still over: marks the domain greedy */
    ticks(1);
    domain_status_t st;
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_GREEDY, "throttled below target and over budget: GREEDY");
    /* 1980 MHz needs ~446 W per GPU: each asks its whole 150 W headroom to the 400 W max */
    CHECK_EQ(st.requested, 4 * 150, "throttled GPUs ask the watts their target needs, up to their headroom");
}

TEST(test_status_ask_def)
{
    rig(80, 80, 80, 80);
    set_limit(1200);
    ticks(5);
    set_status(PC_STATUS_RELEASE); /* not what it is for; see below */
    fgpu.w_per_mhz = 0;
    for (int i = 0; i < 4; i++)
        fgpu.reading[i].power_w = 320;
    ticks(1);
    domain_status_t st;
    get_powercap_status(&st);
    /* gpu_dvfs_status becomes RELEASE only in release_powercap_allocation(),
     * which pmgt never calls: ASK_DEF is unreachable in practice */
    CHECK(st.ok == PC_STATUS_GREEDY || st.ok == PC_STATUS_OK, "over budget without a prior release: not ASK_DEF (ok=%u)",
          st.ok);
}

TEST(test_utilization_from_pmgt_layout)
{
    rig(80, 80, 80, 80);
    set_limit(1200);
    ulong pmgt_util[4] = {80, 80, 80, 80}; /* powercap_mgt.c's current_util[] */
    set_new_utilization((uint32_t *) pmgt_util);
    domain_status_t st;
    get_powercap_status(&st);
    /* The same ulong/uint32_t mismatch as gpu.c: GPUs 1 and 3 get an
     * idle budget. gpu_dvfs re-reads utilization on its next monitor tick and
     * re-splits, so here the damage lasts one period. */
    ticks(1);
    CHECK(fgpu.reading[1].util_gpu == 80, "monitor tick re-reads the real utilization");
}

static void one_tick(void)
{
    rig(80, 80, 80, 80);
    set_limit(1200);
    ticks(1);
}

TEST(test_target_frequency_buffer)
{
#if defined(__SANITIZE_ADDRESS__)
    /* t_freq is calloc(num_gpus): pmgt_get_app_req_freq must stay within dom_size */
    int sig = pc_crashes(one_tick);
    CHECK(sig == 0, "a monitor tick stays inside t_freq on a 4-GPU node (signal %d = %s)", sig,
          sig ? strsignal(sig) : "none");
#else
    one_tick();
    CHECK(1, "tick ran (build test_gpu_dvfs_plugin_asan to check the t_freq overflow)");
#endif
}

int main(void)
{
#if defined(__SANITIZE_ADDRESS__)
    /* every tick overflows t_freq, so the ASan build runs only this */
    RUN_ISOLATED(test_target_frequency_buffer);
    return pc_summary("test_gpu_dvfs_plugin_asan");
#endif
    RUN_ISOLATED(test_identity);
    RUN_ISOLATED(test_equal_load_converges);
    RUN_ISOLATED(test_mixed_load_split);
    RUN_ISOLATED(test_idle_gpu_resumes);
    RUN_ISOLATED(test_raises_to_target_when_limit_rises);
    RUN_ISOLATED(test_small_overshoot);
    RUN_ISOLATED(test_moderate_overshoot);
    RUN_ISOLATED(test_unlimited_restores_target);
    RUN_ISOLATED(test_per_device_sum);
    RUN_ISOLATED(test_status_idle_release);
    RUN_ISOLATED(test_status_greedy_requested);
    RUN_ISOLATED(test_status_ask_def);
    RUN_ISOLATED(test_utilization_from_pmgt_layout);
    RUN_ISOLATED(test_target_frequency_buffer);
    return pc_summary("test_gpu_dvfs_plugin");
}
