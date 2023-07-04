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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef EAR_POLICIES_H
#define EAR_POLICIES_H

#include <common/states.h>
#include <common/types/loop.h>
#include <common/types/projection.h>

#include <library/common/library_shared_data.h>
#include <library/api/mpi_support.h>
#include <library/policies/policy_ctx.h>

#include <daemon/shared_configuration.h>

/** This is the entry function of the module. It basically loads the policy plug-in,
 * configures the context and reads and configure available P-States and max/min permitted.
 * Finally, it calls policy_init method. */
state_t init_power_policy(settings_conf_t *app_settings, resched_t *res);


/** \todo */
state_t policy_init();


/** \todo */
state_t policy_node_apply(signature_t *my_sig, ulong *freq_set, int *ready);

/** \todo */
state_t policy_app_apply(ulong *freq_set, int *ready);


/** This function fills \p freq_set with the default freq (only CPU) configured by the policy plugin loaded.
 * If the plug-in has no entry method, the default frequency got from the context of the current instance is set.
 * Only if the master has called this function the parameter will be set.
 * If called by other process just returns EAR_SUCCESS. */
state_t policy_get_default_freq(node_freqs_t *freq_set);


state_t policy_get_current_freq(node_freqs_t *freq_set);
state_t policy_set_default_freq(signature_t *sig);
state_t policy_ok(signature_t *c_sig, signature_t *l_sig, int *ok);
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
state_t policy_restore_to_mpi_pstate(uint index);

/**
 * \todo These two functions needs to be reconsidered. */
state_t policy_configure();



#endif // EAR_POLICIES_H
