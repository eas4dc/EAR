/*
 * test_gpu_plugin.c -- unit tests for src/daemon/powercap/gpu.c (gpu.so)
 *
 * The GPU domain plugin receives one limit for all the GPUs of the node and
 * splits it by utilization: idle GPUs get their minimum, busy GPUs share the
 * rest in proportion to their utilization, clamped to [min, max]. It also
 * decides the GPU domain status (OK / GREEDY / RELEASE / ASK_DEF) from
 * frequency and power readings, and runs a monitor thread that moves spare
 * power between GPUs.
 *
 * Rig: 4 GPUs (2 where stated), cap range 100..400 W, frequencies
 * 1410..210 MHz (kHz in the API).
 */
#include <pthread.h>

#include <common/config.h>
#include <common/system/monitor.h>
#include <daemon/powercap/powercap_status_conf.h>

#include "fake_dev_api.h"
#include "pc_test.h"

/* gpu.so exports (see powercap_mgt.c:pcsyms_names) */
state_t enable(suscription_t *sus);
state_t disable();
state_t set_powercap_value(uint pid, uint32_t domain, uint32_t *limit, uint32_t *gpu_util);
state_t reset_powercap_value();
state_t get_powercap_value(uint pid, uint32_t *powercap);
uint get_powercap_status(domain_status_t *status);
void set_new_utilization(uint32_t *util);
void set_app_req_freq(ulong *f);
void set_status(uint status);
state_t gpu_pc_thread_main(void *p);

static suscription_t *sus;

static void gpu_setup(uint n)
{
    fake_gpus_default(n);
    sus = suscription();
    enable(sus);
    sus->call_init(NULL);
}

static uint32_t hw_sum(void)
{
    uint32_t s = 0;
    for (uint i = 0; i < fgpu.n; i++)
        s += fgpu.hw_cap[i];
    return s;
}

static void set_limit(uint32_t limit, uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3)
{
    uint32_t util[4] = {u0, u1, u2, u3};
    set_powercap_value(0, LEVEL_NODE, &limit, util);
}

static void targets(ulong khz)
{
    ulong f[MAX_GPUS_SUPPORTED] = {0};
    for (uint i = 0; i < fgpu.n; i++) {
        f[i]              = khz;
        fgpu.target_khz[i] = khz;
    }
    set_app_req_freq(f);
}

static void readings(ulong util, double power, ulong khz)
{
    for (uint i = 0; i < fgpu.n; i++) {
        fgpu.reading[i].util_gpu = util;
        fgpu.reading[i].power_w  = power;
        fgpu.reading[i].freq_gpu = khz;
        fgpu.reading[i].working  = util > 0;
    }
}

/* ============================================================== split */

TEST(test_equal_load_equal_split)
{
    gpu_setup(4);
    set_limit(1200, 80, 80, 80, 80);
    for (int i = 0; i < 4; i++)
        CHECK_EQ(fgpu.hw_cap[i], 300, "GPU%d gets 1200/4", i);
    for (int i = 0; i < 4; i++)
        CHECK_EQ(fake_stored[DOMAIN_GPU][i], 300, "GPU%d stored value matches the hardware", i);
}

TEST(test_proportional_split)
{
    gpu_setup(4);
    set_limit(1000, 100, 50, 25, 25);
    CHECK_EQ(fgpu.hw_cap[0], 400, "GPU0 (50%% of the load): 500 W clamped to its 400 W max");
    /* the other GPUs share the remaining 600 W by load (50/25/25) */
    CHECK_EQ(fgpu.hw_cap[1], 300, "GPU1: 300 W");
    CHECK_EQ(fgpu.hw_cap[2], 150, "GPU2: 150 W");
    CHECK_EQ(fgpu.hw_cap[3], 150, "GPU3: 150 W");
    CHECK(hw_sum() <= 1000, "sum within the domain limit (%u)", hw_sum());
    CHECK(hw_sum() >= 990, "the 100 W GPU0 cannot use is handed to the other busy GPUs (sum %u)", hw_sum());
}

TEST(test_min_clamp_when_limit_is_tight)
{
    gpu_setup(4);
    set_limit(300, 80, 80, 80, 80);
    for (int i = 0; i < 4; i++)
        CHECK_EQ(fgpu.hw_cap[i], 100, "GPU%d never below its 100 W minimum", i);
    /* 4 x 100 W > 300 W is physically unavoidable; the domain must say so */
    domain_status_t st;
    targets(1410000);
    readings(80, 100, 1410000);
    gpu_pc_thread_main(NULL);
    get_powercap_status(&st);
    CHECK(st.ok != PC_STATUS_RELEASE, "a domain over its limit because of minimums never offers power back");
}

TEST(test_idle_gpus_get_minimum)
{
    gpu_setup(4);
    set_limit(1000, 80, 80, 0, 0);
    CHECK(fgpu.hw_cap[2] == 100 && fgpu.hw_cap[3] == 100, "idle GPUs get their minimum");
    CHECK(fgpu.hw_cap[0] == 400 && fgpu.hw_cap[1] == 400, "busy GPUs share the rest (clamped to max)");
    CHECK(hw_sum() <= 1000, "sum within the limit (%u)", hw_sum());

    set_limit(800, 80, 80, 0, 0);
    CHECK(hw_sum() <= 800, "2 idle + 2 busy GPUs stay within an 800 W limit (sum %u)", hw_sum());
}

TEST(test_small_limit_underflow)
{
    gpu_setup(4);
    set_limit(100, 50, 0, 0, 0);
    CHECK(fgpu.hw_cap[0] <= 100, "one busy GPU on a 100 W domain limit is not given 400 W (got %u)",
          fgpu.hw_cap[0]);
}

TEST(test_all_idle)
{
    gpu_setup(4);
    set_limit(1000, 0, 0, 0, 0);
    for (int i = 0; i < 4; i++)
        CHECK_EQ(fgpu.hw_cap[i], 100, "all idle: GPU%d at its minimum", i);
    domain_status_t st;
    targets(1410000);
    readings(0, 30, 210000);
    gpu_pc_thread_main(NULL);
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_RELEASE, "all idle: the domain releases");
    CHECK_EQ(st.exceed, 1000 - 4 * 100, "... everything but the 100 W minimum each idle GPU is held at");
}

TEST(test_two_gpus)
{
    gpu_setup(2);
    set_limit(700, 90, 30, 0, 0);
    CHECK_EQ(fgpu.hw_cap[0], 400, "2 GPUs 90/30: GPU0 525 W clamped to 400");
    CHECK_EQ(fgpu.hw_cap[1], 300, "2 GPUs 90/30: GPU1 gets the 300 W GPU0 can't use");
    CHECK(hw_sum() <= 700, "sum within the limit");
}

TEST(test_unlimited_restores_max)
{
    gpu_setup(4);
    set_limit(1000, 80, 80, 80, 80);
    set_limit(POWER_CAP_UNLIMITED, 80, 80, 80, 80);
    for (int i = 0; i < 4; i++)
        CHECK_EQ(fgpu.hw_cap[i], 400, "unlimited: GPU%d back to its max", i);
    CHECK_EQ(fake_stored[DOMAIN_GPU][0], POWER_CAP_UNLIMITED, "stored as unlimited");
    domain_status_t st;
    CHECK_EQ(get_powercap_status(&st), 0, "unlimited: no status to report");
    CHECK_EQ(st.ok, PC_STATUS_OK, "... OK");
}

TEST(test_per_device_limits)
{
    gpu_setup(4);
    uint32_t lim[4] = {350, 300, POWER_CAP_UNLIMITED, 150};
    uint32_t util[4] = {80, 80, 80, 80};
    set_powercap_value(0, LEVEL_DEVICE, lim, util);
    CHECK(fgpu.hw_cap[0] == 350 && fgpu.hw_cap[1] == 300 && fgpu.hw_cap[3] == 150, "explicit per-GPU caps applied");
    CHECK_EQ(fgpu.hw_cap[2], 400, "per-device unlimited -> GPU max");
    CHECK_EQ(fake_stored[DOMAIN_GPU][2], 400, "stored as the max value that was applied");
    uint32_t back[4];
    get_powercap_value(0, back);
    CHECK(back[0] == 350 && back[3] == 150, "read back from the device API");
}

TEST(test_utilization_from_pmgt_layout)
{
    gpu_setup(4);
    /* powercap_mgt.c keeps utilization as uint32_t[] (current_util[]), the type gpu.c reads */
    uint32_t pmgt_util[4] = {80, 80, 80, 80};
    uint32_t limit        = 1200;
    set_powercap_value(0, LEVEL_NODE, &limit, pmgt_util);
    CHECK(fgpu.hw_cap[1] == 300 && fgpu.hw_cap[3] == 300,
          "4 equally busy GPUs get equal caps when utilization comes from pmgt (%u %u %u %u)", fgpu.hw_cap[0],
          fgpu.hw_cap[1], fgpu.hw_cap[2], fgpu.hw_cap[3]);
    set_new_utilization(pmgt_util);
    CHECK(fgpu.hw_cap[1] == 300, "... and through set_new_utilization (%u %u %u %u)", fgpu.hw_cap[0],
          fgpu.hw_cap[1], fgpu.hw_cap[2], fgpu.hw_cap[3]);
}

static void util_before_enable(void)
{
    fake_gpus_default(4);
    uint32_t util[4] = {80, 80, 80, 80};
    set_new_utilization(util); /* pmgt's util detection may fire before enable() */
}

TEST(test_new_utilization_before_enable)
{
    int sig = pc_crashes(util_before_enable);
    CHECK(sig == 0, "a utilization change before enable() is ignored, not a crash (signal %d = %s)", sig,
          sig ? strsignal(sig) : "none");
}

TEST(test_new_utilization_resplits)
{
    gpu_setup(4);
    set_limit(1200, 80, 80, 80, 80);
    uint32_t util[4] = {90, 90, 0, 0};
    set_new_utilization(util);
    CHECK(fgpu.hw_cap[2] == 100 && fgpu.hw_cap[3] == 100, "two GPUs went idle: minimum");
    CHECK(fgpu.hw_cap[0] == 400 && fgpu.hw_cap[1] == 400, "the busy ones get the rest");
    reset_powercap_value();
    CHECK_EQ(hw_sum(), 1000, "reset re-applies the last domain limit with the current utilization");
}

/* ============================================================= status */

TEST(test_status_no_target)
{
    gpu_setup(4);
    set_limit(1200, 80, 80, 80, 80);
    domain_status_t st;
    CHECK_EQ(get_powercap_status(&st), 0, "before any reading and with no target: nothing to decide");
    CHECK_EQ(st.ok, PC_STATUS_OK, "OK");
    CHECK_EQ(st.current_pc, 1200, "current_pc is the domain limit");

    /* No EARL target: the monitor assumes the nominal frequency */
    readings(80, 200, 1410000);
    gpu_pc_thread_main(NULL);
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_RELEASE, "no target, running at nominal with headroom: RELEASE");
}

TEST(test_status_release_at_target)
{
    gpu_setup(4);
    set_limit(1200, 80, 80, 80, 80);
    targets(1410000);
    readings(80, 200, 1410000);
    gpu_pc_thread_main(NULL);
    domain_status_t st;
    CHECK_EQ(get_powercap_status(&st), 1, "at target, 100 W under each cap: can release");
    CHECK_EQ(st.ok, PC_STATUS_RELEASE, "RELEASE");
    CHECK_EQ(st.exceed, 200, "releases half the headroom: 4 x 50 W");
    CHECK_EQ(st.stress, 0, "no stress at target");

    readings(80, 296, 1410000);
    gpu_pc_thread_main(NULL);
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_OK, "headroom under the 10 W margin: OK");
}

TEST(test_status_greedy_below_target)
{
    gpu_setup(4);
    set_limit(1200, 80, 80, 80, 80);
    targets(1410000);
    readings(80, 300, 1080000);
    gpu_pc_thread_main(NULL);
    domain_status_t st;
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_GREEDY, "running 2 P-states below target: GREEDY");
    CHECK_EQ(st.stress, 27, "stress = (1410-1080)/(1410-210) = 27");
    CHECK(st.requested > 0, "a greedy GPU domain requests something (got %u)", st.requested);
    /* 330 MHz below target x (400 - 100 W) / (1410 - 210 MHz) = 82 W per GPU */
    CHECK_EQ(st.requested, 4 * 82, "each throttled GPU asks what its clock gap costs, from its power range (got %u)",
             st.requested);

    set_status(PC_STATUS_RELEASE);
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_ASK_DEF, "throttled after releasing power: ASK_DEF");
}

TEST(test_status_greedy_over_power)
{
    gpu_setup(4);
    set_limit(1200, 80, 80, 80, 80);
    targets(1410000);
    readings(80, 320, 1410000);
    gpu_pc_thread_main(NULL);
    domain_status_t st;
    get_powercap_status(&st);
    CHECK_EQ(st.ok, PC_STATUS_GREEDY, "at target but drawing more than the cap: GREEDY");
    readings(80, 360, 1410000);
    gpu_pc_thread_main(NULL);
    get_powercap_status(&st);
    CHECK_EQ(st.requested, 4 * 60, "each GPU asks the 60 W it draws over its 300 W cap");
    readings(80, 500, 1410000);
    gpu_pc_thread_main(NULL);
    get_powercap_status(&st);
    CHECK_EQ(st.requested, 4 * 100, "... but never more than its headroom to the 400 W max");
}

TEST(test_status_above_target)
{
    gpu_setup(4);
    set_limit(1200, 80, 80, 80, 80);
    targets(1080000);
    readings(80, 200, 1410000);
    gpu_pc_thread_main(NULL);
    domain_status_t st;
    get_powercap_status(&st);
    CHECK(st.stress == 0, "GPUs faster than the target have no stress (got %u)", st.stress);
}

/* ============================================ internal redistribution */

TEST(test_monitor_moves_spare_power)
{
    gpu_setup(4);
    set_limit(1200, 80, 80, 80, 80);
    targets(1410000);
    readings(80, 250, 1410000);
    gpu_pc_thread_main(NULL); /* settle: first tick re-splits on the utilization change */
    int sets = fgpu.set_calls;

    /* GPU0 hits its cap, GPU1 has 50 W spare; same utilization */
    fgpu.reading[0].power_w = 300;
    fgpu.reading[1].power_w = 250;
    gpu_pc_thread_main(NULL);
    CHECK(fgpu.set_calls > sets && fgpu.hw_cap[0] > 300,
          "GPU0 at its cap gets GPU1's spare power on the device (caps %u %u)", fgpu.hw_cap[0], fgpu.hw_cap[1]);

    /* GPU0 over its cap (throttling) */
    fgpu.reading[0].power_w = 330;
    gpu_pc_thread_main(NULL);
    CHECK(fgpu.hw_cap[0] > 300, "GPU0 drawing 330 W on a 300 W cap gets spare power (cap %u)",
          fgpu.hw_cap[0]);
    CHECK(hw_sum() <= 1200, "device caps never exceed the domain limit (%u)", hw_sum());
}

int main(void)
{
    RUN_ISOLATED(test_equal_load_equal_split);
    RUN_ISOLATED(test_proportional_split);
    RUN_ISOLATED(test_min_clamp_when_limit_is_tight);
    RUN_ISOLATED(test_idle_gpus_get_minimum);
    RUN_ISOLATED(test_small_limit_underflow);
    RUN_ISOLATED(test_all_idle);
    RUN_ISOLATED(test_two_gpus);
    RUN_ISOLATED(test_unlimited_restores_max);
    RUN_ISOLATED(test_per_device_limits);
    RUN_ISOLATED(test_utilization_from_pmgt_layout);
    RUN_ISOLATED(test_new_utilization_before_enable);
    RUN_ISOLATED(test_new_utilization_resplits);
    RUN_ISOLATED(test_status_no_target);
    RUN_ISOLATED(test_status_release_at_target);
    RUN_ISOLATED(test_status_greedy_below_target);
    RUN_ISOLATED(test_status_greedy_over_power);
    RUN_ISOLATED(test_status_above_target);
    RUN_ISOLATED(test_monitor_moves_spare_power);
    return pc_summary("test_gpu_plugin");
}
