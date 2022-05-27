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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// #define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/types/projection.h>

#include <management/cpufreq/frequency.h>

#include <daemon/powercap/powercap_status.h>

#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/policies/policy_api.h>
#include <library/policies/policy_state.h>
#include <library/policies/common/cpu_support.h>
#include <library/policies/common/gpu_support.h>
#include <library/policies/common/mpi_stats_support.h>

extern uint gpu_initialized;
extern uint cpu_ready, gpu_ready;


static polsym_t gpus;
state_t policy_init(polctx_t *c)
{
    if (is_mpi_enabled()) {
        mpi_app_init(c);
    }

#if USE_GPUS
    if (masters_info.my_master_rank >= 0 && gpu_initialized) {
        if (policy_gpu_init_ctx(c) != EAR_SUCCESS) {
            verbose(2, "%sError%s Loading GPU context.", COL_RED, COL_CLR);
        }
        verbose(2, "RANK %d GPU init", masters_info.my_master_rank);

        if (c->num_gpus) {
            verbose(2, "RANK %d GPUs %d", masters_info.my_master_rank, c->num_gpus);

            if (policy_gpu_load(c->app, &gpus) != EAR_SUCCESS) {
                verbose(2, "%sError%s Loading GPU policy.", COL_RED, COL_CLR);
            }

            if (gpus.init != NULL) {
                gpus.init(c);
            }
        }
    }
#endif

	return EAR_SUCCESS;
}

state_t policy_end(polctx_t *c)
{
	if (c != NULL) {
        return mpi_app_end(c);
    }
	return EAR_ERROR;
}

state_t policy_apply(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, int *ready)
{
    ulong *new_freq = freqs->cpu_freq;
    *new_freq= DEF_FREQ(c->app->def_freq);

    cpu_ready = EAR_POLICY_READY; // TODO: This variable is not used.

    if (gpus.node_policy_apply != NULL) {
        /* Basic support for GPUS */
        gpus.node_policy_apply(c, my_sig, freqs, (int *) &gpu_ready);
    }
    *ready = EAR_POLICY_READY;

    return EAR_SUCCESS;
}

state_t policy_ok(polctx_t *c, signature_t *curr_sig, signature_t *prev_sig, int *ok)
{
    if ((c == NULL) || (curr_sig == NULL) || (prev_sig == NULL)) return EAR_ERROR;

    *ok = 1;

    return EAR_SUCCESS;
}

state_t policy_get_default_freq(polctx_t *c, node_freqs_t *freq_set, signature_t *s)
{
    // TODO: The behaviour is the same done by policy.c by default when a plugin does not have this symbol.
	freq_set->cpu_freq[0] = DEF_FREQ(c->app->def_freq);
#if USE_GPUS
  if (gpus.get_default_freq != NULL){
    gpus.get_default_freq(c, freq_set, s);
  }
#endif

	return EAR_SUCCESS;
}

state_t policy_max_tries(polctx_t *c, int *intents)
{
	*intents = 0;
	return EAR_SUCCESS;
}

state_t policy_domain(polctx_t *c, node_freq_domain_t *domain)
{
    domain->cpu     = POL_GRAIN_NODE;
    domain->mem     = POL_NOT_SUPPORTED;

#if USE_GPUS		
    if (c->num_gpus) {
        domain->gpu = POL_GRAIN_CORE;
    } else {
        domain->gpu = POL_NOT_SUPPORTED;
    }
#else
    domain->gpu     = POL_NOT_SUPPORTED;
#endif

    domain->gpumem  = POL_NOT_SUPPORTED;
    return EAR_SUCCESS;
}

state_t policy_mpi_init(polctx_t *c, mpi_call call_type)
{
	if (c != NULL) {
        return mpi_call_init(c, call_type);
    }
	return EAR_ERROR;
}

state_t policy_mpi_end(polctx_t *c, mpi_call call_type)
{
	if (c != NULL) {
        return mpi_call_end(c,call_type);
    }
	return EAR_ERROR;
}

state_t policy_new_iteration(polctx_t *c, signature_t *sig)
{
	return EAR_SUCCESS;
}
