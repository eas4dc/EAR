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
 * test_dvfs_plugin.c -- unit tests for src/daemon/powercap/dvfs.c (dvfs.so)
 *
 * dvfs.so is EARD's DEFAULT CPU powercap plugin (DEFAULT_PC_PLUGIN_NAME_CPU).
 * It does not program RAPL: a monitor thread reads PKG+DRAM energy every
 * 500 ms and moves the CPU frequency one or two P-states at a time to keep
 * the measured power under the domain limit, never going faster than what
 * the application asked for.
 *
 * The control loop runs closed: the fake PKG power follows the P-state the
 * plugin sets (p0_power_w - w_per_pstate * P-state, per socket), and the fake
 * clock advances exactly 500 ms per tick.
 *
 * Rig: 2 sockets, 16 CPUs, 21 P-states 3.0 GHz (idx 0) .. 1.0 GHz (idx 20),
 * PKG 150 W per socket at P0 minus 5 W per P-state.
 */
#include <pthread.h>

#include <common/config.h>
#include <common/system/monitor.h>
#include <daemon/powercap/powercap_status_conf.h>

#include "fake_dev_api.h"
#include "fake_monitor.h"
#include "pc_test.h"

/* dvfs.so exports */
state_t enable(suscription_t *sus);
state_t set_powercap_value(uint pid, uint domain, uint *limit, uint *cpu_util);
state_t get_powercap_value(uint pid, uint32_t *powercap);
uint get_powercap_status(domain_status_t *status);
uint get_powercap_strategy();
void set_pc_mode(uint mode);
void plugin_get_settings(domain_settings_t *s);
state_t dvfs_pc_thread_main(void *p);
uint compute_extra_power(uint current_power, uint max_steps, uint *current, uint *target);

/* powercap_mgt.c global dvfs.c reads when restoring frequencies */
ulong pmgt_idle_def_freq = 2400000;

static suscription_t *sus;

static void dvfs_setup(void)
{
    fake_cpus_default();
    fcpu.p0_power_w   = 150;
    fcpu.w_per_pstate = 5;
    fcpu.target_khz   = fake_cpu_pstate_khz(0);
    for (int i = 0; i < FD_MAX * 2; i++)
        fcpu.cur_idx[i] = 0;
    fake_clock_ns = 1000000000LL;
    sus           = suscription();
    enable(sus);
    set_pc_mode(PC_MODE_TARGET); /* what pmgt_init does via pmgt_set_pc_mode */
    sus->call_init(NULL);
}

static void set_limit(uint32_t w)
{
    uint lim = w;
    set_powercap_value(0, LEVEL_NODE, &lim, NULL);
}

static void tick(void)
{
    fake_clock_ns += 500000000LL;
    dvfs_pc_thread_main(NULL);
}

static double node_cpu_power(void)
{
    double p = 0;
    int per  = fcpu.cpus / fcpu.sockets;
    for (int s = 0; s < fcpu.sockets; s++) {
        double idx = 0;
        for (int c = s * per; c < (s + 1) * per; c++)
            idx += fcpu.cur_idx[c];
        p += fcpu.p0_power_w - fcpu.w_per_pstate * idx / per + fcpu.dram_power_w[s];
    }
    return p;
}

/* ============================================================ basics */

TEST(test_identity)
{
    dvfs_setup();
    CHECK_EQ(get_powercap_strategy(), PC_DVFS, "strategy is DVFS");
    domain_settings_t s;
    plugin_get_settings(&s);
    CHECK(s.node_ratio > 0.09 && s.node_ratio < 0.11, "node_ratio 0.1");
    CHECK(s.security_range > 0.049 && s.security_range < 0.051, "security_range 0.05");
    CHECK(fcpu.governor_calls >= 1, "userspace governor set at enable");
    CHECK(sus->time_burst == 500 && sus->time_relax == 500, "monitor period 500 ms");
}

TEST(test_no_limit_no_action)
{
    dvfs_setup();
    for (int i = 0; i < 5; i++)
        tick();
    CHECK_EQ(fcpu.freq_set_calls, 0, "no limit (0): frequencies untouched");
    set_limit(POWER_CAP_UNLIMITED);
    int n = fcpu.freq_set_calls;
    for (int i = 0; i < 5; i++)
        tick();
    CHECK_EQ(fcpu.freq_set_calls, n, "unlimited: frequencies untouched by the loop");
}

/* ======================================================= control loop */

TEST(test_converges_under_limit)
{
    dvfs_setup();
    set_limit(200); /* 300 W at P0 */
    int t = 0;
    while (node_cpu_power() > 200 && t < 40) {
        tick();
        t++;
    }
    CHECK(node_cpu_power() <= 200, "power brought under 200 W (%.0f W at P%u)", node_cpu_power(), fcpu.cur_idx[0]);
    CHECK(t <= 8, "within 8 periods (4 s): took %d", t);
    uint idx = fcpu.cur_idx[0];
    int moves = 0;
    for (int i = 0; i < 20; i++) {
        uint before = fcpu.cur_idx[0];
        tick();
        moves += (fcpu.cur_idx[0] != before);
    }
    CHECK(node_cpu_power() <= 200, "stays under the limit for 10 s (%.0f W)", node_cpu_power());
    CHECK(moves <= 2, "and does not oscillate (%d moves around P%u)", moves, idx);
    CHECK(node_cpu_power() >= 190, "without wasting more than one P-state of headroom (%.0f W)", node_cpu_power());
}

TEST(test_includes_dram)
{
    dvfs_setup();
    fcpu.dram_power_w[0] = fcpu.dram_power_w[1] = 20;
    set_limit(240); /* PKG 300 + DRAM 40 at P0 */
    for (int i = 0; i < 30; i++)
        tick();
    CHECK(node_cpu_power() <= 240, "PKG + DRAM kept under 240 W (%.0f W)", node_cpu_power());
}

TEST(test_recovers_when_limit_rises)
{
    dvfs_setup();
    set_limit(200);
    for (int i = 0; i < 20; i++)
        tick();
    uint low = fcpu.cur_idx[0];
    set_limit(400);
    for (int i = 0; i < 40; i++)
        tick();
    CHECK(fcpu.cur_idx[0] < low, "limit raised: frequency climbs back (P%u -> P%u)", low, fcpu.cur_idx[0]);
    CHECK_EQ(fcpu.cur_idx[0], 0, "... all the way to the requested 3.0 GHz");
}

TEST(test_never_faster_than_requested)
{
    dvfs_setup();
    fcpu.target_khz = fake_cpu_pstate_khz(5); /* app asks 2.5 GHz */
    set_limit(400);
    tick();
    CHECK_EQ(fcpu.cur_idx[0], 5, "below the limit, running faster than requested: set to the request");
    for (int i = 0; i < 10; i++)
        tick();
    CHECK_EQ(fcpu.cur_idx[0], 5, "and stays there");
}

TEST(test_unlimited_restores_frequency)
{
    dvfs_setup();
    set_limit(200);
    for (int i = 0; i < 20; i++)
        tick();
    fcpu.target_khz = 0; /* no application: idle default */
    set_limit(POWER_CAP_UNLIMITED);
    CHECK_EQ(fcpu.cur_idx[0], 6, "unlimited with no app: CPUs back to the idle default 2.4 GHz (P%u)", fcpu.cur_idx[0]);
}

TEST(test_stale_energy_after_unlimited)
{
    dvfs_setup();
    set_limit(320);
    for (int i = 0; i < 4; i++)
        tick();
    CHECK_EQ(fcpu.cur_idx[0], 0, "300 W under a 320 W limit: P0");
    set_limit(POWER_CAP_UNLIMITED);
    for (int i = 0; i < 20; i++) /* 10 s uncapped */
        tick();
    set_limit(320);
    tick();
    /* The unlimited ticks update last_dvfs_time but not the energy baseline:
     * the first capped tick divides 10.5 s of energy by 0.5 s */
    CHECK(fcpu.cur_idx[0] == 0, "first capped period after 10 s unlimited does not throttle (P%u)",
          fcpu.cur_idx[0]);
}

/* ============================================================ status */

TEST(test_status)
{
    dvfs_setup();
    domain_status_t st;
    fcpu.target_khz = 0;
    set_limit(400);
    tick();
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_OK, "no requested frequency: OK, nothing to decide");

    fcpu.target_khz = fake_cpu_pstate_khz(0);
    set_limit(400);
    tick();
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_RELEASE, "300 W of 400 W at target: RELEASE");
    CHECK_EQ(st.exceed, 50, "releases half of the 100 W headroom");
    CHECK_EQ(st.current_pc, 400, "current_pc");

    set_limit(200);
    for (int i = 0; i < 20; i++)
        tick();
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_GREEDY, "throttled below target: GREEDY");
    CHECK(st.requested > 0 && st.requested <= 140, "requests a plausible amount of watts (%u)", st.requested);
    CHECK_EQ(st.stress, 50, "stress = (3.0 - 2.0) / (3.0 - 1.0) = 50 at P10 (P%u)", fcpu.cur_idx[0]);
}

TEST(test_status_over_limit_at_target)
{
    dvfs_setup();
    fcpu.target_khz = fake_cpu_pstate_khz(12);
    set_limit(400);
    tick(); /* at P12: 2 x (150 - 60) = 180 W */
    set_limit(150);
    domain_status_t st;
    get_powercap_status(&st); /* before the loop reacts */
    CHECK(st.ok != PC_STATUS_RELEASE, "30 W over the limit at target is not a RELEASE (ok=%u exceed=%u)", st.ok,
          st.exceed);
}

TEST(test_extra_power_estimate)
{
    dvfs_setup();
    uint cur[MAX_CPUS_SUPPORTED] = {0}, tgt[MAX_CPUS_SUPPORTED] = {0};
    for (int i = 0; i < 16; i++) {
        cur[i] = 5;
        tgt[i] = 2;
    }
    CHECK_EQ(compute_extra_power(200, 21, cur, tgt), 24, "3 P-states (5 -> 2) on every CPU: 3 x 8 W");
    for (int i = 0; i < 16; i++) {
        cur[i] = 2;
        tgt[i] = 0;
    }
    CHECK_EQ(compute_extra_power(200, 21, cur, tgt), 60, "P2 -> P0: 2 x 30 W (turbo steps)");
    CHECK_EQ(compute_extra_power(200, 21, tgt, tgt), 0, "no change: 0");
}

/* ======================================================== per device */

TEST(test_per_device_limits)
{
    dvfs_setup();
    /* pmgt hands one value per socket; the array is longer here only so the
     * over-read is visible instead of undefined */
    uint lim[MAX_CPUS_SUPPORTED];
    for (int i = 0; i < MAX_CPUS_SUPPORTED; i++)
        lim[i] = 1000;
    lim[0] = 150;
    lim[1] = 120;
    set_powercap_value(0, LEVEL_DEVICE, lim, NULL);
    uint32_t v[2] = {0, 0xdead};
    get_powercap_value(0, v);
    CHECK(v[0] == 150 && v[1] == 120, "per-socket 150 + 120 W kept per socket, a 270 W domain (got %u + %u)", v[0],
          v[1]);
}

TEST(test_device_storage)
{
    dvfs_setup();
    set_limit(300);
    CHECK(fake_stored_calls > 0, "dvfs records its caps in the per-device storage (%d calls)",
          fake_stored_calls);
    uint32_t v[2] = {0, 0xdead};
    get_powercap_value(0, v);
    CHECK(v[1] != 0xdead, "get_powercap_value answers per socket, as pmgt reads it (v[1] untouched)");
}

int main(void)
{
    RUN_ISOLATED(test_identity);
    RUN_ISOLATED(test_no_limit_no_action);
    RUN_ISOLATED(test_converges_under_limit);
    RUN_ISOLATED(test_includes_dram);
    RUN_ISOLATED(test_recovers_when_limit_rises);
    RUN_ISOLATED(test_never_faster_than_requested);
    RUN_ISOLATED(test_unlimited_restores_frequency);
    RUN_ISOLATED(test_stale_energy_after_unlimited);
    RUN_ISOLATED(test_status);
    RUN_ISOLATED(test_status_over_limit_at_target);
    RUN_ISOLATED(test_extra_power_estimate);
    RUN_ISOLATED(test_per_device_limits);
    RUN_ISOLATED(test_device_storage);
    return pc_summary("test_dvfs_plugin");
}
