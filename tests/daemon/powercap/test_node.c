/*
 * test_node.c -- unit tests for src/daemon/powercap/powercap.c
 *
 * powercap.c is the node-level powercap: it owns node_powercap_opt_t
 * (default / allocated / current limit), the IDLE -> RUN -> IDLE life cycle,
 * and every entry point EARGM reaches through the remote API. Here it runs
 * against fake_pmgt (which records what would be pushed to the domains) and
 * fake_powermon, while eargm_sim plays the EARGM side of the protocol.
 *
 * Node under test: ear.conf default powercap 1000 W, max powercap 1500 W,
 * IP 10.0.0.1. Other nodes in greedy lists are 10.0.0.2 / 10.0.0.3.
 */
#include <pthread.h>

#include <common/config.h>
#include <common/types/event_type.h>
#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_status.h>

#include "eargm_sim.h"
#include "fake_pmgt.h"
#include "fake_powermon.h"
#include "pc_test.h"

extern node_powercap_opt_t my_pc_opt;
extern uint32_t current_mode;
extern int *ips;

#define ME    0x0a000001
#define OTHER 0x0a000002
#define DEF   1000
#define MAX   1500

static void node_setup(uint def, uint max)
{
    fake_powermon_reset();
    fake_pmgt_reset();
    fake_pc_def = def;
    fake_pc_max = max;
    ips[0]      = ME;
    powercap_init();
}

static void node_running(void)
{
    node_setup(DEF, MAX);
    fake_ctx_start(1, 2400000, 1410, 4);
    powercap_idle_to_run();
    powercap_new_job();
}

static int32_t ips_me_other[2] = {OTHER, ME};

/* ======================================================= init / life cycle */

TEST(test_init_applies_idle_cap)
{
    node_setup(DEF, MAX);
    CHECK_EQ(my_pc_opt.baseline_powercap, DEF, "default powercap comes from ear.conf");
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_IDLE, "node starts IDLE");
    CHECK_EQ(my_pc_opt.current_pc, DEF, "idle cap = default * EARD_POWERCAP_IDLE_PERC (1)");
    CHECK_EQ(fake_pmgt_last_limit(), DEF, "idle cap pushed to the domain layer");
    CHECK_EQ(fake_pmgt_calls[0].domain, DOMAIN_NODE, "... as a node-level limit");
    CHECK_EQ(my_pc_opt.released, 0, "nothing released at start");
    CHECK(is_powercap_on(&my_pc_opt), "powercap is on");
}

TEST(test_init_disabled_when_default_is_zero)
{
    node_setup(0, 0);
    CHECK_EQ(fake_pmgt_num_calls, 0, "powercap 0 in ear.conf: no limit applied");
    CHECK(!is_powercap_on(&my_pc_opt), "powercap is off");
    powercap_idle_to_run();
    int32_t extra[2] = {0, 100};
    eargm_send_opt(2, ips_me_other, extra, 50);
    CHECK_EQ(fake_pmgt_num_calls, 0, "idle_to_run and a greedy grant are ignored when off");
    CHECK_EQ(my_pc_opt.current_pc, 0, "cap stays 0 (disabled)");
}

TEST(test_init_fails_when_no_domain_loads)
{
    fake_powermon_reset();
    fake_pmgt_reset();
    fake_pc_def        = DEF;
    fake_pmgt_init_ret = EAR_ERROR;
    CHECK_EQ(powercap_init(), EAR_ERROR, "pmgt_init failure is reported");
    CHECK(!is_powercap_on(&my_pc_opt), "powercap stays off");
    powercap_idle_to_run();
    eargm_set_powerlimit(700);
    CHECK(fake_pmgt_num_calls == 0, "a node whose powercap failed to init ignores EARGM limits (%d pushes)",
          fake_pmgt_num_calls);
}

TEST(test_idle_to_run_with_default_power)
{
    node_setup(DEF, MAX);
    int before = fake_pmgt_num_calls;
    fake_ctx_start(1, 2400000, 1410, 4);
    powercap_idle_to_run();
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_OK, "IDLE -> OK when the default is already allocated");
    CHECK_EQ(my_pc_opt.current_pc, DEF, "cap unchanged");
    CHECK_EQ(fake_pmgt_num_calls, before, "no redundant push when the limit does not change");
    CHECK_EQ(fake_pmgt_idle_to_run, 1, "domains told idle -> run");
    CHECK_EQ(fake_pmgt_last_status, PC_STATUS_RUN, "domains status RUN");

    /* A second idle_to_run is an error path and must not corrupt the state */
    powercap_idle_to_run();
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_OK, "double idle_to_run keeps OK");
    CHECK_EQ(my_pc_opt.current_pc, DEF, "double idle_to_run keeps the cap");
}

TEST(test_run_to_idle_and_release_idle_power)
{
    node_running();
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 50;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    int32_t extra[2] = {0, 50};
    eargm_send_opt(2, ips_me_other, extra, 60);
    CHECK_EQ(my_pc_opt.current_pc, DEF + 50, "running with 50 W extra");

    powercap_end_job();
    fake_ctx_stop(1);
    powercap_run_to_idle();
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_IDLE, "back to IDLE");
    CHECK_EQ(my_pc_opt.current_pc, DEF, "idle cap re-applied");
    CHECK_EQ(my_pc_opt.released, 50, "the extra 50 W is marked as released");
    CHECK_EQ(fake_pmgt_run_to_idle, 1, "domains told run -> idle");

    eargm_get_status(&r, 1);
    CHECK_EQ(r.st.idle_nodes, 1, "status: one idle node");
    CHECK_EQ(r.st.released, 50, "status: 50 W released");
    /*
     * EARGM (cluster_hard_powercap) subtracts rel_power.released from
     * total_powercap after ear_cluster_release_idle_power. For that to be right
     * the node must still be counting the released watts in its allocation
     * until release_idle collects them.
     */
    CHECK(r.st.total_powercap - r.st.released == my_pc_opt.current_pc,
          "allocation - released == cap actually held (%u - %u vs %u): no double count", r.st.total_powercap,
          r.st.released, my_pc_opt.current_pc);
    eargm_reply_free(&r);

    uint32_t rel = eargm_release_idle();
    CHECK_EQ(rel, 50, "release_idle returns the released watts");
    CHECK_EQ(my_pc_opt.released, 0, "... and clears them");
    fake_event_t *ev = fake_last_event(RELEASE_POWER);
    CHECK(ev && ev->value == 50, "RELEASE_POWER event with 50 W");
    CHECK_EQ(eargm_release_idle(), 0, "a second release returns nothing");
}

TEST(test_release_idle_ignored_while_running)
{
    node_running();
    CHECK_EQ(eargm_release_idle(), 0, "running node releases nothing through release_idle");
    CHECK_EQ(my_pc_opt.current_pc, DEF, "cap unchanged");
}

TEST(test_power_per_domain_dispatch)
{
    node_setup(DEF, MAX);
    dom_power_t p = {.platform = 900, .cpu = 300, .dram = 50, .gpu = 400};
    powercap_set_power_per_domain(&p, 0, 2000000);
    CHECK_EQ(fake_pmgt_per_domain_last_st, PC_STATUS_IDLE, "idle node: domains rebalanced in IDLE mode");
    powercap_idle_to_run();
    powercap_set_power_per_domain(&p, 1, 2000000);
    CHECK_EQ(fake_pmgt_per_domain_last_st, PC_STATUS_RUN, "running node: domains rebalanced in RUN mode");
    int n = fake_pmgt_per_domain_calls;
    powercap_process_message("set", "manual", NULL, 0, NULL);
    powercap_set_power_per_domain(&p, 1, 2000000);
    CHECK_EQ(fake_pmgt_per_domain_calls, n, "manual mode: no automatic rebalancing");
}

/* ============================================================= greedy flow */

TEST(test_greedy_node_is_granted_extra)
{
    node_running();
    fake_node_power            = 998;
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 50;
    fake_pmgt_status.stress    = 40;

    eargm_reply_t r;
    eargm_get_status(&r, 1);
    CHECK_EQ(r.pmgt.status, PC_STATUS_GREEDY, "node reports GREEDY");
    CHECK_EQ(r.st.num_greedy, 1, "node registered in the greedy list");
    CHECK(r.st.num_greedy == 1 && r.st.greedy_nodes[0] == ME, "greedy list carries the node IP");
    CHECK(r.st.num_greedy == 1 && r.st.greedy_data[0].requested == 50, "requested 50 W");
    CHECK(r.st.num_greedy == 1 && r.st.greedy_data[0].stress == 40, "stress 40");
    CHECK(r.st.num_greedy == 1 && r.st.greedy_data[0].extra_power == 0, "no extra power yet");
    CHECK_EQ(r.st.total_powercap, DEF, "status: allocation 1000 W");
    CHECK_EQ(r.st.current_power, 998, "status: node power 998 W");
    CHECK_EQ(r.st.total_nodes, 1, "status: one node");
    fake_event_t *ev = fake_last_event(POWERCAP_VALUE);
    CHECK(ev && ev->value == DEF, "POWERCAP_VALUE event carries the current limit");
    eargm_reply_free(&r);

    int32_t extra[2] = {20, 50};
    CHECK_EQ(eargm_send_opt(2, ips_me_other, extra, 60), 1, "node finds itself at position 1");
    CHECK_EQ(my_pc_opt.last_t1_allocated, DEF + 50, "allocation grows by the grant");
    CHECK_EQ(my_pc_opt.current_pc, DEF + 50, "cap grows by the grant");
    CHECK_EQ(fake_pmgt_last_limit(), DEF + 50, "new cap pushed to the domains");
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_OK, "GREEDY -> OK once served");
    ev = fake_last_event(INC_POWERCAP);
    CHECK(ev && ev->value == 50, "INC_POWERCAP event with 50 W");
    CHECK_EQ(fake_ctx[1].settings.pc_opt.current_pc, DEF + 50, "running app sees the new cap");
    CHECK_EQ(fake_ctx[1].resched.force_rescheduling, 1, "... and is forced to reschedule");

    /*
     * Next T1: the node is OK and holds 50 W above its default. EARGM needs
     * that as extra_power to be able to reclaim it (reduce_allocation and the
     * stress plugin both work on greedy_data[].extra_power).
     */
    fake_pmgt_status.status    = PC_STATUS_OK;
    fake_pmgt_status.requested = 0;
    eargm_get_status(&r, 1);
    CHECK(r.pmgt.extra == 50, "next status reports the 50 W as extra power (got %u)", r.pmgt.extra);
    CHECK(r.st.num_greedy == 1, "an OK node with extra stays visible to EARGM (num_greedy %u)",
          r.st.num_greedy);
    CHECK(my_pc_opt.baseline_powercap == DEF, "node default stays at ear.conf value (got %u)",
          my_pc_opt.baseline_powercap);
    eargm_reply_free(&r);
}

TEST(test_eargm_reclaims_extra_from_ok_node)
{
    node_running();
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 50;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    int32_t grant[2] = {0, 50};
    eargm_send_opt(2, ips_me_other, grant, 60);

    fake_pmgt_status.status = PC_STATUS_OK;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    /* EARGM needs the power for a new job and takes the 50 W back */
    int32_t reclaim[2] = {0, -50};
    eargm_send_opt(2, ips_me_other, reclaim, 99);
    CHECK(my_pc_opt.current_pc == DEF, "negative extra returns the node to its default (cap %u)",
          my_pc_opt.current_pc);
    CHECK(fake_last_event(RED_POWERCAP) != NULL, "RED_POWERCAP event emitted");
}

TEST(test_greedy_grant_clamped_to_max_powercap)
{
    node_running();
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 255;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    int32_t extra[2] = {0, 800};
    eargm_send_opt(2, ips_me_other, extra, 60);
    CHECK_EQ(my_pc_opt.current_pc, MAX, "cap never exceeds the ear.conf max powercap");
    CHECK_EQ(my_pc_opt.last_t1_allocated, MAX, "allocation reported to EARGM is the clamped value");
    fake_event_t *ev = fake_last_event(INC_POWERCAP);
    CHECK(ev && ev->value == MAX - DEF, "INC_POWERCAP reports the 500 W actually granted (got %lld)",
          ev ? ev->value : -1);
}

TEST(test_greedy_node_not_trimmed_below_default)
{
    node_running();
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 50;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    int32_t grant[2] = {0, 50};
    eargm_send_opt(2, ips_me_other, grant, 60); /* the node holds 1050 W */
    fake_pmgt_status.status = PC_STATUS_GREEDY;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    int32_t extra[2] = {0, -100};
    eargm_send_opt(2, ips_me_other, extra, 99);
    CHECK_EQ(my_pc_opt.current_pc, DEF, "EARGM reclaims a greedy node's extra but stops at its default");
    fake_event_t *ev = fake_last_event(RED_POWERCAP);
    CHECK(ev && ev->value == 50, "RED_POWERCAP event with the 50 W reclaimed");
    eargm_send_opt(2, ips_me_other, extra, 99);
    CHECK_EQ(my_pc_opt.current_pc, DEF, "a node at its default is not trimmed below it (reduce_def does that)");
}

TEST(test_greedy_requested_saturates)
{
    node_running();
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 255;
    fake_pmgt_status.stress    = 100;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    CHECK(r.st.num_greedy == 1 && r.st.greedy_data[0].requested == 255, "requested 255 W survives the uint8 field");
    eargm_reply_free(&r);
}

TEST(test_node_not_in_greedy_list_ignores_grants)
{
    node_running();
    int32_t lst[1]   = {OTHER};
    int32_t extra[1] = {300};
    CHECK_EQ(eargm_send_opt(1, lst, extra, 50), -1, "node not in the list");
    CHECK_EQ(my_pc_opt.current_pc, DEF, "cap unchanged");
    CHECK_EQ(eargm_send_opt(0, NULL, NULL, 50), -1, "empty list");
    CHECK_EQ(my_pc_opt.current_pc, DEF, "cap unchanged");
}

TEST(test_release_status_ignores_grants)
{
    node_running();
    fake_pmgt_status.status = PC_STATUS_RELEASE;
    fake_pmgt_status.tbr    = 100;
    eargm_reply_t r;
    eargm_get_status(&r, 0);
    eargm_reply_free(&r);
    /* get_status sets a releasing node back to OK */
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_OK, "RELEASE is transient: node OK after reporting");
}

/* ============================================================ ASK_DEF flow */

TEST(test_ask_def_after_default_increase_while_running)
{
    node_running();
    eargm_increase_def(200);
    CHECK_EQ(my_pc_opt.baseline_powercap, DEF + 200, "EARGM raised the node default");
    fake_pmgt_status.status = PC_STATUS_OK;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    CHECK_EQ(r.pmgt.status, PC_STATUS_ASK_DEF, "allocated < default -> ASK_DEF");
    CHECK_EQ(r.st.requested, 200, "status: 200 W requested to reach the default");
    CHECK_EQ(r.st.num_greedy, 0, "ASK_DEF is not a greedy entry");
    eargm_reply_free(&r);

    /* EARGM grants default power by sending opts without the node in them */
    int32_t lst[1]   = {OTHER};
    int32_t extra[1] = {10};
    eargm_send_opt(1, lst, extra, 70);
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_OK, "ASK_DEF -> OK");
    CHECK_EQ(my_pc_opt.current_pc, DEF + 200, "cap raised to the new default");
    CHECK_EQ(my_pc_opt.requested, 0, "request cleared");
    CHECK(fake_last_event(SET_ASK_DEF) != NULL, "SET_ASK_DEF event");
}

TEST(test_ask_def_request_survives_idle_to_run)
{
    node_setup(DEF, MAX);
    eargm_increase_def(200); /* while idle */
    powercap_idle_to_run();
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_ASK_DEF, "IDLE -> ASK_DEF when the default is not allocated");
    CHECK_EQ(my_pc_opt.requested, 200, "200 W pending");
    CHECK_EQ(my_pc_opt.current_pc, DEF, "runs with what it has meanwhile");
    fake_pmgt_status.status = PC_STATUS_OK;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    CHECK(r.pmgt.status == PC_STATUS_ASK_DEF,
          "EARGM still sees ASK_DEF (status %u)", r.pmgt.status);
    CHECK(r.st.requested == 200, "EARGM sees the 200 W request (got %u)", r.st.requested);
    eargm_reply_free(&r);
}

TEST(test_ask_def_node_in_greedy_list)
{
    node_running();
    eargm_increase_def(200);
    fake_pmgt_status.status = PC_STATUS_OK;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_ASK_DEF, "node asks for its new default");
    /* EARGM lists it among the greedy nodes (e.g. another EARGM's list) with 30 W */
    int32_t extra[2] = {0, 30};
    eargm_send_opt(2, ips_me_other, extra, 60);
    CHECK_EQ(my_pc_opt.current_pc, DEF + 200 + 30, "listed while ASK_DEF: default granted, plus the extra (cap %u)",
             my_pc_opt.current_pc);
    CHECK_EQ(my_pc_opt.powercap_status, PC_STATUS_OK, "ASK_DEF -> OK");
    CHECK(fake_last_event(SET_ASK_DEF) != NULL, "default granted: SET_ASK_DEF reported");
    fake_event_t *inc = fake_last_event(INC_POWERCAP);
    CHECK(inc != NULL && inc->value == 30, "... and the extra: INC_POWERCAP %lld W", inc ? (long long) inc->value : -1);
}

/* EARL drops its signature on a rescheduling: forced only for significant cap changes */
TEST(test_resched_only_on_significant_cap_change)
{
    node_running();
    int32_t extra[2] = {0, 30};
    fake_ctx[1].resched.force_rescheduling = 0;
    my_pc_opt.powercap_status              = PC_STATUS_GREEDY;
    eargm_send_opt(2, ips_me_other, extra, 60);
    CHECK_EQ(my_pc_opt.current_pc, DEF + 30, "30 W granted (3%%)");
    CHECK_EQ(fake_ctx[1].resched.force_rescheduling, 0, "a 3%% change is copied without a rescheduling");
    CHECK_EQ(fake_ctx[1].settings.pc_opt.current_pc, DEF + 30, "... but the job sees the new cap");
    int32_t more[2] = {0, 40};
    my_pc_opt.powercap_status = PC_STATUS_GREEDY;
    eargm_send_opt(2, ips_me_other, more, 60);
    CHECK_EQ(my_pc_opt.current_pc, DEF + 70, "40 W more: 7%% from the last rescheduling");
    CHECK_EQ(fake_ctx[1].resched.force_rescheduling, 1, "small changes add up: rescheduling forced");
}

/* ============================================================ RELEASE flow */

TEST(test_running_node_releases_power)
{
    node_running();
    fake_node_power         = 600;
    fake_pmgt_status.status = PC_STATUS_RELEASE;
    fake_pmgt_status.tbr    = 150;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    CHECK_EQ(r.pmgt.status, PC_STATUS_RELEASE, "node reports RELEASE");
    CHECK_EQ(my_pc_opt.current_pc, DEF - 150, "cap lowered by the released watts");
    CHECK_EQ(my_pc_opt.last_t1_allocated, DEF - 150, "allocation lowered too");
    CHECK_EQ(fake_pmgt_last_limit(), DEF - 150, "lower cap pushed to the domains");
    CHECK_EQ(r.st.total_powercap, DEF - 150, "EARGM sees the lower allocation");
    CHECK_EQ(my_pc_opt.baseline_powercap, DEF, "default untouched");
    eargm_reply_free(&r);

    /* Next T1 it asks the default back: release is a per-period loan */
    fake_pmgt_status.status = PC_STATUS_OK;
    fake_pmgt_status.tbr    = 0;
    eargm_get_status(&r, 1);
    CHECK_EQ(r.pmgt.status, PC_STATUS_ASK_DEF, "next status asks the default back");
    CHECK_EQ(r.st.requested, 150, "requesting the 150 W released");
    eargm_reply_free(&r);
    eargm_send_opt(0, NULL, NULL, 50);
    CHECK_EQ(my_pc_opt.current_pc, DEF, "default restored");
}

TEST(test_release_only_when_eargm_asks)
{
    node_running();
    fake_pmgt_status.status = PC_STATUS_RELEASE;
    fake_pmgt_status.tbr    = 150;
    eargm_reply_t r;
    eargm_get_status(&r, 0); /* e.g. an econtrol status query */
    CHECK_EQ(my_pc_opt.current_pc, DEF, "release_power=0: nothing released");
    CHECK_EQ(my_pc_opt.baseline_powercap, DEF, "default untouched");
    eargm_reply_free(&r);
}

static void release_everything(void)
{
    node_running();
    fake_pmgt_status.status = PC_STATUS_RELEASE;
    fake_pmgt_status.tbr    = DEF;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
}

TEST(test_release_more_than_cap)
{
    node_running();
    fake_pmgt_status.status = PC_STATUS_RELEASE;
    fake_pmgt_status.tbr    = 1200; /* domains overestimate what they can spare */
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    CHECK(my_pc_opt.current_pc <= DEF && my_pc_opt.current_pc > 0,
          "releasing more than the cap leaves a sane cap (got %u)", my_pc_opt.current_pc);
    eargm_reply_free(&r);

    release_everything();
    CHECK(is_powercap_on(&my_pc_opt), "releasing the whole cap must not switch the powercap off (cap %u)",
          my_pc_opt.current_pc);
}

TEST(test_run_to_idle_after_release)
{
    node_running();
    fake_pmgt_status.status = PC_STATUS_RELEASE;
    fake_pmgt_status.tbr    = 150;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    /* job ends before EARGM gives the default back */
    powercap_run_to_idle();
    CHECK(my_pc_opt.released <= DEF, "released after a run below the idle cap is sane (got %u)",
          my_pc_opt.released);
    uint32_t rel = eargm_release_idle();
    CHECK(rel <= DEF, "release_idle hands EARGM a sane value (got %u W)", rel);
}

/* ================================================ soft powercap / defaults */

TEST(test_soft_powercap_limits)
{
    node_running();
    eargm_set_powerlimit(700);
    CHECK_EQ(my_pc_opt.current_pc, 700, "soft powercap applied");
    CHECK_EQ(fake_pmgt_last_limit(), 700, "... and pushed");
    eargm_set_powerlimit(5000);
    CHECK_EQ(my_pc_opt.current_pc, MAX, "a limit above max powercap is clamped to it");
    eargm_set_powerlimit(POWER_CAP_UNLIMITED);
    CHECK_EQ(my_pc_opt.current_pc, POWER_CAP_UNLIMITED, "EARGM 'unlimited' (1) is applied as is");
    CHECK_EQ(fake_pmgt_last_limit(), POWER_CAP_UNLIMITED, "... and pushed as 1 to the domains");
    CHECK(is_powercap_on(&my_pc_opt), "powercap still on (unlimited is a value, not off)");
    eargm_set_powerlimit(900);
    CHECK_EQ(my_pc_opt.current_pc, 900, "a limit after unlimited is applied again");
    int n = fake_pmgt_num_calls;
    eargm_set_powerlimit(900);
    CHECK_EQ(fake_pmgt_num_calls, n, "same limit twice: not pushed twice");
}

TEST(test_soft_powercap_survives_job_end)
{
    node_running();
    eargm_set_powerlimit(700); /* cluster over its soft limit */
    powercap_end_job();
    powercap_run_to_idle();
    CHECK(my_pc_opt.current_pc <= 700, "after the job ends the node stays under EARGM's 700 W (cap %u)",
          my_pc_opt.current_pc);
}

TEST(test_soft_powercap_keeps_accounting)
{
    node_running();
    eargm_set_powerlimit(700);
    CHECK_EQ(my_pc_opt.baseline_powercap, DEF, "soft limit does not change the default");
    CHECK_EQ(my_pc_opt.last_t1_allocated, DEF, "... nor the allocation");
    fake_pmgt_status.status = PC_STATUS_OK;
    eargm_reply_t r;
    eargm_get_status(&r, 0); /* soft-mode EARGM never asks nodes to release power */
    CHECK(r.pmgt.status != PC_STATUS_ASK_DEF, "a soft-capped node does not ask for its default (status %u)",
          r.pmgt.status);
    CHECK_EQ(my_pc_opt.current_pc, 700, "a status read keeps the soft limit (cap %u)", my_pc_opt.current_pc);
    eargm_reply_free(&r);
    eargm_reset_def(); /* EARGM reload lowering its budget, in soft mode */
    CHECK_EQ(my_pc_opt.current_pc, 700, "a reload reset keeps the soft limit (cap %u)", my_pc_opt.current_pc);
    eargm_set_powerlimit(POWER_CAP_UNLIMITED);
    powercap_end_job();
    powercap_run_to_idle();
    CHECK_EQ(my_pc_opt.current_pc, POWER_CAP_UNLIMITED, "soft 'unlimited' holds across job end (cap %u)",
             my_pc_opt.current_pc);
}

TEST(test_status_query_keeps_allocation_under_soft_limit)
{
    node_running();
    eargm_set_powerlimit(700);
    fake_pmgt_status.status = PC_STATUS_RELEASE;
    fake_pmgt_status.tbr    = 100;
    eargm_reply_t r;
    eargm_get_status(&r, 0); /* e.g. econtrol: a RELEASE status read without releasing */
    eargm_reply_free(&r);
    CHECK_EQ(my_pc_opt.last_t1_allocated, DEF, "the soft limit is not written into the allocation (%u)",
             my_pc_opt.last_t1_allocated);
    CHECK_EQ(my_pc_opt.current_pc, 700, "the soft limit still applies (cap %u)", my_pc_opt.current_pc);
}

/* EARGM restarted in hard mode after soft mode, and the other way round */
TEST(test_eargm_switches_soft_and_hard)
{
    node_running();
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 50;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    int32_t grant[2] = {0, 50};
    eargm_send_opt(2, ips_me_other, grant, 60); /* hard mode: the node holds 1050 W */

    eargm_set_powerlimit(700); /* restarted in soft mode */
    CHECK_EQ(my_pc_opt.current_pc, 700, "hard -> soft: the soft limit applies (cap %u)", my_pc_opt.current_pc);
    CHECK_EQ(my_pc_opt.last_t1_allocated, DEF + 50, "... the allocation is kept");

    fake_pmgt_status.status = PC_STATUS_OK;
    eargm_get_status(&r, 1); /* restarted in hard mode: its first status may release power */
    eargm_reply_free(&r);
    CHECK_EQ(my_pc_opt.current_pc, DEF + 50, "soft -> hard: back to the 1050 W the node holds (cap %u)",
             my_pc_opt.current_pc);

    eargm_set_powerlimit(600);
    eargm_send_opt(0, NULL, NULL, 50); /* hard options alone also mean hard mode */
    CHECK_EQ(my_pc_opt.current_pc, DEF + 50, "hard options drop the soft limit too (cap %u)", my_pc_opt.current_pc);

    eargm_set_powerlimit(600);
    powercap_end_job();
    powercap_run_to_idle();
    eargm_release_idle(); /* idle release: hard mode */
    CHECK_EQ(my_pc_opt.current_pc, DEF, "idle node back in hard mode: idle cap (cap %u)", my_pc_opt.current_pc);
    CHECK_EQ(my_pc_opt.last_t1_allocated, DEF, "... and the released extra leaves the allocation (%u)",
             my_pc_opt.last_t1_allocated);
}

extern uint32_t fake_hw_min, fake_hw_max;

TEST(test_floor_is_node_working_minimum)
{
    fake_min_working = 400; /* ear.conf MIN_WORKING_POWER */
    node_running();
    eargm_set_powerlimit(200);
    CHECK_EQ(my_pc_opt.current_pc, 400, "a soft limit under MIN_WORKING_POWER applies (and reports) the minimum");
    eargm_set_powerlimit(POWER_CAP_UNLIMITED);
    CHECK_EQ(my_pc_opt.current_pc, POWER_CAP_UNLIMITED, "unlimited is not raised");
    fake_pmgt_status.status = PC_STATUS_RELEASE;
    fake_pmgt_status.tbr    = 900;
    eargm_set_powerlimit(1000);
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    CHECK_EQ(my_pc_opt.current_pc, 400, "a release stops at the working minimum");
    fake_min_working = 0;
}

TEST(test_default_raised_to_hardware_minimum)
{
    fake_hw_min = 1200; /* devices can't be capped under 1200 W */
    node_setup(DEF, MAX);
    CHECK_EQ(my_pc_opt.baseline_powercap, 1200, "a 1000 W default under the 1200 W hardware minimum is raised");
    CHECK_EQ(my_pc_opt.current_pc, 1200, "... and applied");
    fake_hw_min = 0;
}

TEST(test_default_reduce_increase_reset)
{
    node_running();
    eargm_reduce_def(200);
    CHECK_EQ(my_pc_opt.baseline_powercap, DEF - 200, "reduce 200 W");
    eargm_reduce_def(5000);
    CHECK_EQ(my_pc_opt.baseline_powercap, DEF - 200, "reduce more than the default is ignored");
    eargm_increase_def(300);
    CHECK_EQ(my_pc_opt.baseline_powercap, DEF + 100, "increase 300 W");
    eargm_reset_def();
    CHECK_EQ(my_pc_opt.current_pc, DEF + 100, "reset applies the current default");

    eargm_reduce_def(my_pc_opt.baseline_powercap); /* reduce to exactly 0 */
    eargm_reset_def();
    CHECK(is_powercap_on(&my_pc_opt), "reducing the default to 0 then resetting keeps the node capped (cap %u)",
          my_pc_opt.current_pc);

    node_running();
    eargm_increase_def(UINT32_MAX - 10);
    CHECK(my_pc_opt.baseline_powercap >= DEF, "increasing the default saturates instead of wrapping (got %u)",
          my_pc_opt.baseline_powercap);
}

TEST(test_reset_after_grant_returns_to_conf_default)
{
    node_running();
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 80;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    int32_t extra[2] = {0, 80};
    eargm_send_opt(2, ips_me_other, extra, 60);
    eargm_reset_def();
    CHECK(my_pc_opt.current_pc == DEF, "EARGM reset brings the node back to 1000 W (cap %u)",
          my_pc_opt.current_pc);
}

TEST(test_cluster_near_limit_resets_on_idle)
{
    node_running();
    int32_t lst[1]   = {OTHER};
    int32_t extra[1] = {0};
    eargm_send_opt(1, lst, extra, 95);
    eargm_reduce_def(100);
    powercap_run_to_idle();
    /* last_cluster_perc > 90: run_to_idle re-applies the (reduced) default */
    CHECK(my_pc_opt.current_pc == DEF - 100,
          "cluster at 95%%: node falls back to its reduced default (900) when idle (cap %u)", my_pc_opt.current_pc);
}

TEST(test_next_job_keeps_extra_not_yet_released)
{
    node_running();
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 50;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    int32_t grant[2] = {0, 50};
    eargm_send_opt(2, ips_me_other, grant, 60);
    powercap_end_job();
    powercap_run_to_idle();
    CHECK_EQ(my_pc_opt.released, 50, "job end: 50 W extra marked as released");
    powercap_idle_to_run(); /* next job before EARGM collected it */
    CHECK_EQ(my_pc_opt.current_pc, DEF + 50, "the next job keeps the 1050 W the node holds (cap %u)",
             my_pc_opt.current_pc);
    CHECK_EQ(my_pc_opt.released, 0, "... nothing is pending release any more");
    fake_pmgt_status.status = PC_STATUS_OK;
    eargm_get_status(&r, 1);
    CHECK(r.st.num_greedy == 1 && r.st.greedy_data[0].extra_power == 50, "and EARGM sees the 50 W as extra (%u)",
          r.st.num_greedy ? r.st.greedy_data[0].extra_power : 0);
    eargm_reply_free(&r);
}

TEST(test_reduced_default_survives_idle_to_run)
{
    node_setup(DEF, MAX);
    eargm_reduce_def(100); /* EARGM shrinks this node's share while it is idle */
    powercap_idle_to_run();
    CHECK_EQ(my_pc_opt.baseline_powercap, DEF - 100, "the reduced default is kept (def %u)",
             my_pc_opt.baseline_powercap);
    CHECK_EQ(my_pc_opt.current_pc, DEF, "the node keeps the power it holds (cap %u)", my_pc_opt.current_pc);
    fake_pmgt_status.status = PC_STATUS_OK;
    eargm_reply_t r;
    eargm_get_status(&r, 1);
    CHECK(r.st.num_greedy == 1 && r.st.greedy_data[0].extra_power == 100,
          "the 100 W above the reduced default are reported to EARGM as extra (%u)",
          r.st.num_greedy ? r.st.greedy_data[0].extra_power : 0);
    eargm_reply_free(&r);
    int32_t reclaim[2] = {0, -100};
    eargm_send_opt(2, ips_me_other, reclaim, 99);
    CHECK_EQ(my_pc_opt.current_pc, DEF - 100, "EARGM reclaims them when it needs them (cap %u)",
             my_pc_opt.current_pc);
}

TEST(test_unlimited_node_ignores_eargm)
{
    node_setup(POWER_CAP_UNLIMITED, MAX);
    int n = fake_pmgt_num_calls;
    powercap_idle_to_run();
    int32_t extra[2] = {0, 100};
    eargm_send_opt(2, ips_me_other, extra, 50);
    CHECK_EQ(fake_pmgt_num_calls, n, "powercap unlimited in ear.conf: EARGM opts ignored");
}

/* ============================================================ manual mode */

TEST(test_manual_mode)
{
    node_running();
    powercap_process_message(NULL, "manual", NULL, 0, NULL);
    CHECK_EQ(current_mode, PC_MODE_MANUAL, "mode manual");
    int n = fake_pmgt_num_calls;
    eargm_set_powerlimit(800);
    CHECK_EQ(my_pc_opt.current_pc, 800, "node accounting follows EARGM in manual mode");
    CHECK_EQ(fake_pmgt_num_calls, n, "... but nothing is pushed to the domains");

    int32_t v[2] = {200, 300};
    powercap_process_message("set", NULL, "domain", 2, v);
    CHECK_EQ(fake_pmgt_msg_calls, 1, "set/domain forwarded to pmgt");
    CHECK(!strcmp(fake_pmgt_msg_level, "domain") && fake_pmgt_msg_num == 2, "... with level and values");

    powercap_process_message("deactivate", "manual", NULL, 0, NULL);
    CHECK_EQ(current_mode, PC_MODE_AUTO, "deactivate -> auto");
    CHECK(fake_pmgt_num_calls > n, "leaving manual mode re-applies the node cap (800 W)");

    powercap_process_message("set", NULL, "cpu", 1, v);
    CHECK_EQ(current_mode, PC_MODE_MANUAL, "level cpu switches to manual");
    powercap_process_message("set", NULL, "node", 1, v);
    CHECK_EQ(current_mode, PC_MODE_AUTO, "level node switches back to auto");
    powercap_process_message("set", "monitor", NULL, 1, v);
    CHECK_EQ(fake_pmgt_msg_calls, 3, "monitor mode with 1 value is not forwarded as a limit");
    powercap_process_message("get", NULL, "gpu", 1, v);
    CHECK_EQ(fake_pmgt_msg_calls, 3, "non-set actions are not forwarded");
    CHECK_EQ(current_mode, PC_MODE_MANUAL, "... but still switch the mode (level gpu)");
}

/* ===================================================== aggregation / misc */

TEST(test_status_aggregates_over_tree)
{
    node_running();
    fake_node_power = 870;
    powercap_status_t st;
    pmgt_status_t p;
    memset(&st, 0, sizeof(st));
    /* As if two children had already filled the struct */
    st.total_nodes    = 2;
    st.current_power  = 1500;
    st.total_powercap = 2000;
    powercap_get_status(&st, &p, 1);
    CHECK_EQ(st.total_nodes, 3, "node count accumulates");
    CHECK_EQ(st.current_power, 2370, "power accumulates");
    CHECK_EQ(st.total_powercap, 3000, "allocation accumulates");
}

TEST(test_device_storage_and_verification)
{
    node_setup(DEF, MAX);
    for (uint d = DOMAIN_CPU; d <= DOMAIN_GPU; d++)
        for (uint i = 0; i < fake_pmgt_devices[d]; i++)
            CHECK_EQ(powercap_get_stored_device_value(d, i), POWER_CAP_UNLIMITED, "domain %u dev %u starts unlimited",
                     d, i);
    CHECK_EQ(powercap_set_stored_device_value(DOMAIN_CPU, 2, 100), EAR_ERROR, "CPU device out of range rejected");
    CHECK_EQ(powercap_set_stored_device_value(DOMAIN_NODE, 0, 100), EAR_ERROR, "node domain has no devices");
    CHECK_EQ(powercap_set_stored_device_value(7, 0, 100), EAR_ERROR, "invalid domain rejected");
    CHECK_EQ(powercap_get_stored_device_value(DOMAIN_GPU, 9), 0, "GPU out of range reads 0");

    powercap_update_all_device_storage(DOMAIN_GPU, 250);
    for (uint i = 0; i < 4; i++)
        fake_pmgt_hw_caps[DOMAIN_GPU][i] = 250;
    powercap_set_stored_device_value(DOMAIN_CPU, 0, 180);
    powercap_set_stored_device_value(DOMAIN_CPU, 1, 180);
    fake_pmgt_hw_caps[DOMAIN_CPU][0] = 180;
    fake_pmgt_hw_caps[DOMAIN_CPU][1] = 180;
    powercap_update_all_device_storage(DOMAIN_DRAM, 30);
    fake_pmgt_hw_caps[DOMAIN_DRAM][0] = 30;
    fake_pmgt_hw_caps[DOMAIN_DRAM][1] = 30;
    CHECK_EQ(powercap_verify_all_devices(), EAR_SUCCESS, "stored == hardware -> SUCCESS");

    fake_pmgt_hw_caps[DOMAIN_GPU][3] = 300; /* someone changed GPU3 behind EAR's back */
    CHECK_EQ(powercap_verify_device_values(DOMAIN_GPU), EAR_WARNING, "GPU mismatch detected");
    CHECK_EQ(powercap_verify_all_devices(), EAR_WARNING, "... and reported by verify_all");
    fake_pmgt_hw_caps[DOMAIN_GPU][3] = 250;

    fake_pmgt_hw_caps[DOMAIN_CPU][1] = 0; /* failed read */
    CHECK(powercap_verify_device_values(DOMAIN_CPU) != EAR_SUCCESS,
          "a device that cannot be read back is not reported as verified");
    CHECK_EQ(powercap_verify_device_values(9), EAR_ERROR, "invalid domain");
}

/* =============================================================== threads */

static volatile int stop_threads = 0;

static void *eargm_thread(void *arg)
{
    int32_t extra[2] = {0, 20};
    while (!stop_threads) {
        eargm_reply_t r;
        eargm_get_status(&r, 1);
        eargm_reply_free(&r);
        eargm_send_opt(2, ips_me_other, extra, 60);
        extra[1] = -extra[1];
        usleep(50);
    }
    return NULL;
}

static void *pmon_thread(void *arg)
{
    dom_power_t p = {.platform = 900, .cpu = 300, .dram = 50, .gpu = 400};
    while (!stop_threads) {
        powercap_set_power_per_domain(&p, 1, 2000000);
        usleep(50);
    }
    return NULL;
}

static void *jobs_thread(void *arg)
{
    for (int i = 0; i < 1000 && !stop_threads; i++) {
        powercap_idle_to_run();
        powercap_run_to_idle();
    }
    return NULL;
}

TEST(test_concurrent_eargm_and_monitor)
{
    node_setup(DEF, MAX);
    fake_pmgt_status.status    = PC_STATUS_GREEDY;
    fake_pmgt_status.requested = 20;
    pthread_t a, b, c;
    pthread_create(&a, NULL, eargm_thread, NULL);
    pthread_create(&b, NULL, pmon_thread, NULL);
    pthread_create(&c, NULL, jobs_thread, NULL);
    pthread_join(c, NULL);
    stop_threads = 1;
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    CHECK(1, "1000 job cycles against a busy EARGM and power monitor: no deadlock");
    CHECK(my_pc_opt.current_pc >= 1 && my_pc_opt.current_pc <= MAX, "cap within [1, max] afterwards (%u)",
          my_pc_opt.current_pc);
    CHECK(pthread_mutex_trylock(&my_pc_opt.lock) == 0, "node lock not left held");
}

int main(void)
{
    RUN_ISOLATED(test_init_applies_idle_cap);
    RUN_ISOLATED(test_init_disabled_when_default_is_zero);
    RUN_ISOLATED(test_init_fails_when_no_domain_loads);
    RUN_ISOLATED(test_idle_to_run_with_default_power);
    RUN_ISOLATED(test_run_to_idle_and_release_idle_power);
    RUN_ISOLATED(test_release_idle_ignored_while_running);
    RUN_ISOLATED(test_power_per_domain_dispatch);
    RUN_ISOLATED(test_greedy_node_is_granted_extra);
    RUN_ISOLATED(test_eargm_reclaims_extra_from_ok_node);
    RUN_ISOLATED(test_greedy_grant_clamped_to_max_powercap);
    RUN_ISOLATED(test_greedy_node_not_trimmed_below_default);
    RUN_ISOLATED(test_greedy_requested_saturates);
    RUN_ISOLATED(test_node_not_in_greedy_list_ignores_grants);
    RUN_ISOLATED(test_release_status_ignores_grants);
    RUN_ISOLATED(test_ask_def_after_default_increase_while_running);
    RUN_ISOLATED(test_ask_def_request_survives_idle_to_run);
    RUN_ISOLATED(test_ask_def_node_in_greedy_list);
    RUN_ISOLATED(test_resched_only_on_significant_cap_change);
    RUN_ISOLATED(test_running_node_releases_power);
    RUN_ISOLATED(test_release_only_when_eargm_asks);
    RUN_ISOLATED(test_release_more_than_cap);
    RUN_ISOLATED(test_run_to_idle_after_release);
    RUN_ISOLATED(test_soft_powercap_limits);
    RUN_ISOLATED(test_soft_powercap_survives_job_end);
    RUN_ISOLATED(test_soft_powercap_keeps_accounting);
    RUN_ISOLATED(test_eargm_switches_soft_and_hard);
    RUN_ISOLATED(test_status_query_keeps_allocation_under_soft_limit);
    RUN_ISOLATED(test_floor_is_node_working_minimum);
    RUN_ISOLATED(test_default_raised_to_hardware_minimum);
    RUN_ISOLATED(test_default_reduce_increase_reset);
    RUN_ISOLATED(test_reset_after_grant_returns_to_conf_default);
    RUN_ISOLATED(test_cluster_near_limit_resets_on_idle);
    RUN_ISOLATED(test_next_job_keeps_extra_not_yet_released);
    RUN_ISOLATED(test_reduced_default_survives_idle_to_run);
    RUN_ISOLATED(test_unlimited_node_ignores_eargm);
    RUN_ISOLATED(test_manual_mode);
    RUN_ISOLATED(test_status_aggregates_over_tree);
    RUN_ISOLATED(test_device_storage_and_verification);
    RUN_ISOLATED(test_concurrent_eargm_and_monitor);
    return pc_summary("test_node");
}
