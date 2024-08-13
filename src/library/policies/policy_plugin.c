/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <library/policies/policy_plugin.h>

#include <common/states.h>
#include <common/config.h>
#include <common/system/symplug.h>
#include <common/plugins.h>

#include <library/common/verbose_lib.h>
#include <library/policies/policy_ctx.h>
#include <library/common/externs.h>


struct policy_plugin_s
{
	char *plugin_api_version;
	state_t (*init)								(polctx_t *c);
	state_t (*node_policy_apply) 	(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,int *ready);
	state_t (*app_policy_apply)	 	(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs,int *ready);
	state_t (*get_default_freq)	 	(polctx_t *c, node_freqs_t *freq_set, signature_t *s);
	state_t (*ok)								 	(polctx_t *c, signature_t *curr_sig, signature_t *prev_sig, int *ok);
	state_t (*max_tries)				 	(polctx_t *c, int *intents);
	state_t (*end)							 	(polctx_t *c);
	state_t (*loop_init)				 	(polctx_t *c, loop_id_t *loop_id);
	state_t (*loop_end)					 	(polctx_t *c, loop_id_t *loop_id);
	state_t (*new_iter)					 	(polctx_t *c, signature_t *sig);
	state_t (*mpi_init)					 	(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id);
	state_t (*mpi_end)					 	(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id);
	state_t (*configure)				 	(polctx_t *c);
	state_t (*domain)						 	(polctx_t *c, node_freq_domain_t *domain);
	state_t (*io_settings)			 	(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs);
	state_t (*cpu_gpu_settings)		(polctx_t *c, signature_t *my_sig,node_freqs_t *freqs);
	state_t (*busy_wait_settings) (polctx_t *c, signature_t *my_sig, node_freqs_t *freqs);
	state_t (*restore_settings)		(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs);
};


static const int polsyms_n = 19;

static const char *polsyms_nam[] = {
	"plugin_api_version",
	"policy_init",
	"policy_apply",
	"policy_app_apply",
	"policy_get_default_freq",
	"policy_ok",
	"policy_max_tries",
	"policy_end",
	"policy_loop_init",
	"policy_loop_end",
	"policy_new_iteration",
	"policy_mpi_init",
	"policy_mpi_end",
	"policy_configure",
	"policy_domain",
	"policy_io_settings",
	"policy_cpu_gpu_settings",
	"policy_busy_wait_settings",
	"policy_restore_settings"
};


static const char api_version = VERSION_MAJOR;


p_plugin_t policy_plugin_load(char *plugin_path_name)
{
	if (!plugin_path_name)
	{
		return NULL;
	}

	p_plugin_t policy_plugin = malloc(sizeof *policy_plugin);
	if (!policy_plugin)
	{
		return NULL;
	}

	if (state_fail(symplug_open(plugin_path_name, (void *) policy_plugin, polsyms_nam, polsyms_n)))
	{
		verbose_error_master("Loading %s: %s", plugin_path_name, state_msg);
		free(policy_plugin);
		policy_plugin = NULL;
	}

	// Check plug-in compatibility
	if (policy_plugin->plugin_api_version)
	{
		if (*(policy_plugin->plugin_api_version) > api_version)
		{
			verbose_warning_master("Plugin %s version (%c) greater than API version (%c)", plugin_path_name, *(policy_plugin->plugin_api_version), api_version);
		}
	} else
	{
		verbose_error_master("Policy plug-in does not have API version.");
	}

	return policy_plugin;
}


void policy_plugin_dispose(p_plugin_t policy_plugin)
{
	free(policy_plugin);
	policy_plugin = NULL;
}


state_t policy_plugin_init(p_plugin_t p_plugin, polctx_t *c)
{
	if (!p_plugin || !c)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	if (!p_plugin->init)
	{
		return_msg(EAR_WARNING, "policy_init method not implemented.");
	}

	return p_plugin->init(c);
}


int policy_plugin_has_node_apply(p_plugin_t p_plugin)
{
	if (p_plugin)
	{
		return (p_plugin->node_policy_apply != NULL);
	}

	return 0;
}


state_t policy_plugin_apply(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs, int *ready)
{
	if (!p_plugin || !c || !my_sig || !freqs || !ready)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	state_t ret = EAR_ERROR;

	if (p_plugin->node_policy_apply)
	{
		ret = p_plugin->node_policy_apply(c, my_sig, freqs, ready);
	}

	return ret;
}


int policy_plugin_has_app_apply(p_plugin_t p_plugin)
{
	if (p_plugin)
	{
		return (p_plugin->app_policy_apply != NULL);
	}

	return 0;
}


state_t policy_plugin_app_apply(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs, int *ready)
{
	if (!p_plugin || !c || !my_sig || !freqs || !ready)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	if (p_plugin->app_policy_apply)
	{
		return p_plugin->app_policy_apply(c, my_sig, freqs, ready);
	}

	return EAR_ERROR;
}


state_t policy_plugin_get_default_freq(p_plugin_t p_plugin, polctx_t *c,
		node_freqs_t *freq_set, signature_t *s)
{
	if (!p_plugin || !c || !freq_set || !s)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	if (p_plugin->get_default_freq)
	{
		return p_plugin->get_default_freq(c, freq_set, s);
	}

	verbose_warning("Policy plug-in hasn't get_default_freq symbol. Default freq.: %lu", c->app->def_freq);

	for (int i = 0; i < MAX_CPUS_SUPPORTED; i++)
	{
		freq_set->cpu_freq[i] = c->app->def_freq;
	}

	return EAR_ERROR;
}


state_t policy_plugin_ok(p_plugin_t p_plugin, polctx_t *c, signature_t *curr_sig,
		signature_t *prev_sig, int *ok)
{
	if (!p_plugin || !c || !curr_sig || !prev_sig || !ok)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	if (!p_plugin->ok)
	{
		*ok = 1;

		verbose_warning_master("The plug-in doesn't implement the method. Assuming ok...");

		return EAR_SUCCESS;
	}

#if 0
	pc_support_compute_next_state(c, &my_pol_ctx.app->pc_opt, curr);
#endif

	return p_plugin->ok(c, curr_sig, prev_sig, ok);
}


state_t policy_plugin_max_tries(p_plugin_t p_plugin, polctx_t *c, int *intents)
{
	if (!p_plugin || !c || !intents)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	if (!p_plugin->max_tries)
	{
		*intents = 1;
		return EAR_SUCCESS;
	}

	return p_plugin->max_tries(c, intents);
}


state_t policy_plugin_end(p_plugin_t p_plugin, polctx_t *c)
{
	if (!p_plugin || !c)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	preturn(p_plugin->end, c);
}


state_t policy_plugin_loop_init(p_plugin_t p_plugin, polctx_t *c, loop_id_t *loop_id)
{
	if (!p_plugin || !c || !loop_id)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	preturn(p_plugin->loop_init, c, loop_id);
}


state_t policy_plugin_loop_end(p_plugin_t p_plugin, polctx_t *c, loop_id_t *loop_id)
{
	if (!p_plugin || !c || !loop_id)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	preturn(p_plugin->loop_end, c, loop_id);
}


state_t policy_plugin_new_iteration(p_plugin_t p_plugin, polctx_t *c, signature_t *sig)
{
	if (!p_plugin || !c || !sig)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	preturn_opt(p_plugin->new_iter, c, sig);
}


state_t policy_plugin_mpi_init(p_plugin_t p_plugin, polctx_t *c, mpi_call call_type,
		node_freqs_t *freqs, int *process_id)
{
	if (!p_plugin || !c || !freqs || !process_id)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	preturn_opt(p_plugin->mpi_init, c, call_type, freqs, process_id);
}

state_t policy_plugin_mpi_end(p_plugin_t p_plugin, polctx_t *c, mpi_call call_type,
		node_freqs_t *freqs, int *process_id)
{
	if (!p_plugin || !c || !freqs || !process_id)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	preturn_opt(p_plugin->mpi_end, c, call_type, freqs, process_id);
}


state_t policy_plugin_configure(p_plugin_t p_plugin, polctx_t *c)
{
	if (!p_plugin || !c)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	preturn(p_plugin->configure, c);
}


state_t policy_plugin_domain(p_plugin_t p_plugin, polctx_t *c, node_freq_domain_t *domain)
{
	if (!p_plugin || !c || !domain)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	state_t ret = EAR_ERROR;

	if (p_plugin->domain)
	{
		ret = p_plugin->domain(c, domain);
	}

	if (state_fail(ret))
	{
		domain->cpu = POL_GRAIN_NODE;
		domain->mem = POL_NOT_SUPPORTED;
		domain->gpu = POL_NOT_SUPPORTED;
		domain->gpumem = POL_NOT_SUPPORTED;
	}

	// TODO: Show domain function here.

	return ret;
}


state_t policy_plugin_io_settings(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs)
{
	if (!p_plugin || !c || !my_sig || !freqs)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	if (p_plugin->io_settings)
	{
		p_plugin->io_settings(c, my_sig, freqs);
	}

	return EAR_SUCCESS;
}


state_t policy_plugin_cpu_gpu_settings(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs)
{
	if (!p_plugin || !c || !my_sig || !freqs)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	if (p_plugin->cpu_gpu_settings)
	{
		p_plugin->cpu_gpu_settings(c, my_sig, freqs);
	}

	return EAR_SUCCESS;
}


state_t policy_plugin_busy_wait_settings(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs)
{
	if (!p_plugin || !c || !my_sig || !freqs)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	if (p_plugin->busy_wait_settings)
	{
		p_plugin->busy_wait_settings(c, my_sig, freqs);
	}

	return EAR_SUCCESS;
}


state_t policy_plugin_restore_settings(p_plugin_t p_plugin, polctx_t *c, signature_t *my_sig,
		node_freqs_t *freqs)
{
	if (!p_plugin || !c || !my_sig || !freqs)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	state_t ret = EAR_ERROR;

	if (p_plugin->restore_settings)
	{
		ret = p_plugin->restore_settings(c, my_sig, freqs);
	}

	return ret;
}
