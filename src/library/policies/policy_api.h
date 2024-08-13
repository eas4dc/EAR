/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#ifndef _EARL_POLICY_API_H
#define _EARL_POLICY_API_H


#include <common/states.h>
#include <common/config.h>
#include <library/common/externs.h>
#include <library/policies/policy_ctx.h>
#include <library/policies/policy_state.h>

const char plugin_api_version = VERSION_MAJOR;

/** This function is called once the plugin symbols are loaded, and policy options are read
 * through environment variables. Plugins use this function to define their internal variables,
 * to read their own configuration, etc.
 * \param c The policy context within this function is called. */
state_t policy_init(polctx_t *c);

/** This is the main function for node-level frequency selection.
 * \param c The policy context within this function is called.
 * \param my_sig The input signature with node metrics.
 * \param freq_set Output array with per-proc frequency. The plugin must fill this array with the frequencies selected.
 * \param ready Output argument to inform the state of the policy plugin. */
state_t policy_apply(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, int *ready);

/** \todo  */
state_t policy_app_apply(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,int *ready);

/** This function only is called by a node master process.
 * \param c The policy context within this function is called.
 * \param freq_set Output array with per-proc frequency. The plugin must fill this array with the frequencies selected.
 * \param sig Not used. */
state_t policy_get_default_freq(polctx_t *c, node_freqs_t *freq_set, signature_t *sig);

/** This function checks if the frequency selection is correct.
 * \param c The policy context within this function is called.
 * \param curr_sig The current node application signature.
 * \param prev_sig The previous node application signature.
 * \param ok An output parameter to tell if the policy plugin says ok. */
state_t policy_ok(polctx_t *c, signature_t *curr_sig, signature_t *prev_sig, int *ok);

/** This function is used to tell how many number of attempts (at most) needs your plugin to select the optimal
 * CPU frequency. Only called by the node master process.
 * \param c The policy context within this function is called.
 * \param intents The output parameter. */
state_t policy_max_tries(polctx_t *c, int *intents);

/** This function is called at application end.
 * \param c The policy context within this function is called. */
state_t policy_end(polctx_t *c);

/** This function is executed at loop init or period init.
 * \param c The policy context within this function is called.
 * \param loop_id The loop information. */
state_t policy_loop_init(polctx_t *c, loop_id_t *loop_id);

/** This function is executed at each loop end.
 * \param c The policy context within this function is called.
 * \param loop_id The loop information. */
state_t policy_loop_end(polctx_t *c, loop_id_t *loop_id);

/** This function is executed at each loop iteration or beginning of a period.
 * \param c The policy context within this function is called.
 * \param sig The signature computed. */
state_t policy_new_iteration(polctx_t *c, signature_t *sig);

/** This function is executed at the beginning of each MPI call.
 * Warning! it could introduce a lot of overhead.
 * \param c The policy context within this function is called.
 * \param call_type The call type information. */
state_t policy_mpi_init(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id);

/** This function is executed after each MPI call.
 * Warning! it could introduce a lot of overhead.
 * \param c The policy context within this function is called.
 * \param call_type The call type information. */
state_t policy_mpi_end(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id);

/** This function is executed when a reconfiguration needs to be done.
 * \param c The policy context within this function is called. */
state_t policy_configure(polctx_t *c);

/** \todo */
state_t policy_domain(polctx_t *c, node_freq_domain_t *domain);

/** This function confgures freqs based on IO criteria. It is called before call policy_apply.
 * \param c The policy context within this function is called.
 * \param my_sig The input signature with node metrics.
 * \param freq_set Output array with per-proc frequency. */
state_t policy_io_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs);

/** This function confgures freqs based on CPU/GPU criteria. It is called before call policy_apply.
 * \param c The policy context within this function is called.
 * \param my_sig The input signature with node metrics.
 * \param freq_set Output array with per-proc frequency. */
state_t policy_cpu_gpu_settings(polctx_t *c,signature_t *my_sig, node_freqs_t *freqs);

/** This function confgures freqs based on Busy Waiting criteria. It is called before call policy_apply.
 * \param c The policy context within this function is called.
 * \param my_sig The input signature with node metrics.
 * \param freq_set Output array with per-proc frequency. */
state_t policy_busy_wait_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs);

/** This function sets again default frequencies after a setting modification.
 * \param c The policy context within this function is called.
 * \param my_sig The input signature with node metrics.
 * \param freq_set Output array with per-proc frequency. */
state_t policy_restore_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs);

#endif // _EARL_POLICY_API_H
