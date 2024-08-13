/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#ifndef _EARL_POLICY_PLUGIN_H
#define _EARL_POLICY_PLUGIN_H


#include <common/states.h>
#include <common/types/signature.h>

#include <library/policies/policy_ctx.h>


/** A policy plug-in handle. */
typedef struct policy_plugin_s* p_plugin_t;


/** Loads the policy plug-in specified by \ref plugin_path_name. */
p_plugin_t policy_plugin_load(char *plugin_path_name);

/** Frees memory allocated by the input argument. */
void policy_plugin_dispose(p_plugin_t p_plugin);

/** Calls init method of the \ref p_plugin loaded plug-in.
 * \return EAR_SUCCESS If the plug-in method returns EAR_SUCCESS.
 * \return EAR_WARNING If the plug-in does not implment the method (fills state_msg).
 * \return EAR_ERROR If the plug-in returns EAR_ERROR.
 * \return EAR_ERROR If the \ref p_plugin is a null pointer. */
state_t policy_plugin_init(p_plugin_t p_plugin, polctx_t *c);

/** Returns non-zero value if \ref p_plugin has node_apply method. */
int policy_plugin_has_node_apply(p_plugin_t p_plugin);

/** Calls node_policy_apply method of the loaded plug-in.
 * \return What the implemented method returns.
 * \return EAR_ERROR If any of the input arguments is null
 * or the plug-in does not implement the method. */
state_t policy_plugin_apply(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, int *ready);

/** Returns non-zero value if \ref p_plugin has app_apply method. */
int policy_plugin_has_app_apply(p_plugin_t p_plugin);

/** Calls app_policy_apply method of the loaded plug-in.
 * \return What the implemented method returns.
 * \return EAR_ERROR If any of the input arguments is null
 * or the plugi-in does not implement the method. */
state_t policy_plugin_app_apply(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs, int *ready);

/** Calls get_default_freq method of the loaded plug-in.
 * If the plug-in does not implement the method, \ref freqs->cpu_freq are filled with the default
 * frequency stored in \ref c->app->def_freq and EAR_ERROR is returned.
 * \return EAR_ERROR if any of the input arguments are null. */
state_t policy_plugin_get_default_freq(p_plugin_t p_plugin, polctx_t *c,
		node_freqs_t *freq_set, signature_t *s);

/** Calls ok method of the loaded plug-in. If the plug-in does not implement
 * the method, \ref ok is set to non-zero and returns EAR_SUCCESS.
 * \return Whatever plug-in's method returns. */
state_t policy_plugin_ok(p_plugin_t p_plugin, polctx_t *c, signature_t *curr_sig,
		signature_t *prev_sig, int *ok);

/** Calls max_tries method if implemented by the loaded plug-in.
 * If not implemented, \ref intents is set to 1 and EAR_SUCCESS is returned. */
state_t policy_plugin_max_tries(p_plugin_t p_plugin, polctx_t *c, int *intents);

/** Calls end method of the loaded plug-in.
 * If the method is not implemented a non-success state_t is returned.
 * \return Whatever the plug-in's method returns.
 * \return EAR_ERROR if any of the input arguments is null. */
state_t policy_plugin_end(p_plugin_t p_plugin, polctx_t *c);

/** Calls loop_init method of the loaded plug-in.
 * If the method is not implemented a non-success state_t is returned.
 * \return Whatever the plug-in's method returns.
 * \return EAR_ERROR if any of the input arguments is null. */
state_t policy_plugin_loop_init(p_plugin_t p_plugin, polctx_t *c, loop_id_t *loop_id);

/** Calls loop_end method of the loaded plug-in.
 * If the method is not implemented a non-success state_t is returned.
 * \return Whatever the plug-in's method returns.
 * \return EAR_ERROR if any of the input arguments is null. */
state_t policy_plugin_loop_end(p_plugin_t p_plugin, polctx_t *c, loop_id_t *loop_id);

/** Calls new_iter method of the loaded plug-in.
 * \return Whatever the plug-in's method returns, if implemented.
 * \return EAR_SUCCESS if the plug-in does not implement the method.
 * \return EAR_ERROR if any of the input arguments is null. */
state_t policy_plugin_new_iteration(p_plugin_t p_plugin, polctx_t *c, signature_t *sig);

/** Calls mpi_init method of the loaded plug-in.
 * \return Whatever the plug-in's method returns, if implemented.
 * \return EAR_SUCCESS if the plug-in does not implement the method.
 * \return EAR_ERROR if any of the input arguments is null, except call_type. */
state_t policy_plugin_mpi_init(p_plugin_t p_plugin, polctx_t *c, mpi_call call_type,
		node_freqs_t *freqs, int *process_id);

/** Calls mpi_end method of the loaded plug-in.
 * \return Whatever the plug-in's method returns, if implemented.
 * \return EAR_SUCCESS if the plug-in does not implement the method.
 * \return EAR_ERROR if any of the input arguments is null, except call_type. */
state_t policy_plugin_mpi_end(p_plugin_t p_plugin, polctx_t *c, mpi_call call_type,
		node_freqs_t *freqs, int *process_id);

/** Calls configure method of the loaded plug-in.
 * If the method is not implemented a non-success state_t is returned.
 * \return Whatever the plug-in's method returns.
 * \return EAR_ERROR if any of the input arguments is null. */
state_t policy_plugin_configure(p_plugin_t p_plugin, polctx_t *c);

/** Calls the domain retrieve method of the loaded plug-in \ref p_plugin.
 * If the plug-in does not implement the method or it returns an error, the following
 * default domain is set:
 * - cpu = POL_GRAIN_NODE
 * - mem = POL_NOT_SUPPORTED
 * - gpu = POL_NOT_SUPPORTED
 * - gpumem = POL_NOT_SUPPORTED
 * This function also prints the domain set.
 *
 * \return EAR_ERROR if the plug-in does not implement the method nor it returns an error.
 * \return EAR_SUCCESS otherwise.
 */
state_t policy_plugin_domain(p_plugin_t p_plugin, polctx_t *c, node_freq_domain_t *domain);

/** Calls the io_settings function of the loaded plug-in.
 * \return EAR_SUCCESS Even the plug-in does not implement the method. */
state_t policy_plugin_io_settings(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs);

/** Calls the cpu_gpu_settings function of the loaded plug-in.
 * \return EAR_SUCCESS Even the plug-in does not implement the method. */
state_t policy_plugin_cpu_gpu_settings(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs);

/** Calls the busy_wait_settings function of the loaded plug-in.
 * \return EAR_SUCCESS Even the plug-in does not implement the method. */
state_t policy_plugin_busy_wait_settings(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs);

/** Calls the restore_settings function of the loaded plug-in.
 * \return EAR_ERROR The input arguments are null, the plug-in does not implment the method,
 * or the plug-in's method returns an error. */
state_t policy_plugin_restore_settings(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs);

#endif // _EARL_POLICY_PLUGIN_H
