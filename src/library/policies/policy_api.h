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
#include <common/types/configuration/policy_conf.h>

#include <library/policies/policy_ctx.h>

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

/** This function is executed at each loop iteration or beginning of a period.
 * \param c The policy context within this function is called.
 * \param sig The signature computed. */
state_t policy_new_iteration(polctx_t *c, signature_t *sig);

/** This function is executed at the beginning of each MPI call.
 * Warning! it could introduce a lot of overhead.
 * \param c The policy context within this function is called.
 * \param call_type The call type information. */
state_t policy_mpi_init(polctx_t *c, mpi_call call_type);

/** This function is executed after each MPI call.
 * Warning! it could introduce a lot of overhead.
 * \param c The policy context within this function is called.
 * \param call_type The call type information. */
state_t policy_mpi_end(polctx_t *c, mpi_call call_type);

/** This function is executed at loop init or period init.
 * \param c The policy context within this function is called.
 * \param loop_id The loop information. */
state_t policy_loop_init(polctx_t *c, loop_id_t *loop_id);

/** This function is executed at each loop end.
 * \param c The policy context within this function is called.
 * \param loop_id The loop information. */
state_t policy_loop_end(polctx_t *c, loop_id_t *loop_id);

/** This function confgures freqs based on IO criteria. It is called before call policy_apply.
 * \param c The policy context within this function is called.
 * \param my_sig The input signature with node metrics.
 * \param freq_set Output array with per-proc frequency. */
state_t policy_io_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs);

/** This function confgures freqs based on Busy Waiting criteria. It is called before call policy_apply.
 * \param c The policy context within this function is called.
 * \param my_sig The input signature with node metrics.
 * \param freq_set Output array with per-proc frequency. */
state_t policy_busy_wait_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs);

/** This function sets again default frequencies after a setting modification.
 * \param c The policy context within this function is called.
 * \param my_sig The input signature with node metrics.
 * \param freq_set Output array with per-proc frequency. */
state_t policy_restore_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs);

/** \todo This function does not exist. */
state_t policy_set_risk(policy_conf_t *ref, policy_conf_t *current, ulong risk_level, ulong opt_target,
        ulong mfreq, ulong *nfreq, ulong *f_list, uint nump);

/** This function is executed when a reconfiguration needs to be done.
 * \param c The policy context within this function is called. */
state_t policy_configure(polctx_t *c);

#endif // EAR_POLICIES_API_H
