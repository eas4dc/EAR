/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_POLICIES_H
#define EAR_POLICIES_H

#include <common/states.h>
#include <common/types/loop.h>

#include <library/api/mpi_support.h>
#include <library/common/library_shared_data.h>
#include <library/policies/policy_ctx.h>

#include <daemon/shared_configuration.h>

/** This is the entry function of the module. It basically loads the policy plug-ins,
 * configures the context and reads and configure available P-States and max/min permitted.
 * Finally, it calls the policy_init method. */
state_t init_power_policy(settings_conf_t *app_settings, resched_t *res);

/** \todo */
state_t policy_node_apply(signature_t *my_sig, ulong *freq_set, int *ready);

/** Frequency selection at application-level.
 * Just the master process does something.
 * If non-master or no connection with EARD, `ready` set to EAR_POLICY_CONTINUE.
 * If the policy plug-in doesn't implement the method, `ready` set to EAR_POLICY_LOCAL_EV. */
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

#if MPI_OPTIMIZED
state_t policy_restore_to_mpi_pstate(uint index);
#endif

/**
 * \todo These two functions needs to be reconsidered. */
state_t policy_configure();

#endif // EAR_POLICIES_H
