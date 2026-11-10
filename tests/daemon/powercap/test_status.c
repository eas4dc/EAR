/*
 * test_status.c -- unit tests for src/daemon/powercap/powercap_status.c
 *
 * powercap_status.c is the pure decision layer of the node powercap: given a
 * node_powercap_opt_t and a power reading it decides OK / GREEDY / RELEASE /
 * ASK_DEF, and it computes the "stress" figure that EARGM uses to rank greedy
 * nodes. It has no dependencies beyond headers, so it is linked as-is.
 */

#include <math.h>
#include <limits.h>

#include <common/config.h>
#include <common/types/powercap.h>
#include <daemon/powercap/powercap_status.h>

#include "pc_test.h"

static node_powercap_opt_t make_opt(uint32_t current_pc, uint32_t def, uint32_t last_t1)
{
    node_powercap_opt_t o;
    memset(&o, 0, sizeof(o));
    o.current_pc        = current_pc;
    o.baseline_powercap      = def;
    o.last_t1_allocated = last_t1;
    o.th_inc            = 10; /* same defaults as set_default_node_powercap_opt() */
    o.th_red            = 50;
    o.th_release        = 25;
    o.powercap_status   = PC_STATUS_OK;
    return o;
}

TEST(test_util_changed_edges)
{
    CHECK_EQ(util_changed(0, 0), 0, "0 -> 0 is not a change");
    CHECK_EQ(util_changed(10, 0), 1, "0 -> 10 is a change (idle to busy)");
    CHECK_EQ(util_changed(0, 10), 1, "10 -> 0 is a change (busy to idle)");
    CHECK_EQ(util_changed(120, 100), 0, "100 -> 120: 16.7%% of curr, below the 25%% threshold");
    CHECK_EQ(util_changed(140, 100), 1, "100 -> 140: 28.6%% of curr, above the threshold");
    CHECK_EQ(util_changed(80, 100), 0, "100 -> 80: 20%% of prev, below the threshold");
    CHECK_EQ(util_changed(70, 100), 1, "100 -> 70: 30%% of prev, above the threshold");
    CHECK_EQ(util_changed(75, 100), 0, "100 -> 75: exactly 25%% is not a change (strict >)");
    CHECK_EQ(util_changed(100, 75), 0, "75 -> 100: exactly 25%% of curr is not a change");
    CHECK_EQ(util_changed(400, 100), 1, "4 GPUs going from 1 busy to 4 busy is a change");
}

TEST(test_power_bands)
{
    node_powercap_opt_t o = make_opt(1000, 1000, 1000);

    /* more_power: distance to the cap below th_inc, or above the cap */
    CHECK_EQ(more_power(&o, 995), 1, "5 W from the cap: greedy band");
    CHECK_EQ(more_power(&o, 1005), 1, "over the cap: greedy band");
    CHECK_EQ(more_power(&o, 900), 0, "100 W from the cap: not greedy");

    /* free_power: distance >= th_red */
    CHECK_EQ(free_power(&o, 950), 1, "50 W below the cap: release band (>= th_red)");
    CHECK_EQ(free_power(&o, 951), 0, "49 W below the cap: not release");
    CHECK_EQ(free_power(&o, 1001), 0, "over the cap: never release");

    /* ok_power: strictly between th_inc and th_red */
    CHECK_EQ(ok_power(&o, 960), 1, "40 W below the cap: OK band");
    CHECK_EQ(ok_power(&o, 990), 0, "10 W below the cap: boundary, not OK band");
    CHECK_EQ(ok_power(&o, 950), 0, "50 W below the cap: boundary, not OK band");
}

TEST(test_compute_power_status)
{
    node_powercap_opt_t o = make_opt(1000, 1000, 1000);
    CHECK_EQ(compute_power_status(&o, 960), PC_STATUS_OK, "in the OK band -> OK");
    CHECK_EQ(compute_power_status(&o, 998), PC_STATUS_GREEDY, "at the cap -> GREEDY");
    CHECK_EQ(compute_power_status(&o, 1200), PC_STATUS_GREEDY, "over the cap -> GREEDY");
    CHECK_EQ(compute_power_status(&o, 300), PC_STATUS_RELEASE, "far below the cap -> RELEASE");
    CHECK_EQ(compute_power_status(&o, 990), PC_STATUS_OK, "exactly th_inc from the cap falls through to OK");

    o = make_opt(800, 1000, 800);
    CHECK_EQ(compute_power_status(&o, 100), PC_STATUS_ASK_DEF,
             "allocated below the default -> ASK_DEF, whatever the power");

    /* The whole band sweep must return one of the four legal states */
    int bad = 0;
    o       = make_opt(1000, 1000, 1000);
    for (uint p = 0; p < 3000; p += 7) {
        uint s = compute_power_status(&o, p);
        if (s != PC_STATUS_OK && s != PC_STATUS_GREEDY && s != PC_STATUS_RELEASE)
            bad++;
    }
    CHECK_EQ(bad, 0, "a 0..3000 W sweep only yields OK/GREEDY/RELEASE");
}

TEST(test_compute_next_status)
{
    node_powercap_opt_t o = make_opt(1000, 1000, 1000);
    /* RELEASE is kept only if the app runs at the frequency it asked for */
    CHECK_EQ(compute_next_status(&o, 300, 2000000, 2000000), PC_STATUS_RELEASE, "release and f == req -> RELEASE");
    CHECK_EQ(compute_next_status(&o, 300, 1800000, 2000000), PC_STATUS_OK, "release but f < req -> OK");
    /* GREEDY is kept only if the app runs below the requested frequency */
    CHECK_EQ(compute_next_status(&o, 1200, 1800000, 2000000), PC_STATUS_GREEDY, "greedy and f < req -> GREEDY");
    CHECK_EQ(compute_next_status(&o, 1200, 2000000, 2000000), PC_STATUS_OK, "greedy but f == req -> OK");
    /* OK is promoted to GREEDY when the app is being throttled */
    CHECK_EQ(compute_next_status(&o, 960, 1800000, 2000000), PC_STATUS_GREEDY, "ok but f < req -> GREEDY");
    o = make_opt(800, 1000, 800);
    CHECK_EQ(compute_next_status(&o, 960, 2000000, 2000000), PC_STATUS_ASK_DEF, "ASK_DEF passes through untouched");
}

TEST(test_stress)
{
    /* CPU frequencies in kHz: 1.0 .. 3.0 GHz */
    CHECK_EQ(powercap_get_stress(3000000, 1000000, 2500000, 2000000), 25, "target 2.5, running 2.0 -> 25");
    CHECK_EQ(powercap_get_stress(3000000, 1000000, 3000000, 1000000), 100, "target max, running min -> 100");
    CHECK_EQ(powercap_get_stress(3000000, 1000000, 2000000, 2000000), 0, "at target -> 0");
    CHECK_EQ(powercap_get_stress(2000000, 2000000, 2000000, 1000000), 0, "max == min -> 0 (no range)");
    /* GPU frequencies in MHz */
    CHECK_EQ(powercap_get_stress(1410, 210, 1410, 1110), 25, "GPU: target 1410, running 1110 -> 25");

    /*
     * A device running FASTER than its target (turbo, or an app asking for a
     * low frequency) has no stress. The subtraction is unsigned, so it wraps.
     * stress is truncated to uint8_t by every caller, which turns the wrapped
     * value into noise that EARGM then uses to rank greedy nodes.
     */
    uint s = powercap_get_stress(3000000, 1000000, 2000000, 2500000);
    CHECK(s == 0, "running above target yields stress 0 (got %u)", s);
    uint8_t s8 = (uint8_t) s;
    CHECK(s8 == 0, "uint8 stress seen by EARGM is 0 too (got %u: a plausible-looking wrong value)", s8);

    /* A zero frequency reading (device not read yet) must not exceed 100 */
    s = powercap_get_stress(3000000, 1000000, 3000000, 0);
    CHECK(s <= 100, "a 0 kHz reading keeps stress <= 100 (got %u)", s);

    /* Inverted range (min > max): the wrapped divisor happens to yield 0 */
    s = powercap_get_stress(1000000, 3000000, 2000000, 1500000);
    CHECK(s <= 100, "inverted max/min keeps stress <= 100 (got %u)", s);
}

TEST(test_is_powercap_on)
{
    node_powercap_opt_t o = make_opt(1000, 1000, 1000);
    CHECK_EQ(is_powercap_on(&o), 1, "status OK and a cap set -> on");
    o.powercap_status = PC_STATUS_ERROR;
    CHECK_EQ(is_powercap_on(&o), 0, "status ERROR -> off");
    o = make_opt(0, 1000, 1000);
    CHECK_EQ(is_powercap_on(&o), 0, "current_pc 0 -> off (0 means disabled)");
    o = make_opt(POWER_CAP_UNLIMITED, 1000, 1000);
    CHECK_EQ(is_powercap_on(&o), 1, "current_pc UNLIMITED (1) still counts as on");
    CHECK_EQ(get_powercap_allocated(&o), 1000, "allocated is last_t1_allocated");
    CHECK_EQ(get_powercapopt_value(&o), POWER_CAP_UNLIMITED, "value is current_pc");
}

TEST(test_status_to_str)
{
    char b[64];
    struct {
        uint s;
        const char *str;
    } map[] = {{PC_STATUS_OK, "OK"},       {PC_STATUS_GREEDY, "greedy"}, {PC_STATUS_RELEASE, "release"},
               {PC_STATUS_ASK_DEF, "ask_def"}, {PC_STATUS_IDLE, "idle"},   {PC_STATUS_STOP, "stop"},
               {PC_STATUS_START, "start"}, {PC_STATUS_RUN, "run"},       {PC_STATUS_ERROR, "undef"}};
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        powercap_status_to_str(map[i].s, b);
        CHECK(!strcmp(b, map[i].str), "status %u -> \"%s\"", map[i].s, map[i].str);
    }
}

/* common/types/powercap.c: device names used by econtrol per-device requests
 * (migrated from tests/test_powercap_mgt.c) */
TEST(test_device_enum)
{
    CHECK_EQ(get_device_enum("DRAM_0"), DRAM0, "DRAM_0");
    CHECK_EQ(get_device_enum("DRAM1"), DRAM1, "DRAM1");
    CHECK_EQ(get_device_enum("CPU_0"), CPU0, "CPU_0");
    CHECK_EQ(get_device_enum("PCK_1"), CPU1, "PCK_1 is CPU1");
    CHECK_EQ(get_device_enum("GPU_0"), GPUSTART, "GPU_0");
    CHECK_EQ(get_device_enum("GPU_1"), GPUSTART + 1, "GPU_1");
    CHECK_EQ(get_device_enum("GPU_3"), GPUSTART + 3, "GPU_3");
    CHECK_EQ(get_device_enum("GPU_x"), NO_TYPE, "GPU_x is invalid");
    CHECK_EQ(get_device_enum("INVALID"), NO_TYPE, "unknown name");
    /* The GPU offset is only added when the matching table row index happens
     * to equal GPUSTART (row 4, "GPU_"); the "GPU" row (13) returns GPU0 */
    CHECK(get_device_enum("GPU3") == GPUSTART + 3, "GPU3 (no underscore) is GPU 3 (got %d)",
          get_device_enum("GPU3"));
}

/* ======================================================= power estimator (#814) */

TEST(test_power_estimator)
{
    power_estimator_t e;
    /* a 100..400 W GPU with clocks 210..1410 MHz: 0.25 W/MHz */
    power_estimator_init(&e, 100, 400, 210000, 1410000);
    CHECK(fabs(e.w_per_khz - 0.00025) < 1e-9, "slope seeded from the device range (%.6f W/kHz)", e.w_per_khz);
    CHECK(fabs(power_estimator_at(&e, 300, 1080000, 1410000) - 382.5) < 0.01,
          "300 W at 1080 MHz -> 382.5 W at 1410 MHz (%.1f)", power_estimator_at(&e, 300, 1080000, 1410000));

    /* measured moves teach the real slope (0.2 W/MHz) */
    power_estimator_sample(&e, 300, 1080000);
    power_estimator_sample(&e, 340, 1280000);
    power_estimator_sample(&e, 320, 1180000);
    CHECK(e.w_per_khz < 0.00025 && e.w_per_khz > 0.0002, "slope moves towards the measured 0.2 W/MHz (%.6f)",
          e.w_per_khz);
    power_estimator_sample(&e, 900, 1190000); /* a load change, not a clock effect */
    CHECK(e.w_per_khz <= 4 * 0.00025, "a noisy sample can't push the slope past 4x the seed (%.6f)", e.w_per_khz);
    double before = e.w_per_khz;
    power_estimator_sample(&e, 950, 1191000); /* 1 MHz: noise */
    CHECK_EQ(e.w_per_khz, before, "moves under 1 W of expected change are ignored");

    /* opposite moves are a load change (e.g. under a cap): the slope is kept */
    power_estimator_init(&e, 100, 400, 210000, 1410000);
    power_estimator_sample(&e, 300, 1080000);
    power_estimator_sample(&e, 300, 980000); /* clock down, power held at the cap */
    CHECK(fabs(e.w_per_khz - 0.00025) < 1e-12, "clock down at constant power: slope kept (%.6f)", e.w_per_khz);
    power_estimator_sample(&e, 330, 880000); /* clock down, power up */
    CHECK(fabs(e.w_per_khz - 0.00025) < 1e-12, "clock down, power up: slope kept (%.6f)", e.w_per_khz);
    power_estimator_sample(&e, 310, 1080000); /* clock up, power down */
    CHECK(fabs(e.w_per_khz - 0.00025) < 1e-12, "clock up, power down: slope kept (%.6f)", e.w_per_khz);

    /* a memory-bound load: 0.03 W/MHz, under a quarter of the seed, is still learned */
    power_estimator_init(&e, 100, 400, 210000, 1410000);
    power_estimator_sample(&e, 200, 410000);
    for (int i = 0; i < 10; i++) {
        power_estimator_sample(&e, 230, 1410000);
        power_estimator_sample(&e, 200, 410000);
    }
    CHECK(e.w_per_khz < 0.00025 / 4 && e.w_per_khz > 0.000025, "low slope learned below seed/4 (%.6f W/kHz)",
          e.w_per_khz);
    CHECK(e.w_per_khz >= 0.00025 / 16, "... never below seed/16");

    power_estimator_t unknown;
    power_estimator_init(&unknown, 0, 0, 210000, 1410000);
    CHECK(power_estimator_at(&unknown, 300, 1080000, 1410000) == 300, "unknown range: no prediction beyond the power now");
    power_estimator_init(&unknown, 0, 400, 210000, 1410000);
    CHECK(unknown.w_per_khz > 0, "unknown minimum: seeded from 0..max");

    CHECK_EQ(powercap_device_request(400, 300, 82), 82, "request = the need");
    CHECK_EQ(powercap_device_request(400, 300, 500), 100, "... bounded by the headroom to the max");
    CHECK_EQ(powercap_device_request(400, 400, 82), 0, "... nothing at the max");
    CHECK_EQ(powercap_device_request(400, 300, 0), 0, "... nothing when nothing is needed");

    /* the hardware layer reports 0 for an unknown maximum; powercap takes the TDP */
    uint32_t wmax[3] = {250, 0, 0}, tdp[3] = {200, 200, 0};
    powercap_max_or_tdp(wmax, tdp, 3);
    CHECK(wmax[0] == 250 && wmax[1] == 200 && wmax[2] == 0,
          "reported max kept, unknown max -> TDP, neither -> 0 (%u %u %u)", wmax[0], wmax[1], wmax[2]);
}

int main(void)
{
    RUN_ISOLATED(test_power_estimator);
    RUN(test_util_changed_edges);
    RUN(test_power_bands);
    RUN(test_compute_power_status);
    RUN(test_compute_next_status);
    RUN(test_stress);
    RUN(test_is_powercap_on);
    RUN(test_status_to_str);
    RUN(test_device_enum);
    return pc_summary("test_status");
}
