/*
 * test_monitor.c -- the optional powercap monitor of powercap.c (POWERCAP_MON)
 *
 * With POWERCAP_MON=1 powercap.c subscribes a periodic check that reads every
 * device cap back from the hardware and compares it with what the plugins
 * stored (powercap_verify_all_devices). econtrol drives it through
 * powercap_process_message(action, "monitor", ...): enable, disable, and
 * "set <relax_ms> <burst_ms>". This binary builds powercap.c with the monitor
 * on; everything else about powercap.c is in test_node.c.
 *
 * (Migrated from tests/test_powercap_monitor.c.)
 */
#include <common/config.h>
#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_status.h>

#include "fake_monitor.h"
#include "fake_pmgt.h"
#include "fake_powermon.h"
#include "pc_test.h"

extern int *ips;

static suscription_t *mon;

static void setup(void)
{
    fake_powermon_reset();
    fake_pmgt_reset();
    fake_pc_def = 1000;
    fake_pc_max = 1500;
    powercap_init();
    mon = fake_num_subs ? fake_subs[fake_num_subs - 1] : NULL;
}

TEST(test_monitor_subscribed_at_init)
{
    setup();
    CHECK(mon != NULL, "powercap_init subscribes the monitor");
    CHECK(mon && mon->time_relax == 10000 && mon->time_burst == 10000, "default period 10 s");
}

TEST(test_monitor_disable_enable)
{
    setup();
    powercap_process_message("disable", "monitor", NULL, 0, NULL);
    CHECK_EQ(fake_unregister_calls, 1, "disable unregisters the monitor");
    CHECK_EQ(fake_num_subs, 0, "... and it no longer runs");
    powercap_process_message("enable", "monitor", NULL, 0, NULL);
    CHECK_EQ(fake_num_subs, 1, "enable subscribes it again");
    powercap_process_message("enable", "monitor", NULL, 0, NULL);
    CHECK_EQ(fake_num_subs, 1, "enable twice does not subscribe twice");
}

TEST(test_monitor_set_time)
{
    setup();
    int32_t t[2] = {8000, 9000};
    powercap_process_message("set", "monitor", NULL, 2, t);
    CHECK(mon->time_relax == 8000 && mon->time_burst == 9000, "set 8000 9000 applied");
    CHECK_EQ(fake_num_subs, 1, "running monitor restarted with the new period");

    int32_t small[2] = {1000, 9000};
    powercap_process_message("set", "monitor", NULL, 2, small);
    CHECK(mon->time_relax == 8000, "periods under 5 s are rejected");

    int32_t one[1] = {7000};
    powercap_process_message("set", "monitor", NULL, 1, one);
    CHECK(mon->time_relax == 8000, "set with one value is rejected");
    CHECK_EQ(fake_pmgt_msg_calls, 0, "monitor messages never reach the domains");

    powercap_process_message("disable", "monitor", NULL, 0, NULL);
    int32_t t2[2] = {6000, 6000};
    powercap_process_message("set", "monitor", NULL, 2, t2);
    CHECK(mon->time_relax == 6000 && fake_num_subs == 0, "set while disabled changes the period, does not restart");
}

TEST(test_monitor_tick_verifies_devices)
{
    setup();
    /* init leaves every stored value UNLIMITED; make the hardware agree */
    for (uint d = DOMAIN_CPU; d <= DOMAIN_GPU; d++)
        for (uint i = 0; i < fake_pmgt_devices[d]; i++)
            fake_pmgt_hw_caps[d][i] = POWER_CAP_UNLIMITED;
    CHECK_EQ(mon->call_main(NULL), EAR_SUCCESS, "tick runs");
    CHECK_EQ(powercap_verify_all_devices(), EAR_SUCCESS, "hardware matches storage");
    fake_pmgt_hw_caps[DOMAIN_CPU][1] = 120;
    CHECK_EQ(powercap_verify_all_devices(), EAR_WARNING, "a changed CPU1 cap is caught");
}

TEST(test_storage_released_at_end)
{
    setup();
    powercap_set_stored_device_value(DOMAIN_CPU, 0, 100);
    powercap_end();
    CHECK_EQ(fake_pmgt_disabled, 1, "powercap_end disables the domains");
    CHECK_EQ(powercap_get_stored_device_value(DOMAIN_CPU, 0), 0, "storage freed: reads 0");
    CHECK_EQ(powercap_set_stored_device_value(DOMAIN_CPU, 0, 100), EAR_ERROR, "... and rejects writes");
}

int main(void)
{
    RUN_ISOLATED(test_monitor_subscribed_at_init);
    RUN_ISOLATED(test_monitor_disable_enable);
    RUN_ISOLATED(test_monitor_set_time);
    RUN_ISOLATED(test_monitor_tick_verifies_devices);
    RUN_ISOLATED(test_storage_released_at_end);
    return pc_summary("test_monitor");
}
