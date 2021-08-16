/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef EAR_POLICIES_API_H
#define EAR_POLICIES_API_H

#include <common/states.h>
#include <library/policies/policy_ctx.h>
#include <common/types/configuration/policy_conf.h>
state_t policy_init(polctx_t *c);
state_t policy_apply(polctx_t *c,signature_t *my_sig, node_freqs_t *freqs,int *ready);
state_t policy_get_default_freq(polctx_t *c, node_freqs_t *freq_set,signature_t *sig);
state_t policy_ok(polctx_t *c, signature_t *curr_sig,signature_t *prev_sig,int *ok);
state_t policy_max_tries(polctx_t *c,int *intents);
state_t policy_end(polctx_t *c);
state_t policy_loop_init(polctx_t *c,loop_id_t *loop_id);
state_t policy_loop_end(polctx_t *c,loop_id_t *loop_id);
state_t policy_new_iteration(polctx_t *c,signature_t *sig);
state_t policy_mpi_init(polctx_t *c,mpi_call call_type);
state_t policy_mpi_end(polctx_t *c,mpi_call call_type);
state_t policy_configure(polctx_t *c);
state_t policy_set_risk(policy_conf_t *ref,policy_conf_t *current,ulong risk_level,ulong opt_target,ulong mfreq,ulong *nfreq,ulong *f_list,uint nump);
state_t policy_io_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs);
state_t policy_busy_wait_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs);
state_t policy_restore_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs);




#endif //EAR_POLICIES_API_H
