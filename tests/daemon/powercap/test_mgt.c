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
 * test_mgt.c -- unit tests for src/daemon/powercap/powercap_mgt.c
 *
 * powercap_mgt.c splits the node limit between the domain plugins (CPU incl.
 * DRAM, GPU), rebalances them when the node goes over / under its limit, moves
 * power between domains from their status, and answers the node status that
 * powercap.c forwards to EARGM. Plugins and hardware discovery are faked
 * (fake_hw_mgt.c), the util-detection thread is driven tick by tick through
 * the fake monitor.
 *
 * Default rig: 2 sockets, PKG TDP 200 W and DRAM TDP 40 W each, 16 CPUs, and
 * 4 (or 2, or 0) NVML GPUs with TDP 400 W / min 100 W. CPU and GPU plugins use
 * the same domain settings as cpu_generic.so and gpu.so.
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>

#include <common/config.h>
#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>
#include <metrics/common/apis.h>

#include "fake_hw_mgt.h"
#include "fake_monitor.h"
#include "fake_powermon.h"
#include "pc_test.h"

/* Globals that powercap.c would provide */
node_powercap_opt_t my_pc_opt;
uint32_t current_mode = 1; /* PC_MODE_AUTO */

static pwr_mgt_t *h;

static void mgt_init(void)
{
    fake_powermon_reset();
    pmgt_init();
    pmgt_handler_alloc(&h);
    pmgt_enable(h);
    pmgt_set_pc_mode(h, PC_MODE_TARGET);
    if (fake_num_subs > 0 && fake_subs[0]->call_init)
        fake_subs[0]->call_init(NULL);
}

static void mgt_setup(int gpus)
{
    fake_hw_default(gpus);
    mgt_init();
}

/* Sets a node limit the way powercap.c does, including my_pc_opt */
static void node_limit(uint32_t limit)
{
    my_pc_opt.base_powercap     = limit;
    my_pc_opt.last_t1_allocated = limit;
    my_pc_opt.current_pc        = limit;
    pmgt_set_powercap_value(h, 0, DOMAIN_NODE, limit);
}

/* One effective pass of util_detect_main (it works every other 500 ms call) */
static void util_tick(void)
{
    suscription_t *s = fake_num_subs ? fake_subs[0] : NULL;
    if (s == NULL)
        return;
    s->call_main(NULL);
    s->call_main(NULL);
}

static uint32_t cpu_cap(void)
{
    return fake_plug[DOMAIN_CPU].cur;
}
static uint32_t gpu_cap(void)
{
    return fake_plug[DOMAIN_GPU].cur;
}

static void set_status(int d, uint ok, uint8_t requested, uint8_t stress, uint32_t exceed)
{
    fake_plug[d].status.ok        = ok;
    fake_plug[d].status.requested = requested;
    fake_plug[d].status.stress    = stress;
    fake_plug[d].status.exceed    = exceed;
}

/* ================================================== static distribution */

TEST(test_split_2cpu_4gpu)
{
    mgt_setup(4);
    CHECK_EQ(fake_plug[DOMAIN_CPU].enable_calls, 1, "CPU plugin enabled");
    CHECK_EQ(fake_plug[DOMAIN_GPU].enable_calls, 1, "GPU plugin enabled");
    CHECK_EQ(fake_plug[DOMAIN_DRAM].enable_calls, 0, "no DRAM plugin (DRAM is inside the CPU domain)");
    node_limit(2000);
    /* TDP shares: PKG+DRAM 480 W, GPUs 1600 W, node overhead 10% of 480 = 48 W */
    CHECK_EQ(cpu_cap(), 451, "CPU domain (2 PKG + 2 DRAM) gets 480/2128 of 2000 W");
    CHECK_EQ(gpu_cap(), 1503, "GPU domain (4 GPUs) gets 1600/2128 of 2000 W");
    CHECK(cpu_cap() + gpu_cap() <= 2000, "CPU + GPU caps within the node limit (%u)", cpu_cap() + gpu_cap());
    CHECK(cpu_cap() + gpu_cap() >= 1900, "... and at most 5%% is held back for the rest of the node");
    CHECK_EQ(fake_plug[DOMAIN_CPU].last_level, LEVEL_NODE, "node-level request");
    CHECK_EQ(fake_plug[DOMAIN_CPU].burst_calls, 1, "a real limit puts the CPU plugin in burst mode");
    CHECK_EQ(fake_plug[DOMAIN_GPU].burst_calls, 1, "... and the GPU plugin");
}

TEST(test_split_2cpu_2gpu)
{
    mgt_setup(2);
    node_limit(2000);
    CHECK_EQ(cpu_cap(), 722, "2 GPUs: CPU domain gets 480/1328");
    CHECK_EQ(gpu_cap(), 1204, "2 GPUs: GPU domain gets 800/1328");
    CHECK(cpu_cap() + gpu_cap() <= 2000, "sum within the node limit");
}

TEST(test_split_cpu_only)
{
    mgt_setup(0);
    node_limit(1000);
    CHECK_EQ(cpu_cap(), 909, "no GPUs: CPU domain gets 480/528 of the node");
    CHECK_EQ(fake_plug[DOMAIN_GPU].set_calls, 0, "no GPU plugin called");
    CHECK_EQ(fake_num_subs, 0, "no util-detection thread without GPUs");
}

TEST(test_split_gpu_plugin_missing)
{
    fake_hw_default(4);
    fake_plug[DOMAIN_GPU].loaded = 0; /* GPUs present, plugin fails to load */
    fake_hw_export_env();
    setenv("EAR_POWERCAP_POLICY_GPU", "fake_gpu", 1);
    mgt_init();
    node_limit(1000);
    CHECK_EQ(cpu_cap(), 909, "GPU plugin not loaded: CPU domain takes the whole CPU share");
}

TEST(test_split_heterogeneous_sockets)
{
    fake_hw_default(4);
    fake_hw.pkg_tdp[1] = 100; /* socket 1 is a smaller part */
    fake_hw.gpu_tdp[3] = 300; /* and one GPU is a different SKU */
    mgt_init();
    node_limit(2000);
    /* PKG 300 + DRAM 80 = 380, GPUs 1500, overhead 38 -> CPU 380/1918, GPU 1500/1918 */
    CHECK(cpu_cap() == 396, "CPU share uses every socket's TDP, not socket 0 x N (cap %u, want 396)",
          cpu_cap());
    CHECK(gpu_cap() == 1564, "GPU share uses every GPU's TDP, not GPU 0 x N (cap %u, want 1564)",
          gpu_cap());
}

TEST(test_unlimited_and_one_watt)
{
    mgt_setup(4);
    pmgt_set_powercap_value(h, 0, DOMAIN_NODE, POWER_CAP_UNLIMITED);
    CHECK_EQ(cpu_cap(), POWER_CAP_UNLIMITED, "node unlimited -> CPU plugin told unlimited (1)");
    CHECK_EQ(gpu_cap(), POWER_CAP_UNLIMITED, "node unlimited -> GPU plugin told unlimited (1)");
    CHECK_EQ(fake_plug[DOMAIN_CPU].relax_calls, 1, "unlimited relaxes the CPU plugin monitor");
    pmgt_status_t st;
    pmgt_get_status(&st);
    CHECK_EQ(st.status, PC_STATUS_OK, "unlimited node status is OK");
    CHECK_EQ(fake_plug[DOMAIN_CPU].status_calls, 0, "... without asking the domains");
    int calls = fake_plug[DOMAIN_CPU].set_calls;
    dom_power_t p = {.platform = 5000};
    pmgt_set_power_per_domain(h, &p, PC_STATUS_RUN);
    CHECK_EQ(fake_plug[DOMAIN_CPU].set_calls, calls, "unlimited: no rebalancing even at 5 kW");

    /* A small limit must never be rounded to the 'unlimited' sentinel */
    pmgt_set_powercap_value(h, 0, DOMAIN_NODE, 4);
    CHECK(cpu_cap() != POWER_CAP_UNLIMITED,
          "a 4 W node limit does not become 'unlimited' in the CPU domain (cap %u)", cpu_cap());
}

TEST(test_manual_mode_blocks_auto)
{
    mgt_setup(4);
    current_mode = 2; /* PC_MODE_MANUAL */
    node_limit(2000);
    CHECK_EQ(fake_plug[DOMAIN_CPU].set_calls, 0, "manual mode: node limit not distributed");
    int ticks = fake_plug[DOMAIN_CPU].status_calls;
    util_tick();
    CHECK_EQ(fake_plug[DOMAIN_CPU].status_calls, ticks, "manual mode: util thread does nothing");
    current_mode = 1;
}

/* =============================================== over / under the limit */

TEST(test_over_limit_shrinks_domains)
{
    mgt_setup(4);
    node_limit(2000);
    dom_power_t p = {.platform = 2200, .cpu = 500, .dram = 60, .gpu = 1500};
    pmgt_set_power_per_domain(h, &p, PC_STATUS_RUN);
    /* 10% over: every weight scaled by 0.9 */
    CHECK_EQ(cpu_cap(), 406, "10%% over: CPU cap scaled by 0.9");
    CHECK_EQ(gpu_cap(), 1353, "10%% over: GPU cap scaled by 0.9");
    CHECK(cpu_cap() + gpu_cap() <= 1800, "domains now sum to <= 90%% of the limit");

    /* Sustained 10% overshoot from something EAR does not cap (fans, NICs...) */
    for (int i = 0; i < 30; i++)
        pmgt_set_power_per_domain(h, &p, PC_STATUS_RUN);
    /* floor: the hardware minimum (4 x 100 W GPUs), or a quarter of the share when unknown (CPU) */
    CHECK(cpu_cap() >= 451 / 4, "sustained overshoot keeps the CPU above a quarter of its share (cap %u)", cpu_cap());
    CHECK(gpu_cap() >= 400, "sustained overshoot keeps the GPUs at their 4 x 100 W hardware minimum (cap %u)",
          gpu_cap());
}

TEST(test_domain_shares_published)
{
    mgt_setup(4);
    node_limit(2000);
    CHECK(abs((int) my_pc_opt.pper_domain[DOMAIN_CPU] - (int) cpu_cap()) <= 1 &&
              abs((int) my_pc_opt.pper_domain[DOMAIN_GPU] - (int) gpu_cap()) <= 1,
          "pper_domain holds the CPU/GPU split applications read (%u/%u vs %u/%u)", my_pc_opt.pper_domain[DOMAIN_CPU],
          my_pc_opt.pper_domain[DOMAIN_GPU], cpu_cap(), gpu_cap());
    dom_power_t p = {.platform = 2200};
    pmgt_set_power_per_domain(h, &p, PC_STATUS_RUN);
    CHECK(abs((int) my_pc_opt.pper_domain[DOMAIN_CPU] - (int) cpu_cap()) <= 1,
          "... and follows a rebalance (%u vs %u)", my_pc_opt.pper_domain[DOMAIN_CPU], cpu_cap());
}

/* Each job sees the part of the CPU share its CPUs are of the node; GPU share is the node's */
TEST(test_domain_shares_per_job)
{
    mgt_setup(4);
    fake_ctx_start(1, 2400000, 1200000, 4);
    fake_ctx_start(2, 2400000, 1200000, 4);
    CPU_ZERO(&fake_ctx[1].app_info.node_mask);
    CPU_ZERO(&fake_ctx[2].app_info.node_mask);
    for (int c = 0; c < 4; c++)
        CPU_SET(c, &fake_ctx[1].app_info.node_mask); /* 4 of 16 CPUs */
    for (int c = 4; c < 16; c++)
        CPU_SET(c, &fake_ctx[2].app_info.node_mask); /* 12 of 16 CPUs */
    node_limit(2000);
    uint32_t cpu = my_pc_opt.pper_domain[DOMAIN_CPU], gpu = my_pc_opt.pper_domain[DOMAIN_GPU];
    uint32_t *j1 = fake_ctx[1].settings.pc_opt.pper_domain, *j2 = fake_ctx[2].settings.pc_opt.pper_domain;
    CHECK(abs((int) j1[DOMAIN_CPU] - (int) (cpu / 4)) <= 1 && abs((int) j2[DOMAIN_CPU] - (int) (cpu * 3 / 4)) <= 1,
          "CPU share by the job's CPUs: %u and %u of %u W", j1[DOMAIN_CPU], j2[DOMAIN_CPU], cpu);
    CHECK(j1[DOMAIN_GPU] == gpu && j2[DOMAIN_GPU] == gpu, "GPU share is the node's (no GPU map): %u %u of %u W",
          j1[DOMAIN_GPU], j2[DOMAIN_GPU], gpu);

    /* a CPU<->GPU move reaches the jobs too */
    set_status(DOMAIN_CPU, PC_STATUS_GREEDY, 15, 80, 0);
    set_status(DOMAIN_GPU, PC_STATUS_RELEASE, 0, 0, 100);
    util_tick();
    CHECK(my_pc_opt.pper_domain[DOMAIN_GPU] == gpu_cap() && abs((int) my_pc_opt.pper_domain[DOMAIN_CPU] - (int) cpu_cap()) <= 1,
          "after a move the node shares follow the caps (%u/%u vs %u/%u)", my_pc_opt.pper_domain[DOMAIN_CPU],
          my_pc_opt.pper_domain[DOMAIN_GPU], cpu_cap(), gpu_cap());
    CHECK(abs((int) j2[DOMAIN_CPU] - (int) (cpu_cap() * 3 / 4)) <= 1 && j2[DOMAIN_GPU] == gpu_cap(),
          "... and so do the jobs' (%u/%u)", j2[DOMAIN_CPU], j2[DOMAIN_GPU]);
}

TEST(test_floor_is_hardware_minimum)
{
    fake_hw_default(4);
    for (int s = 0; s < 2; s++) {
        fake_hw.pkg_min[s]  = 120; /* RAPL POWER_INFO */
        fake_hw.dram_min[s] = 10;
    }
    mgt_init();
    node_limit(2000);
    dom_power_t p = {.platform = 2200};
    for (int i = 0; i < 40; i++)
        pmgt_set_power_per_domain(h, &p, PC_STATUS_RUN);
    CHECK(cpu_cap() >= 260, "the CPU share stops at its RAPL minimum, 2 x (120 + 10) W (cap %u)", cpu_cap());
}

TEST(test_under_limit_recovers_to_default)
{
    mgt_setup(4);
    node_limit(2000);
    dom_power_t over = {.platform = 2400};
    pmgt_set_power_per_domain(h, &over, PC_STATUS_RUN);
    CHECK(cpu_cap() < 451, "shrunk after 20%% overshoot (cpu %u)", cpu_cap());

    /* Inside the security band (98%..100%) nothing moves */
    uint32_t c = cpu_cap(), g = gpu_cap();
    dom_power_t band = {.platform = 1970};
    pmgt_set_power_per_domain(h, &band, PC_STATUS_RUN);
    CHECK(cpu_cap() == c && gpu_cap() == g, "power in the 98..100%% band: weights frozen");

    dom_power_t under = {.platform = 1500};
    for (int i = 0; i < 40; i++)
        pmgt_set_power_per_domain(h, &under, PC_STATUS_RUN);
    CHECK_EQ(cpu_cap(), 451, "well under the limit: CPU grows back to its default, not beyond");
    CHECK_EQ(gpu_cap(), 1503, "well under the limit: GPU grows back to its default, not beyond");
}

TEST(test_rebalance_while_asking_default)
{
    mgt_setup(4);
    node_limit(2000);
    my_pc_opt.last_t1_allocated = 1800; /* node waiting for its default */
    uint c                      = cpu_cap();
    dom_power_t p               = {.platform = 2500};
    pmgt_set_power_per_domain(h, &p, PC_STATUS_RUN);
    CHECK(cpu_cap() < c, "allocated < default and 25%% over the limit: domains still shrink (cpu %u -> %u)", c,
          cpu_cap());
}

TEST(test_job_events_reset_weights)
{
    mgt_setup(4);
    node_limit(2000);
    dom_power_t over = {.platform = 2400};
    pmgt_set_power_per_domain(h, &over, PC_STATUS_RUN);
    pmgt_new_job(h);
    CHECK_EQ(cpu_cap(), 451, "new job restores the default CPU share");
    CHECK_EQ(gpu_cap(), 1503, "new job restores the default GPU share");
    CHECK_EQ(fake_sub_state[0], 1, "new job puts util detection in burst");
    pmgt_set_power_per_domain(h, &over, PC_STATUS_RUN);
    pmgt_idle_to_run(h);
    CHECK_EQ(cpu_cap(), 451, "idle_to_run restores the default share");
    CHECK_EQ(fake_plug[DOMAIN_GPU].last_set_status, PC_STATUS_RUN, "domains told RUN");
    pmgt_run_to_idle(h);
    CHECK_EQ(fake_plug[DOMAIN_CPU].last_set_status, PC_STATUS_IDLE, "domains told IDLE");
    CHECK_EQ(cpu_cap(), 451, "idle share == default share");
    pmgt_end_job(h);
    CHECK_EQ(fake_sub_state[0], 0, "end job relaxes util detection");
}

/* ======================================================== node status */

TEST(test_node_status_combinations)
{
    mgt_setup(4);
    node_limit(2000);
    pmgt_status_t st;

    set_status(DOMAIN_CPU, PC_STATUS_OK, 0, 0, 0);
    set_status(DOMAIN_GPU, PC_STATUS_OK, 0, 0, 0);
    pmgt_get_status(&st);
    CHECK_EQ(st.status, PC_STATUS_OK, "OK + OK -> OK");

    set_status(DOMAIN_CPU, PC_STATUS_GREEDY, 30, 70, 0);
    set_status(DOMAIN_GPU, PC_STATUS_OK, 0, 10, 0);
    pmgt_get_status(&st);
    CHECK_EQ(st.status, PC_STATUS_GREEDY, "GREEDY + OK -> GREEDY");
    CHECK_EQ(st.requested, 30, "requested = CPU request");
    CHECK_EQ(st.stress, 70, "stress = max stress");

    set_status(DOMAIN_GPU, PC_STATUS_GREEDY, 40, 90, 0);
    pmgt_get_status(&st);
    CHECK_EQ(st.requested, 70, "GREEDY + GREEDY -> requests add up");
    CHECK_EQ(st.stress, 90, "... stress is the max");

    set_status(DOMAIN_CPU, PC_STATUS_GREEDY, 200, 50, 0);
    set_status(DOMAIN_GPU, PC_STATUS_GREEDY, 200, 50, 0);
    pmgt_get_status(&st);
    CHECK_EQ(st.requested, 400, "200 + 200 W requested adds up to 400 W (no uint8 wrap)");

    set_status(DOMAIN_GPU, PC_STATUS_RELEASE, 0, 0, 120);
    pmgt_get_status(&st);
    CHECK_EQ(st.status, PC_STATUS_OK, "GREEDY + RELEASE -> OK (rebalanced internally)");

    set_status(DOMAIN_CPU, PC_STATUS_OK, 0, 0, 0);
    set_status(DOMAIN_GPU, PC_STATUS_RELEASE, 0, 0, 120);
    pmgt_get_status(&st);
    CHECK_EQ(st.status, PC_STATUS_RELEASE, "OK + RELEASE -> RELEASE");
    CHECK_EQ(st.tbr, 120, "to-be-released = GPU exceed");

    set_status(DOMAIN_CPU, PC_STATUS_RELEASE, 0, 0, 60);
    pmgt_get_status(&st);
    CHECK_EQ(st.tbr, 180, "RELEASE + RELEASE -> tbr adds up");

    set_status(DOMAIN_CPU, PC_STATUS_OK, 0, 0, 0);
    set_status(DOMAIN_GPU, PC_STATUS_ASK_DEF, 0, 0, 0);
    pmgt_get_status(&st);
    CHECK_EQ(st.status, PC_STATUS_OK, "OK + ASK_DEF -> OK at node level (handled inside the node)");
}

TEST(test_node_status_cpu_only)
{
    mgt_setup(0);
    node_limit(450); /* within the 2 x (200 + 40) W the sockets can take */
    pmgt_status_t st;
    set_status(DOMAIN_CPU, PC_STATUS_GREEDY, 25, 60, 0);
    pmgt_get_status(&st);
    CHECK_EQ(st.status, PC_STATUS_GREEDY, "CPU-only node: CPU GREEDY -> GREEDY");
    CHECK_EQ(st.requested, 25, "requested 25");
    set_status(DOMAIN_CPU, PC_STATUS_RELEASE, 0, 0, 80);
    pmgt_get_status(&st);
    CHECK_EQ(st.status, PC_STATUS_RELEASE, "CPU-only node: CPU RELEASE -> RELEASE");
    CHECK_EQ(st.tbr, 80, "tbr 80");

    /* a limit above what the sockets can ever take: the difference goes back to EARGM */
    node_limit(1000);
    set_status(DOMAIN_CPU, PC_STATUS_OK, 0, 0, 0);
    pmgt_get_status(&st);
    CHECK_EQ(st.status, PC_STATUS_RELEASE, "1000 W on a 480 W node -> RELEASE");
    CHECK_EQ(st.tbr, 520, "offers the 520 W the devices can't use");
}

/* ========================================= internal CPU <-> GPU moves */

TEST(test_move_gpu_release_to_greedy_cpu)
{
    mgt_setup(4);
    node_limit(2000);
    set_status(DOMAIN_CPU, PC_STATUS_GREEDY, 15, 80, 0);
    set_status(DOMAIN_GPU, PC_STATUS_RELEASE, 0, 0, 100);
    util_tick();
    CHECK_EQ(gpu_cap(), 1403, "GPU gives its 100 W surplus");
    CHECK_EQ(cpu_cap(), 551, "CPU receives it");
    CHECK_EQ(cpu_cap() + gpu_cap(), 1954, "power is moved, not created");
    CHECK_EQ(fake_plug[DOMAIN_CPU].last_level, LEVEL_DOMAIN, "moves are domain-level requests");
}

TEST(test_move_cpu_release_to_greedy_gpu)
{
    mgt_setup(4);
    node_limit(2000);
    set_status(DOMAIN_CPU, PC_STATUS_RELEASE, 0, 0, 60);
    set_status(DOMAIN_GPU, PC_STATUS_GREEDY, 30, 80, 0);
    util_tick();
    CHECK_EQ(cpu_cap(), 391, "CPU gives its 60 W surplus");
    CHECK_EQ(gpu_cap(), 1563, "GPU receives it");
}

TEST(test_move_ok_cpu_to_greedy_gpu_is_bounded)
{
    mgt_setup(4);
    node_limit(2000);
    set_status(DOMAIN_CPU, PC_STATUS_OK, 0, 0, 0);
    set_status(DOMAIN_GPU, PC_STATUS_GREEDY, 30, 80, 0);
    util_tick();
    CHECK_EQ(gpu_cap(), 1523, "OK CPU lends min(requested 30, 10%% of 451, 20) = 20 W");
    CHECK_EQ(cpu_cap(), 431, "CPU keeps the rest");

    /* The GPU stays greedy for a long time (memory-bound app pegging the GPUs) */
    for (int i = 0; i < 100; i++)
        util_tick();
    CHECK(cpu_cap() + gpu_cap() <= 2000, "still within the node limit after 100 moves (%u)", cpu_cap() + gpu_cap());
    CHECK(cpu_cap() >= 451 / 4, "an OK CPU is not drained below a quarter of its share (cap %u)",
          cpu_cap());
}

TEST(test_move_between_greedy_domains_by_stress)
{
    mgt_setup(4);
    node_limit(2000);
    set_status(DOMAIN_CPU, PC_STATUS_GREEDY, 50, 90, 0);
    set_status(DOMAIN_GPU, PC_STATUS_GREEDY, 50, 20, 0);
    util_tick();
    CHECK_EQ(cpu_cap(), 471, "both greedy, CPU far more stressed: CPU gets 20 W");
    CHECK_EQ(gpu_cap(), 1483, "... from the GPUs");

    set_status(DOMAIN_CPU, PC_STATUS_GREEDY, 50, 50, 0);
    set_status(DOMAIN_GPU, PC_STATUS_GREEDY, 50, 40, 0);
    uint32_t c = cpu_cap();
    util_tick();
    CHECK_EQ(cpu_cap(), c, "both greedy, stress within 20: nothing moves");

    set_status(DOMAIN_CPU, PC_STATUS_GREEDY, 5, 10, 0);
    set_status(DOMAIN_GPU, PC_STATUS_GREEDY, 50, 95, 0);
    c = cpu_cap();
    util_tick();
    CHECK_EQ(gpu_cap(), 1483 + 20, "GPU far more stressed: GPU gets 20 W");
    CHECK_EQ(cpu_cap(), c - 20, "... from the CPU");
}

TEST(test_both_release_nothing_moves)
{
    mgt_setup(4);
    node_limit(2000);
    set_status(DOMAIN_CPU, PC_STATUS_RELEASE, 0, 0, 50);
    set_status(DOMAIN_GPU, PC_STATUS_RELEASE, 0, 0, 50);
    util_tick();
    CHECK(cpu_cap() == 451 && gpu_cap() == 1503, "RELEASE + RELEASE: no internal move (EARGM collects it)");
}

TEST(test_ask_default_resets_domains)
{
    mgt_setup(4);
    node_limit(2000);
    set_status(DOMAIN_GPU, PC_STATUS_ASK_DEF, 0, 0, 0);
    util_tick();
    CHECK(fake_plug[DOMAIN_GPU].reset_calls >= 1, "a domain in ASK_DEF triggers a reset to default");
    CHECK(fake_plug[DOMAIN_CPU].reset_calls >= 1, "... in every domain");
}

TEST(test_node_plugin_with_gpus)
{
    fake_hw_default(4);
    fake_plug[DOMAIN_NODE].loaded = 1; /* e.g. inm.so for the node + gpu.so */
    fake_plug[DOMAIN_CPU].loaded  = 0;
    fake_hw_export_env();
    mgt_init();
    node_limit(2000);
    CHECK_EQ(fake_plug[DOMAIN_NODE].cur, 2000, "node plugin gets the whole node limit");
    CHECK_EQ(fake_plug[DOMAIN_CPU].enable_calls, 0, "a node plugin excludes the CPU plugin");
    CHECK_EQ(gpu_cap(), 1538, "GPUs get 1600/2080 (no node overhead: it comes from the CPU plugin settings)");
    CHECK_EQ(fake_num_subs, 0, "no util-detection thread with a node plugin: GPUs never rebalanced internally");
}

static void manual_then_auto_move(void)
{
    fake_hw_default(4);
    mgt_init();
    node_limit(2000);
    current_mode   = 2; /* econtrol: manual */
    int32_t dom[2] = {900, 1500};
    pmgt_process_message("domain", 2, dom);
    current_mode = 1; /* econtrol: deactivate */
    set_status(DOMAIN_GPU, PC_STATUS_GREEDY, 30, 80, 0);
    util_tick();
}

TEST(test_leaving_manual_mode_with_domains_over_limit)
{
    /* Domain limits set by hand add up to 2400 W on a 2000 W node */
    int sig = pc_crashes(manual_then_auto_move);
    CHECK(sig == 0, "back to auto with manual domain caps over the node limit: EARD survives (signal %d = %s)",
          sig, sig ? strsignal(sig) : "none");
}

/* ===================================================== utilization */

TEST(test_gpu_utilization_change_redistributes)
{
    mgt_setup(4);
    node_limit(2000);
    util_tick();
    int n = fake_plug[DOMAIN_GPU].new_util_calls;
    fake_hw.gpu_raw[2].util_gpu = 0;
    fake_hw.gpu_raw[3].util_gpu = 0;
    util_tick();
    CHECK_EQ(fake_plug[DOMAIN_GPU].new_util_calls, n + 1, "two GPUs going idle -> GPU plugin gets new utilization");
    CHECK(fake_plug[DOMAIN_GPU].new_util[0] == 80 && fake_plug[DOMAIN_GPU].new_util[2] == 0,
          "per-GPU utilization forwarded (%lu %lu %lu %lu)", fake_plug[DOMAIN_GPU].new_util[0],
          fake_plug[DOMAIN_GPU].new_util[1], fake_plug[DOMAIN_GPU].new_util[2], fake_plug[DOMAIN_GPU].new_util[3]);
    n = fake_plug[DOMAIN_GPU].new_util_calls;
    util_tick();
    CHECK_EQ(fake_plug[DOMAIN_GPU].new_util_calls, n, "same utilization: no new redistribution");
}

TEST(test_cpu_load_change_redistributes)
{
    mgt_setup(4);
    node_limit(2000);
    util_tick();
    int n        = fake_plug[DOMAIN_CPU].new_util_calls;
    fake_hw.load = 2.0;
    util_tick();
    CHECK_EQ(fake_plug[DOMAIN_CPU].new_util_calls, n + 1, "load 8 -> 2: CPU plugin gets new utilization");
}

/* ===================================================== manual messages */

TEST(test_manual_messages)
{
    mgt_setup(4);
    node_limit(2000);
    int32_t dom[2] = {300, 900};
    pmgt_process_message("domain", 2, dom);
    CHECK(cpu_cap() == 300 && gpu_cap() == 900, "domain [300, 900] -> CPU 300, GPU 900 (%u %u)", cpu_cap(),
          gpu_cap());
    CHECK_EQ(fake_plug[DOMAIN_CPU].last_level, LEVEL_DOMAIN, "domain level");

    int32_t one[1] = {250};
    pmgt_process_message("cpu", 1, one);
    CHECK(cpu_cap() == 250 && gpu_cap() == 900, "cpu [250] -> CPU 250, GPU keeps 900");

    int32_t sock[2] = {120, 140};
    pmgt_process_message("cpu", 2, sock);
    CHECK_EQ(fake_plug[DOMAIN_CPU].last_level, LEVEL_DEVICE, "cpu with one value per socket -> per device");
    CHECK(fake_plug[DOMAIN_CPU].last_limits[0] == 120 && fake_plug[DOMAIN_CPU].last_limits[1] == 140,
          "socket 0 120 W, socket 1 140 W");

    int32_t gpus[4] = {300, 250, 200, 150};
    pmgt_process_message("gpu", 4, gpus);
    CHECK_EQ(fake_plug[DOMAIN_GPU].last_level, LEVEL_DEVICE, "gpu with one value per GPU -> per device");
    CHECK(fake_plug[DOMAIN_GPU].last_limits[0] == 300 && fake_plug[DOMAIN_GPU].last_limits[3] == 150,
          "per-GPU limits forwarded in order");

    int32_t dev[8] = {110, 130, 30, 35, 310, 320, 330, 340};
    pmgt_process_message("device", 8, dev);
    CHECK(fake_plug[DOMAIN_CPU].last_limits[0] == 110 && fake_plug[DOMAIN_CPU].last_limits[1] == 130,
          "device: first 2 values are the sockets");
    CHECK(fake_plug[DOMAIN_GPU].last_limits[0] == 310 && fake_plug[DOMAIN_GPU].last_limits[3] == 340,
          "device: values 5..8 are the GPUs (3..4 are DRAM)");
    CHECK(fake_hw.dram_cap[0] == 30 && fake_hw.dram_cap[1] == 35,
          "device: DRAM values 3..4 programmed by the CPU plugin that caps DRAM (%u %u)", fake_hw.dram_cap[0],
          fake_hw.dram_cap[1]);
    CHECK(fake_hw.stored_dram[0] == 30 && fake_hw.stored_dram[1] == 35, "... and stored for verification");

    /* AMD: no DRAM power limit, the DRAM values are ignored and not stored */
    fake_hw.dram_not_capable = 1;
    int32_t dev_amd[8] = {115, 135, 40, 45, 300, 300, 300, 300};
    pmgt_process_message("device", 8, dev_amd);
    CHECK(fake_plug[DOMAIN_CPU].last_limits[0] == 115 && fake_plug[DOMAIN_GPU].last_limits[0] == 300,
          "device without DRAM capping: CPU and GPU values still applied");
    CHECK(fake_hw.dram_cap[0] == 30 && fake_hw.dram_cap[1] == 35, "... DRAM not programmed (%u %u)",
          fake_hw.dram_cap[0], fake_hw.dram_cap[1]);
    CHECK(fake_hw.stored_dram[0] == 30 && fake_hw.stored_dram[1] == 35,
          "... and the stored DRAM values are not overwritten (%u %u)", fake_hw.stored_dram[0], fake_hw.stored_dram[1]);
    fake_hw.dram_not_capable = 0;

    int n = fake_plug[DOMAIN_CPU].set_calls;
    pmgt_process_message("device", 5, dev);
    CHECK_EQ(fake_plug[DOMAIN_CPU].set_calls, n, "device with too few values is rejected");
    pmgt_process_message("gpu", 0, gpus);
    CHECK_EQ(fake_plug[DOMAIN_CPU].set_calls, n, "zero values is a no-op");

    int32_t three[3] = {100, 100, 100};
    pmgt_process_message("cpu", 3, three);
    CHECK(fake_plug[DOMAIN_CPU].set_calls == n,
          "cpu with 3 values on a 2-socket node is rejected, not turned into a re-apply of old domain limits");

    int32_t neg[1] = {-5};
    CHECK_EQ(pmgt_process_message("gpu", 1, neg), EAR_ERROR, "negative value: message refused");
    CHECK(gpu_cap() != (uint32_t) -5, "a negative GPU limit is rejected (GPU cap %u)", gpu_cap());
}

static void null_level_message(void)
{
    fake_hw_default(4);
    mgt_init();
    int32_t v[1] = {500};
    pmgt_process_message(NULL, 1, v);
}

TEST(test_manual_message_without_level)
{
    /* powercap_process_message() forwards a NULL level for "set" without -l */
    int sig = pc_crashes(null_level_message);
    CHECK(sig == 0, "set with no level does not crash EARD (signal %d = %s)", sig,
          sig ? strsignal(sig) : "none");
}

/* ============================================================= misc */

TEST(test_per_device_readback)
{
    mgt_setup(4);
    node_limit(2000);
    uint32_t v[FAKE_MAX_DEV];
    fake_plug[DOMAIN_CPU].dev_caps[0] = 150;
    fake_plug[DOMAIN_CPU].dev_caps[1] = 200;
    CHECK_EQ(pmgt_get_powercap_value_per_device(DOMAIN_CPU, v), EAR_SUCCESS, "CPU per-device read");
    CHECK(v[0] == 150 && v[1] == POWER_CAP_UNLIMITED, "cap >= TDP reads back as unlimited (%u %u)", v[0], v[1]);
    for (int i = 0; i < 4; i++)
        fake_plug[DOMAIN_GPU].dev_caps[i] = 250 + 50 * i;
    pmgt_get_powercap_value_per_device(DOMAIN_GPU, v);
    CHECK(v[0] == 250 && v[2] == 350 && v[3] == POWER_CAP_UNLIMITED, "GPU at TDP (400) reads back as unlimited");
    fake_hw.dram_cap[0] = 30;
    fake_hw.dram_cap[1] = 35;
    CHECK_EQ(pmgt_get_powercap_value_per_device(DOMAIN_DRAM, v), EAR_SUCCESS,
             "no DRAM plugin: DRAM read back through the CPU plugin");
    CHECK(v[0] == 30 && v[1] == 35, "... per socket (%u %u)", v[0], v[1]);
    CHECK(pmgt_domain_readable(DOMAIN_DRAM), "DRAM readable through the CPU plugin");

    /* a CPU plugin that does not cap DRAM (dvfs, inm, grace_hopper) */
    fake_hw_default(4);
    fake_plug[DOMAIN_CPU].owns_dram = 0;
    mgt_init();
    CHECK(pmgt_get_powercap_value_per_device(DOMAIN_DRAM, v) != EAR_SUCCESS,
          "no DRAM plugin, CPU plugin without DRAM -> error");
    CHECK(!pmgt_domain_readable(DOMAIN_DRAM), "... and DRAM is not readable");
    CHECK_EQ(pmgt_get_powercap_value_per_device(9, v), EAR_ERROR, "invalid domain -> error");
    CHECK_EQ(pmgt_get_cpu_devices(), 2, "2 CPU devices");
    CHECK_EQ(pmgt_get_gpu_devices(), 4, "4 GPU devices");
}

TEST(test_requested_frequencies)
{
    mgt_setup(4);
    node_limit(2000);
    pmgt_set_app_req_freq(h);
    CHECK_EQ(fake_plug[DOMAIN_CPU].app_freq[0], fake_pstate_freq(1), "no app: CPUs at the default policy pstate");
    fake_ctx_start(1, 2400000, 1200000, 4);
    pmgt_set_app_req_freq(h);
    CHECK_EQ(fake_plug[DOMAIN_CPU].app_freq[5], 2400000, "app asks 2.4 GHz: CPU plugin sees it");
    CHECK_EQ(fake_plug[DOMAIN_GPU].app_freq[3], 1200000, "app GPU frequency forwarded to the GPU plugin");
    CHECK_EQ(pmgt_get_powercap_cpu_strategy(h), PC_POWER, "CPU strategy");
    fake_plug[DOMAIN_CPU].strategy = PC_DVFS;
    CHECK_EQ(pmgt_get_powercap_cpu_strategy(h), PC_DVFS, "CPU strategy DVFS reported");
}

/* Accessors (migrated from tests/test_powercap_mgt.c) */
static void get_per_domain_value(void)
{
    fake_hw_default(4);
    mgt_init();
    node_limit(2000);
    uint32_t pc[NUM_DOMAINS];
    pmgt_get_powercap_value(h, 0, pc);
}

TEST(test_accessors_and_disable)
{
    mgt_setup(4);
    node_limit(2000);
    CHECK_EQ(pmgt_is_powercap_enabled(h, 0), 2, "CPU and GPU plugins report enabled");

    /* the plugins' get_powercap_value() writes one value per device (2 sockets, 4 GPUs) */
    int sig = pc_crashes(get_per_domain_value);
    CHECK(sig == 0, "per-domain powercap read survives multi-device plugins (signal %d = %s)", sig,
          sig ? strsignal(sig) : "none");
    CHECK_EQ(pmgt_get_powercap_value(h, 0, NULL), EAR_ERROR, "NULL output rejected");

    char b[512] = "";
    pmgt_powercap_to_str(h, b);
    CHECK(strstr(b, "fake_cpu 451") && strstr(b, "fake_gpu 1503"), "to_str lists every loaded domain (%s)", b);
    pmgt_set_powercap_value(h, 0, DOMAIN_NODE, POWER_CAP_UNLIMITED);
    b[0] = 0;
    pmgt_powercap_to_str(h, b);
    CHECK(!strcmp(b, "Node powercap unlimited"), "to_str when unlimited");
    CHECK_EQ(pmgt_disable(h), EAR_SUCCESS, "disable");
    CHECK(fake_plug[DOMAIN_CPU].disable_calls == 1 && fake_plug[DOMAIN_GPU].disable_calls == 1,
          "every loaded plugin disabled");
    uint32_t pc[FAKE_MAX_DEV];
    CHECK_EQ(pmgt_get_powercap_value_per_device(DOMAIN_CPU, pc), EAR_ERROR, "no domain answers after disable");
}

int main(void)
{
    RUN_ISOLATED(test_split_2cpu_4gpu);
    RUN_ISOLATED(test_split_2cpu_2gpu);
    RUN_ISOLATED(test_split_cpu_only);
    RUN_ISOLATED(test_split_gpu_plugin_missing);
    RUN_ISOLATED(test_split_heterogeneous_sockets);
    RUN_ISOLATED(test_unlimited_and_one_watt);
    RUN_ISOLATED(test_manual_mode_blocks_auto);
    RUN_ISOLATED(test_over_limit_shrinks_domains);
    RUN_ISOLATED(test_domain_shares_published);
    RUN_ISOLATED(test_domain_shares_per_job);
    RUN_ISOLATED(test_floor_is_hardware_minimum);
    RUN_ISOLATED(test_under_limit_recovers_to_default);
    RUN_ISOLATED(test_rebalance_while_asking_default);
    RUN_ISOLATED(test_job_events_reset_weights);
    RUN_ISOLATED(test_node_status_combinations);
    RUN_ISOLATED(test_node_status_cpu_only);
    RUN_ISOLATED(test_move_gpu_release_to_greedy_cpu);
    RUN_ISOLATED(test_move_cpu_release_to_greedy_gpu);
    RUN_ISOLATED(test_move_ok_cpu_to_greedy_gpu_is_bounded);
    RUN_ISOLATED(test_move_between_greedy_domains_by_stress);
    RUN_ISOLATED(test_both_release_nothing_moves);
    RUN_ISOLATED(test_ask_default_resets_domains);
    RUN_ISOLATED(test_node_plugin_with_gpus);
    RUN_ISOLATED(test_leaving_manual_mode_with_domains_over_limit);
    RUN_ISOLATED(test_gpu_utilization_change_redistributes);
    RUN_ISOLATED(test_cpu_load_change_redistributes);
    RUN_ISOLATED(test_manual_messages);
    RUN_ISOLATED(test_manual_message_without_level);
    RUN_ISOLATED(test_per_device_readback);
    RUN_ISOLATED(test_requested_frequencies);
    RUN_ISOLATED(test_accessors_and_disable);
    return pc_summary("test_mgt");
}
