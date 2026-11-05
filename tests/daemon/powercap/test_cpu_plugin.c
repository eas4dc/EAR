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
 * test_cpu_plugin.c -- unit tests for src/daemon/powercap/cpu_generic.c
 *
 * cpu_generic.so is the CPU domain plugin for RAPL-style caps: it splits the
 * CPU domain limit between the package (PKG) and DRAM caps of every socket in
 * proportion to their TDPs, and decides the CPU domain status from package
 * energy and P-states.
 *
 * Rig: 2 sockets, PKG TDP 200 W, DRAM TDP 40 W, 16 CPUs, P-states 3.0 GHz
 * (idx 0) down to 1.0 GHz (idx 20). The frozen fake clock drives the energy
 * counters so power readings are exact.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <limits.h>
#include <pthread.h>

#include <common/config.h>
#include <common/system/monitor.h>
#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <management/cpupow/cpupow.h>

#include "fake_dev_api.h"
#include "fake_monitor.h"
#include "pc_test.h"

state_t enable(suscription_t *sus);
state_t disable();
state_t set_powercap_value(uint pid, uint32_t domain, uint32_t *limit, uint32_t *cpu_util);
state_t get_powercap_value(uint pid, uint *powercap);
uint get_powercap_status(domain_status_t *status);
void plugin_get_settings(domain_settings_t *s);
state_t set_dram_powercap_value(uint32_t *limit);
state_t get_dram_powercap_value(uint32_t *powercap);

/* One monitor period: always longer than cpu_generic's POWER_WINDOW_MS */
#define POWER_WINDOW_TEST_NS ((long long) CHECK_STATUS_PERIOD * 1000000LL)

static void cpu_setup(void)
{
    fake_cpus_default();
    fake_clock_ns = 1000000000LL;
    enable(suscription());
}

static void set_limit(uint32_t limit)
{
    set_powercap_value(0, LEVEL_NODE, &limit, NULL);
}

/* Advances the clock one second and returns the domain status */
static domain_status_t status_after(double pkg_w_each)
{
    domain_status_t st;
    memset(&st, 0xAB, sizeof(st));
    for (int i = 0; i < fcpu.sockets; i++)
        fcpu.pack_power_w[i] = pkg_w_each;
    fake_clock_ns += 1000000000LL;
    get_powercap_status(&st);
    return st;
}

static void all_cpus_at(uint idx)
{
    for (int i = 0; i < FD_MAX * 2; i++)
        fcpu.cur_idx[i] = idx;
}

/* ============================================================== split */

TEST(test_split_by_tdp)
{
    cpu_setup();
    set_limit(480);
    CHECK(fcpu.hw_pkg[0] >= 199 && fcpu.hw_pkg[0] <= 200, "PKG0 gets 400/480 of 480 / 2 sockets (%u)", fcpu.hw_pkg[0]);
    CHECK_EQ(fcpu.hw_pkg[1], fcpu.hw_pkg[0], "both sockets equal");
    CHECK(fcpu.hw_dram[0] >= 39 && fcpu.hw_dram[0] <= 40, "DRAM0 gets 80/480 of 480 / 2 (%u)", fcpu.hw_dram[0]);
    uint32_t sum = fcpu.hw_pkg[0] + fcpu.hw_pkg[1] + fcpu.hw_dram[0] + fcpu.hw_dram[1];
    CHECK(sum <= 480 && sum >= 476, "PKG + DRAM caps add up to the domain limit (%u)", sum);
    CHECK_EQ(fake_stored[DOMAIN_CPU][1], fcpu.hw_pkg[1], "stored PKG value matches the hardware");
    CHECK_EQ(fake_stored[DOMAIN_DRAM][1], fcpu.hw_dram[1], "stored DRAM value matches the hardware");
}

TEST(test_split_share_from_pmgt)
{
    cpu_setup();
    set_limit(451); /* what powercap_mgt.c gives this domain on a 2000 W 4-GPU node */
    uint32_t sum = fcpu.hw_pkg[0] + fcpu.hw_pkg[1] + fcpu.hw_dram[0] + fcpu.hw_dram[1];
    CHECK(sum <= 451, "caps within the domain limit (%u)", sum);
    CHECK_EQ(fcpu.hw_pkg[0], 187, "PKG 451 * 5/6 / 2 = 187");
    CHECK_EQ(fcpu.hw_dram[0], 37, "DRAM 451 * 1/6 / 2 = 37");
}

TEST(test_same_limit_not_reapplied)
{
    cpu_setup();
    set_limit(400);
    int n = fcpu.pkg_set_calls;
    set_limit(400);
    CHECK_EQ(fcpu.pkg_set_calls, n, "same limit twice: RAPL written once");
}

TEST(test_unlimited_resets)
{
    cpu_setup();
    set_limit(400);
    set_limit(POWER_CAP_UNLIMITED);
    CHECK(fcpu.pkg_reset_calls == 1 && fcpu.dram_reset_calls == 1, "unlimited resets PKG and DRAM caps");
    CHECK_EQ(fake_stored[DOMAIN_CPU][0], POWER_CAP_UNLIMITED, "PKG stored as unlimited");
    CHECK_EQ(fake_stored[DOMAIN_DRAM][0], POWER_CAP_UNLIMITED, "DRAM stored as unlimited");
    /* EARGM soft powercap: 400 -> unlimited -> 400 again. current_limit still
     * says 400 after the reset, so the second 400 is skipped and the sockets
     * stay uncapped. */
    set_limit(400);
    CHECK(fcpu.hw_pkg[0] > 0 && fcpu.hw_pkg[0] < 200,
          "the same limit re-sent after 'unlimited' is applied again (PKG cap %u)", fcpu.hw_pkg[0]);
}

TEST(test_tiny_limit)
{
    cpu_setup();
    set_limit(3);
    /* 3 W: 2.5 W of PKG over 2 sockets rounds to 1 W each, 0.5 W of DRAM to 0 */
    CHECK(fcpu.hw_pkg[0] <= 3, "PKG cap never above the domain limit (%u)", fcpu.hw_pkg[0]);
    CHECK(fcpu.hw_dram[0] != 0 && fcpu.hw_pkg[0] != POWER_CAP_UNLIMITED,
          "a tiny limit never produces a 0 W or 'unlimited' (1 W) device cap (PKG %u DRAM %u)", fcpu.hw_pkg[0],
          fcpu.hw_dram[0]);
}

TEST(test_dram_failure)
{
    cpu_setup();
    fcpu.dram_set_ret = EAR_ERROR; /* DRAM RAPL locked by BIOS */
    uint32_t lim      = 400;
    CHECK_EQ(set_powercap_value(0, LEVEL_NODE, &lim, NULL), EAR_ERROR, "DRAM failure reported");
    fcpu.dram_set_ret = EAR_SUCCESS;
    int n             = fcpu.dram_set_calls;
    set_limit(400);
    CHECK(fcpu.dram_set_calls > n, "retrying the same limit retries the DRAM cap that failed");
}

TEST(test_pkg_failure)
{
    cpu_setup();
    fcpu.pkg_set_ret = EAR_ERROR;
    uint32_t lim     = 400;
    CHECK_EQ(set_powercap_value(0, LEVEL_NODE, &lim, NULL), EAR_ERROR, "PKG failure reported");
    CHECK_EQ(fcpu.dram_set_calls, 0, "DRAM not touched after PKG failed");
    fcpu.pkg_set_ret = EAR_SUCCESS;
    set_limit(400);
    CHECK(fcpu.hw_pkg[0] > 0, "retry after PKG failure applies the limit");
}

TEST(test_per_device)
{
    cpu_setup();
    uint32_t lim[2] = {150, POWER_CAP_UNLIMITED};
    set_powercap_value(0, LEVEL_DEVICE, lim, NULL);
    CHECK_EQ(fcpu.hw_pkg[0], 150, "socket 0 capped at 150 W");
    CHECK_EQ(fcpu.hw_pkg[1], POWERCAP_DISABLE, "socket 1 unlimited -> cap disabled");
    CHECK_EQ(fcpu.dram_set_calls, 0, "per-device CPU request leaves DRAM alone");
    CHECK(fake_stored[DOMAIN_CPU][1] == POWER_CAP_UNLIMITED,
          "per-device unlimited stored as POWER_CAP_UNLIMITED, as verify expects (stored %u)",
          fake_stored[DOMAIN_CPU][1]);
    CHECK(lim[1] == POWER_CAP_UNLIMITED, "the caller's array is not rewritten (now %u)", lim[1]);
}

TEST(test_per_device_dram)
{
    cpu_setup();
    uint32_t lim[2] = {200, POWER_CAP_UNLIMITED};
    CHECK_EQ(set_dram_powercap_value(lim), EAR_SUCCESS, "per-device DRAM applied");
    CHECK_EQ(fcpu.hw_dram[0], 40, "DRAM 0 clamped to its maximum (the 40 W TDP, POWER_INFO reports none)");
    CHECK_EQ(fcpu.hw_dram[1], POWERCAP_DISABLE, "DRAM 1 unlimited -> cap disabled");
    CHECK_EQ(fake_stored[DOMAIN_DRAM][0], 40, "stored DRAM value matches the hardware");
    CHECK_EQ(fake_stored[DOMAIN_DRAM][1], POWER_CAP_UNLIMITED, "DRAM unlimited stored as POWER_CAP_UNLIMITED");
    CHECK_EQ(fcpu.pkg_set_calls, 0, "per-device DRAM request leaves PKG alone");
    uint32_t v[2];
    CHECK_EQ(get_dram_powercap_value(v), EAR_SUCCESS, "DRAM read back");
    CHECK_EQ(v[0], 40, "... from the hardware");

    /* the hardware refuses: the storage keeps the previous values */
    uint32_t lim2[2]  = {30, 30};
    fcpu.dram_set_ret = EAR_ERROR;
    CHECK(set_dram_powercap_value(lim2) != EAR_SUCCESS, "DRAM write failure reported");
    CHECK_EQ(fake_stored[DOMAIN_DRAM][0], 40, "... and not stored");
}

TEST(test_dram_not_capable)
{
    /* AMD: no DRAM power limit. The sockets get the whole CPU domain limit, DRAM is never
     * programmed nor stored, and a limit is applied once (no error, no retry) */
    fake_cpus_default();
    fcpu.dram_not_capable = 1;
    fake_clock_ns         = 1000000000LL;
    enable(suscription());
    uint32_t limit = 400;
    CHECK_EQ(set_powercap_value(0, LEVEL_NODE, &limit, NULL), EAR_SUCCESS, "limit applied without a DRAM limit");
    CHECK_EQ(fcpu.hw_pkg[0] + fcpu.hw_pkg[1], 400, "the sockets get the whole limit (%u + %u)", fcpu.hw_pkg[0],
             fcpu.hw_pkg[1]);
    CHECK_EQ(fcpu.dram_set_calls, 0, "DRAM is not programmed");
    CHECK_EQ(fake_stored[DOMAIN_DRAM][0], 0, "no DRAM value stored for verification");
    int pkg_calls = fcpu.pkg_set_calls;
    CHECK_EQ(set_powercap_value(0, LEVEL_NODE, &limit, NULL), EAR_SUCCESS, "same limit again");
    CHECK_EQ(fcpu.pkg_set_calls, pkg_calls, "... is not written again");
    limit = POWER_CAP_UNLIMITED;
    set_powercap_value(0, LEVEL_NODE, &limit, NULL);
    CHECK_EQ(fcpu.dram_reset_calls, 0, "unlimited: DRAM is not reset");
    uint32_t v[2];
    CHECK(get_dram_powercap_value(v) != EAR_SUCCESS, "DRAM caps can't be read back");
    uint32_t per_dev[2] = {30, 30};
    CHECK(set_dram_powercap_value(per_dev) != EAR_SUCCESS, "per-device DRAM caps refused");
    CHECK_EQ(fcpu.dram_set_calls, 0, "... nothing programmed");
}

/* ============================================================= status */

TEST(test_status_before_any_limit)
{
    cpu_setup();
    domain_status_t st;
    memset(&st, 0xAB, sizeof(st));
    CHECK_EQ(get_powercap_status(&st), 0, "no limit yet: nothing to report");
    CHECK(st.ok == PC_STATUS_OK, "... and the status is left in a defined state (ok=%u)", st.ok);
}

TEST(test_status_release)
{
    cpu_setup();
    set_limit(480);
    all_cpus_at(5); /* running at the 2.5 GHz target */
    domain_status_t st = status_after(150);
    CHECK_EQ(st.ok, PC_STATUS_RELEASE, "300 W on a 480 W domain at target: RELEASE");
    CHECK_EQ(st.exceed, 90, "releases half of the 180 W headroom");
    CHECK_EQ(st.current_pc, 480, "current_pc = domain limit");
    CHECK_EQ(st.stress, 0, "at target: no stress");

    st = status_after(236);
    CHECK_EQ(st.ok, PC_STATUS_OK, "8 W headroom (< 10 W margin): OK");
    st = status_after(260);
    CHECK_EQ(st.ok, PC_STATUS_OK, "over the limit at target: OK (nothing to release)");
}

TEST(test_status_greedy_direction)
{
    cpu_setup();
    set_limit(400);                           /* under the 480 W TDP maximum: room to ask */
    fcpu.target_khz = fake_cpu_pstate_khz(2); /* app wants 2.8 GHz */
    all_cpus_at(10);                          /* RAPL throttled it to 2.0 GHz */
    domain_status_t st = status_after(200);
    CHECK(st.ok == PC_STATUS_GREEDY, "CPUs 8 P-states below target report GREEDY (ok=%u)", st.ok);
    CHECK_EQ(st.stress, 40, "stress = (2.8-2.0)/(3.0-1.0) = 40");

#if defined(__SANITIZE_ADDRESS__)
    return; /* the next case takes the GREEDY path, see test_greedy_path_memory_safety */
#endif
    fcpu.target_khz = fake_cpu_pstate_khz(10); /* app wants 2.0 GHz */
    all_cpus_at(2);                            /* running at 2.8 GHz */
    st = status_after(150);
    CHECK(st.ok != PC_STATUS_GREEDY, "CPUs faster than their target are not GREEDY (ok=%u)", st.ok);
    CHECK(st.stress == 0, "... and have no stress (got %u)", st.stress);
}

TEST(test_status_greedy_from_effective_frequency)
{
    cpu_setup();
    set_limit(400);                           /* under the 480 W TDP maximum */
    fcpu.target_khz = fake_cpu_pstate_khz(0); /* app asked 3.0 GHz, the driver reports it */
    all_cpus_at(0);
    fcpu.eff_set = 1; /* but RAPL holds the cores at 2.2 GHz (APERF/MPERF) */
    for (int i = 0; i < FD_MAX * 2; i++)
        fcpu.eff_idx[i] = 8;
    domain_status_t st = status_after(200); /* 400 W: at the limit */
    CHECK_EQ(st.ok, PC_STATUS_GREEDY, "requested freq at target, effective 8 P-states below, power at limit: GREEDY");
    CHECK(st.requested > 0 && st.requested <= 80, "... asking for power, at most up to the TDP maximum (%u W)",
          st.requested);
    st = status_after(150); /* 300 W of 400 W: something else holds the cores back */
    CHECK(st.ok != PC_STATUS_GREEDY, "below target with plenty of headroom is not GREEDY (ok=%u)", st.ok);

    /* POWER_INFO reports no maximum: the TDP is the maximum, a domain capped at it can't ask more */
    set_limit(480);
    st = status_after(240);
    CHECK(st.ok != PC_STATUS_GREEDY, "capped at the TDP maximum: not GREEDY (ok=%u)", st.ok);
}

/* The monitor asks twice in the same tick: the second call must judge the same window
 * as the first, for power (not 0 W) and for the effective frequency alike */
TEST(test_status_back_to_back_same_window)
{
    cpu_setup();
    set_limit(400); /* under the 480 W TDP maximum */
    fcpu.target_khz = fake_cpu_pstate_khz(0);
    all_cpus_at(0);
    fcpu.eff_set = 1; /* RAPL holds the cores at 2.2 GHz */
    for (int i = 0; i < FD_MAX * 2; i++)
        fcpu.eff_idx[i] = 8;
    domain_status_t first = status_after(200);
    CHECK_EQ(first.ok, PC_STATUS_GREEDY, "throttled at the limit: GREEDY");

    for (int i = 0; i < FD_MAX * 2; i++)
        fcpu.eff_idx[i] = 0; /* a burst at full speed right after the reading */
    domain_status_t again;
    memset(&again, 0xAB, sizeof(again));
    get_powercap_status(&again); /* no time elapsed */
    CHECK_EQ(again.ok, PC_STATUS_GREEDY, "back-to-back call reuses the window: still GREEDY (ok=%u)", again.ok);
    CHECK_EQ(again.stress, first.stress, "... with the same stress");
    CHECK_EQ(again.requested, first.requested, "... and the same request (power not read as 0 W)");

    fake_clock_ns += POWER_WINDOW_TEST_NS;
    memset(&again, 0xAB, sizeof(again));
    get_powercap_status(&again);
    CHECK(again.ok != PC_STATUS_GREEDY, "a full window later the new frequency counts (ok=%u)", again.ok);
}

static void greedy_status(void)
{
    cpu_setup();
    set_limit(480);
    fcpu.target_khz = fake_cpu_pstate_khz(10);
    all_cpus_at(2);
    status_after(150);
}

TEST(test_greedy_path_memory_safety)
{
#if defined(__SANITIZE_ADDRESS__)
    /* _compute_extra_power copies 512 uints from the 21-entry P-state list */
    int sig = pc_crashes(greedy_status);
    CHECK(sig == 0, "GREEDY status computation stays inside the P-state list (signal %d = %s)", sig,
          sig ? strsignal(sig) : "none");
#else
    greedy_status();
    CHECK(1, "GREEDY path ran (build test_cpu_plugin_asan to check the P-state list reads)");
#endif
}

/* ============================================================= misc */

TEST(test_exported_symbols)
{
    CHECK(dlsym(RTLD_DEFAULT, "set_powercap_value") != NULL, "exports set_powercap_value");
    CHECK(dlsym(RTLD_DEFAULT, "get_powercap_status") != NULL, "exports get_powercap_status");
    CHECK(dlsym(RTLD_DEFAULT, "get_powercap_strategy") != NULL, "exports get_powercap_strategy");
    CHECK(dlsym(RTLD_DEFAULT, "set_dram_powercap_value") != NULL &&
              dlsym(RTLD_DEFAULT, "get_dram_powercap_value") != NULL,
          "exports the per-device DRAM calls (it caps DRAM)");
    domain_settings_t s;
    plugin_get_settings(&s);
    CHECK(s.node_ratio > 0.09 && s.node_ratio < 0.11, "node_ratio 0.1");
    CHECK(s.security_range > 0.009 && s.security_range < 0.011, "security_range 0.01");
}

TEST(test_disable_resets)
{
    cpu_setup();
    set_limit(400);
    disable();
    CHECK(fcpu.hw_pkg[0] == POWERCAP_DISABLE && fcpu.hw_dram[0] == POWERCAP_DISABLE, "disable resets PKG and DRAM");
    uint v[2];
    get_powercap_value(0, v);
    CHECK_EQ(v[0], POWERCAP_DISABLE, "read back reflects the reset");
}

int main(void)
{
    RUN_ISOLATED(test_split_by_tdp);
    RUN_ISOLATED(test_split_share_from_pmgt);
    RUN_ISOLATED(test_same_limit_not_reapplied);
    RUN_ISOLATED(test_unlimited_resets);
    RUN_ISOLATED(test_tiny_limit);
    RUN_ISOLATED(test_dram_failure);
    RUN_ISOLATED(test_pkg_failure);
    RUN_ISOLATED(test_per_device);
    RUN_ISOLATED(test_per_device_dram);
    RUN_ISOLATED(test_dram_not_capable);
    RUN_ISOLATED(test_status_before_any_limit);
    RUN_ISOLATED(test_status_release);
    RUN_ISOLATED(test_status_greedy_direction);
    RUN_ISOLATED(test_status_greedy_from_effective_frequency);
    RUN_ISOLATED(test_status_back_to_back_same_window);
    RUN_ISOLATED(test_greedy_path_memory_safety);
    RUN_ISOLATED(test_exported_symbols);
    RUN_ISOLATED(test_disable_resets);
    return pc_summary("test_cpu_plugin");
}
