/*
 * eargm_sim.h -- simulated EARGM, talking to the powercap of THIS process.
 *
 * In production EARGM reaches EARD through the remote API
 * (daemon/remote_api/eard_rapi.c -> dynamic_configuration.c). These helpers
 * perform, for one node, exactly what the EARD side of each request does
 * (the dyncon_* handlers), minus the socket and the tree propagation:
 *
 *   EARGM request               EARD handler                  powercap entry point
 *   --------------------------  ----------------------------  ------------------------------
 *   ear_get_powercap_status     dyncon_get_powerstatus        powercap_get_status + process_pmgt_status
 *   ear_cluster_set_powercap_opt dyncon_power_management      dyncon_filter_opt + powercap_set_opt
 *   ear_cluster_release_idle    dyncon_release_idle_power     powercap_release_idle_power
 *   ear_set_powerlimit          EAR_RC_SET_POWER              powercap_set_powercap
 *   ear_cluster_red_powercap    EAR_RC_RED_POWER (ABSOLUTE)   powercap_reduce_def_power
 *   ear_cluster_inc_powercap    EAR_RC_INC_POWER (ABSOLUTE)   powercap_increase_def_power
 *   ear_cluster_reset_powercap  EAR_RC_DEF_POWERCAP           powercap_reset_default_power
 *
 * dyncon_filter_opt and process_pmgt_status are reproduced verbatim in
 * eargm_sim.c (they live in dynamic_configuration.c, which drags in the whole
 * daemon); keep them in sync if the originals change.
 */
#ifndef EARGM_SIM_H
#define EARGM_SIM_H

#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_status_conf.h>

/* IP this node answers to (the value EARD finds in ips[self_id]) */
extern int eargm_sim_node_ip;

/* The per-node reply EARGM aggregates. The caller owns st->greedy_* */
typedef struct eargm_reply {
    powercap_status_t st;
    pmgt_status_t pmgt;
} eargm_reply_t;

void eargm_get_status(eargm_reply_t *r, int release_power);
void eargm_reply_free(eargm_reply_t *r);

/*
 * Sends a powercap_opt. greedy_ips/extra are num_greedy long. Returns the id
 * the node found for itself (-1 when it is not in the list).
 */
int eargm_send_opt(uint32_t num_greedy, const int32_t *greedy_ips, const int32_t *extra, uint8_t cluster_perc);

uint32_t eargm_release_idle(void);
void eargm_set_powerlimit(uint32_t limit);
void eargm_reduce_def(uint32_t watts);
void eargm_increase_def(uint32_t watts);
void eargm_reset_def(void);

#endif
