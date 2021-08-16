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

#ifndef EAR_POLICIES_H
#define EAR_POLICIES_H

#include <common/states.h>
#include <common/types/loop.h>
#include <common/types/projection.h>
#include <daemon/shared_configuration.h>
#include <library/policies/policy_ctx.h>
#include <library/api/mpi_support.h>

state_t init_power_policy(settings_conf_t *app_settings,resched_t *res);
state_t policy_init();
state_t policy_node_apply(signature_t *my_sig,ulong *freq_set, int *ready);
state_t policy_app_apply(ulong *freq_set, int *ready);
state_t policy_get_default_freq(node_freqs_t *freq_set);
state_t policy_get_current_freq(node_freqs_t *freq_set);
state_t policy_set_default_freq(signature_t *sig);
state_t policy_ok(signature_t *c_sig,signature_t *l_sig,int *ok);
state_t policy_max_tries(int *intents);
state_t policy_end();
state_t policy_loop_init(loop_id_t *loop_id);
state_t policy_loop_end(loop_id_t *loop_id);
state_t policy_new_iteration(signature_t *sig);
state_t policy_mpi_init(mpi_call call_type);
state_t policy_mpi_end(mpi_call call_type);

state_t policy_domain_detail(int status, char *buff);
state_t policy_show_domain(node_freq_domain_t *dom);

state_t policy_force_global_frequency(ulong new_f);

/* These two functions needs to be reconsidered*/
state_t policy_configure();
#endif //EAR_POLICIES_H
