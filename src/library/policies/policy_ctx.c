/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <library/policies/policy_ctx.h>

#include <library/common/verbose_lib.h>


#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define DEF_FREQ(f) (!ext_def_freq?f:ext_def_freq)
#else
#define DEF_FREQ(f) f
#endif


static state_t policy_domain_detail(int status, char* buff);


void policy_show_domain(node_freq_domain_t *dom)
{
	char detail[32];

	verbose_master(2, "--- EAR policy domain ---");

	if (policy_domain_detail(dom->cpu, detail) == EAR_SUCCESS) {
		verbose_master(2, "      CPU: %s", detail);
	}

	if (policy_domain_detail(dom->mem, detail) == EAR_SUCCESS) {
		verbose_master(2, "      MEM: %s", detail);
	}

	if (policy_domain_detail(dom->gpu, detail) == EAR_SUCCESS) {
		verbose_master(2, "      GPU: %s", detail);
	}

	if (policy_domain_detail(dom->gpumem, detail) == EAR_SUCCESS) {
		verbose_master(2, "  GPU_MEM: %s", detail);
	}

	verbose_master(2, "-------------------------");
}


void verbose_policy_ctx(int verb_lvl, polctx_t *policy_ctx)
{
	// TODO: Improve the function body to minimize the number of I/O operations.
	if (verb_level >= verb_lvl)
	{
		verbose_master(verb_lvl, "\n%*s%s%*s", 1, "", "--- EAR policy context ---", 1, "");

		verbose_master(verb_lvl, "%-18s: %u",  "user_type",          policy_ctx->app->user_type);
		verbose_master(verb_lvl, "%-18s: %s",  "policy_name",        policy_ctx->app->policy_name);
		verbose_master(verb_lvl, "%-18s: %lf", "policy th",          policy_ctx->app->settings[0]);
		verbose_master(verb_lvl, "%-18s: %lu", "def_freq",           DEF_FREQ(policy_ctx->app->def_freq));
		verbose_master(verb_lvl, "%-18s: %u",  "def_pstate",         policy_ctx->app->def_p_state);
		verbose_master(verb_lvl, "%-18s: %d",  "reconfigure",        policy_ctx->reconfigure->force_rescheduling);
		verbose_master(verb_lvl, "%-18s: %lu", "user_selected_freq", policy_ctx->user_selected_freq);
		verbose_master(verb_lvl, "%-18s: %lu", "reset_freq_opt",     policy_ctx->reset_freq_opt);
		verbose_master(verb_lvl, "%-18s: %lu", "ear_frequency",      *(policy_ctx->ear_frequency));
		verbose_master(verb_lvl, "%-18s: %u",  "num_pstates",        policy_ctx->num_pstates);
		verbose_master(verb_lvl, "%-18s: %u",  "use_turbo",          policy_ctx->use_turbo);
		verbose_master(verb_lvl, "%-18s: %u",  "num_gpus",           policy_ctx->num_gpus);

		verbose_master(verb_lvl, "%*s%s%*s\n", 1, "", "--------------------------", 1, "");
	}
}


static state_t policy_domain_detail(int status, char* buff)
{
	switch (status)
	{
		case POL_NOT_SUPPORTED:
			sprintf(buff, "%sNOT SUPPORTED%s", COL_RED, COL_CLR);
			return EAR_SUCCESS;
		case POL_GRAIN_CORE:
			sprintf(buff, "%sGRAIN CORE%s", COL_GRE, COL_CLR);
			return EAR_SUCCESS;
		case POL_GRAIN_NODE:
			sprintf(buff, "%sGRAIN NODE%s", COL_GRE, COL_CLR);
			return EAR_SUCCESS;
		default:
			break;
	}
	return EAR_ERROR;
}
