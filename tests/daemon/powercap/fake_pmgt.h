/*
 * fake_pmgt.h -- recording fake of daemon/powercap/powercap_mgt.c, used to
 * test powercap.c (the node state machine and EARGM entry points) alone.
 *
 * Every call powercap.c makes into the pmgt layer is logged; the node status
 * pmgt_get_status() returns is scripted by the test through fake_pmgt_status.
 */
#ifndef FAKE_PMGT_H
#define FAKE_PMGT_H

#include <daemon/powercap/powercap_mgt.h>

#define FAKE_PMGT_MAX_CALLS 256

typedef struct fake_pmgt_call {
    uint domain;
    uint32_t limit;
} fake_pmgt_call_t;

/* Scripted */
extern pmgt_status_t fake_pmgt_status;    /* returned by pmgt_get_status()      */
extern state_t fake_pmgt_init_ret;        /* returned by pmgt_init()            */
extern uint fake_pmgt_devices[4];         /* per-domain device count            */
extern uint32_t fake_pmgt_hw_caps[4][16]; /* what the "hardware" reports back   */

/* Observed */
extern fake_pmgt_call_t fake_pmgt_calls[FAKE_PMGT_MAX_CALLS];
extern int fake_pmgt_num_calls;
extern uint fake_pmgt_last_status;       /* last pmgt_set_status()             */
extern int fake_pmgt_idle_to_run, fake_pmgt_run_to_idle, fake_pmgt_new_job, fake_pmgt_end_job;
extern int fake_pmgt_per_domain_calls;   /* pmgt_set_power_per_domain()        */
extern uint fake_pmgt_per_domain_last_st;
extern int fake_pmgt_msg_calls;          /* pmgt_process_message()             */
extern char fake_pmgt_msg_level[32];
extern int32_t fake_pmgt_msg_values[16];
extern int32_t fake_pmgt_msg_num;
extern int fake_pmgt_disabled;

void fake_pmgt_reset(void);
/* Last limit pushed to the domain layer, or -1 when none */
long long fake_pmgt_last_limit(void);

#endif
