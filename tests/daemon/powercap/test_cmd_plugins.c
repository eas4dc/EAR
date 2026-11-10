/*
 * test_cmd_plugins.c -- the command-driven powercap plugins:
 *   inm.c           node limit through ipmitool (Intel Node Manager)
 *   grace_hopper.c  module limit through nvidia-smi
 *   gpu_dummy.c     placeholder GPU domain
 *
 * Built once per plugin (-DPLUG_INM / -DPLUG_GH / -DPLUG_GPU_DUMMY). The
 * plugins are called exactly the way powercap_mgt.c calls them, through the
 * powercapsym_t function-pointer types, so any signature mismatch between a
 * plugin and the manager shows up as wrong behaviour here. execute() is
 * stubbed: the test sees the commands that would run on a node.
 */
#include <pthread.h>

#include <common/config.h>
#include <common/system/monitor.h>
#include <daemon/powercap/powercap_status_conf.h>

#include "pc_test.h"

/* --- the manager's view of a plugin (powercap_mgt.c:powercapsym_t) --- */
typedef state_t (*mgr_set_f)(uint pid, uint domain, uint32_t *limits, uint32_t *util);
typedef state_t (*mgr_get_f)(uint pid, uint32_t *powercap);
typedef uint (*mgr_status_f)(domain_status_t *status);

state_t enable(suscription_t *sus);
state_t disable();
void plugin_get_settings(domain_settings_t *s);
state_t get_powercap_value();
state_t set_powercap_value();
uint get_powercap_status();

static mgr_set_f mgr_set       = (mgr_set_f) set_powercap_value;
static mgr_get_f mgr_get       = (mgr_get_f) get_powercap_value;
#if !defined(PLUG_GPU_DUMMY) /* gpu_dummy has no status: pmgt skips it */
static mgr_status_f mgr_status = (mgr_status_f) get_powercap_status;

/* The manager calls get_powercap_status(domain_status_t *): a plugin that is
 * on target must report OK (not GREEDY) and offer nothing */
static void check_status_ok(uint32_t watts)
{
    domain_status_t st;
    memset(&st, 0xAB, sizeof(st));
    CHECK_EQ(mgr_status(&st), 0, "status: nothing to share");
    CHECK_EQ(st.ok, PC_STATUS_OK, "status OK, not GREEDY");
    CHECK(st.exceed == 0 && st.requested == 0, "no power offered or requested (exceed %u requested %u)", st.exceed,
          st.requested);
    CHECK_EQ(st.current_pc, watts, "current_pc is the applied limit");
}
#endif

/* --- stubs --- */
static char cmds[32][256];
static int ncmds;
int execute(char *cmd)
{
    if (ncmds < 32)
        snprintf(cmds[ncmds++], sizeof(cmds[0]), "%s", cmd);
    return EAR_SUCCESS;
}
ulong frequency_get_cpu_freq(uint cpu)
{
    return 2400000;
}

static int ran(const char *needle)
{
    for (int i = 0; i < ncmds; i++)
        if (strstr(cmds[i], needle))
            return 1;
    return 0;
}

static void apply(uint32_t watts)
{
    /* the manager passes a pointer to the value, plus utilization */
    static uint32_t v;
    static uint32_t util[8] = {80, 80, 80, 80};
    v                    = watts;
    mgr_set(0, LEVEL_NODE, &v, util);
}

#if defined(PLUG_INM)
/* ================================================================ inm.so */
TEST(test_enable)
{
    ncmds = 0;
    CHECK_EQ(enable(suscription()), EAR_SUCCESS, "enable");
    CHECK(ran("ipmitool raw 0x06 0x32 0 1"), "XCC bridge enabled");
    CHECK(ran("ipmitool raw 0x3a 0xc7 0x01"), "INM enabled");
    CHECK(ran("nm control enable global"), "NM policy control enabled");
}

TEST(test_set_limit)
{
    enable(suscription());
    ncmds = 0;
    apply(800);
    CHECK_EQ(ncmds, 1, "one ipmitool call per limit");
    CHECK(ran("power 800 "), "an 800 W node limit programs 'power 800' (ran: %s)", ncmds ? cmds[0] : "-");
    uint32_t v = 0;
    mgr_get(0, &v);
    CHECK(v == 800, "and reads back 800 W (got %u)", v);
}

TEST(test_unlimited_removes_policy)
{
    enable(suscription());
    ncmds = 0;
    static uint32_t one = POWER_CAP_UNLIMITED;
    uint32_t util[8]       = {0};
    /* inm reads the pointer as the value, so 'unlimited' is only recognised
     * if the pointer's low 32 bits happen to be 1: they never are */
    mgr_set(0, LEVEL_NODE, &one, util);
    CHECK(ran("nm policy remove policy_id 0"), "unlimited removes the NM policy (ran: %s)",
          ncmds ? cmds[0] : "-");
}

TEST(test_disable)
{
    enable(suscription());
    ncmds = 0;
    disable();
    CHECK(ran("nm policy remove policy_id 0") && ran("nm control disable global"),
          "disable removes the policy and NM policy control");
}

TEST(test_status_abi)
{
    enable(suscription());
    apply(800);
    check_status_ok(800);
}

int main(void)
{
    RUN_ISOLATED(test_enable);
    RUN_ISOLATED(test_set_limit);
    RUN_ISOLATED(test_unlimited_removes_policy);
    RUN_ISOLATED(test_disable);
    RUN_ISOLATED(test_status_abi);
    return pc_summary("test_inm_plugin");
}

#elif defined(PLUG_GH)
/* ======================================================= grace_hopper.so */
TEST(test_set_limit)
{
    enable(suscription());
    ncmds = 0;
    apply(600);
    CHECK(ran("nvidia-smi -pl 600"), "a 600 W limit runs 'nvidia-smi -pl 600 -sc 1' (%d commands run)", ncmds);
    uint32_t v = 0;
    mgr_get(0, &v);
    CHECK(v == 600, "and reads back 600 W (got %u)", v);
}

TEST(test_unlimited)
{
    enable(suscription());
    ncmds = 0;
    apply(POWER_CAP_UNLIMITED);
    CHECK(ran("nvidia-smi -pl 0"), "unlimited runs 'nvidia-smi -pl 0' (%d commands run)", ncmds);
}

TEST(test_per_device_refused)
{
    enable(suscription());
    static uint32_t v[4] = {300, 300, 300, 300};
    uint32_t util[8]        = {0};
    ncmds                = 0;
    mgr_set(0, LEVEL_DEVICE, v, util);
    CHECK_EQ(ncmds, 0, "per-device limits are not applied (no command runs)");
}

TEST(test_settings)
{
    domain_settings_t s;
    plugin_get_settings(&s);
    CHECK(s.node_ratio == 0.0f, "node_ratio 0 (the module limit covers the whole node)");
}

TEST(test_status_abi)
{
    enable(suscription());
    check_status_ok(0);
}

int main(void)
{
    RUN_ISOLATED(test_set_limit);
    RUN_ISOLATED(test_unlimited);
    RUN_ISOLATED(test_per_device_refused);
    RUN_ISOLATED(test_settings);
    RUN_ISOLATED(test_status_abi);
    return pc_summary("test_grace_hopper_plugin");
}

#elif defined(PLUG_GPU_DUMMY)
/* ========================================================== gpu_dummy.so */
TEST(test_set_limit)
{
    enable(suscription());
    apply(900);
    uint32_t v = 0;
    mgr_get(0, &v);
    CHECK(v == 900, "a 900 W GPU domain limit reads back 900 W (got %u: the pointer bits)", v);
}

TEST(test_settings)
{
    domain_settings_t s;
    plugin_get_settings(&s);
    CHECK(s.node_ratio == 0.0f && s.security_range == 0.0f, "settings all zero");
}

int main(void)
{
    RUN_ISOLATED(test_set_limit);
    RUN_ISOLATED(test_settings);
    return pc_summary("test_gpu_dummy_plugin");
}
#endif
