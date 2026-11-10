/*
 * test_powercap_plugin.c -- src/global_manager/plugins/powercap_plugin.c
 *
 * EARGM loads a list of powercap plugins ("a.so:b.so") and must call every
 * one of them; it used to call the first plugin once per loaded plugin.
 */
#include <global_manager/plugins/powercap_plugin.h>

#include "pc_test.h"

static int calls[2][3];

void plugin_record(int plugin, int fn)
{
    calls[plugin][fn]++;
}

TEST(test_every_plugin_is_called)
{
    CHECK_EQ(eargm_plugin_load(".", "fake_egm_a.so:fake_egm_b.so"), EAR_SUCCESS, "two plugins loaded");
    cluster_conf_t conf;
    eargm_plugin_init(&conf);
    CHECK(calls[0][0] == 1 && calls[1][0] == 1, "init: each plugin once (%d, %d)", calls[0][0], calls[1][0]);

    powercap_status_t st         = {0};
    powercap_opt_t opt           = {0};
    bool send                    = false;
    cluster_powercap_ctx_t ctx   = {0};
    state_t r                    = eargm_plugin_hard_powercap_decision(&st, &opt, &send, &ctx);
    CHECK(calls[0][1] == 1 && calls[1][1] == 1, "hard decision: each plugin once (%d, %d)", calls[0][1],
          calls[1][1]);
    CHECK_EQ(r, EAR_WARNING, "a plugin asking for idle power is not hidden by the others");

    power_check_t pw = {0};
    eargm_plugin_soft_powercap_decision(&pw);
    CHECK(calls[0][2] == 1 && calls[1][2] == 1, "soft decision: each plugin once (%d, %d)", calls[0][2],
          calls[1][2]);
}

int main(void)
{
    RUN_ISOLATED(test_every_plugin_is_called);
    return pc_summary("test_powercap_plugin");
}
