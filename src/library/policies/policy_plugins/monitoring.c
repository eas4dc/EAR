/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <library/common/verbose_lib.h>
#include <library/loader/module_mpi.h>
#include <library/policies/common/cpu_support.h>
#include <library/policies/common/gpu_support.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/policy_api.h>
#include <stdio.h>
#include <string.h>
#if MPI_OPTIMIZED
#include <common/system/monitor.h>
#endif

#define MON_ERROR_LVL 2
#define MON_INFO_LVL  3

extern uint cpu_ready, gpu_ready;

static polsym_t gpus;

#if MPI_OPTIMIZED
extern sem_t *lib_shared_lock_sem;
extern uint ear_mpi_opt;
// extern suscription_t *earl_monitor;
extern mpi_opt_policy_t mpi_opt_symbols;
#endif

/* These functions are used for mpi optimization */
static void policy_end_summary(int verb_lvl);

static state_t policy_mpi_init_optimize(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id);

static state_t policy_mpi_end_optimize(node_freqs_t *freqs, int *process_id);

state_t policy_init(polctx_t *c)
{
    if (module_mpi_is_enabled()) {
#if MPI_OPTIMIZED
        mpi_opt_load(&mpi_opt_symbols);
#endif
        mpi_app_init(c);
    }

    for (uint i = 0; i < lib_shared_region->num_processes; i++) {
        sig_shared_region[i].new_freq = DEF_FREQ(c->app->def_freq);
#if MPI_OPTIMIZED
        sig_shared_region[i].mpi_freq = DEF_FREQ(c->app->def_freq);
#endif
    }

#if USE_GPUS
    if (masters_info.my_master_rank >= 0) {
        verbose(MON_INFO_LVL, "RANK %d GPU init", masters_info.my_master_rank);

        if (c->num_gpus) {
            verbose(MON_INFO_LVL, "RANK %d GPUs %d", masters_info.my_master_rank, c->num_gpus);

            if (policy_gpu_load(c->app, &gpus) != EAR_SUCCESS) {
                verbose(MON_ERROR_LVL, "%sError%s Loading GPU policy.", COL_RED, COL_CLR);
            }

            verbose(2, "Initialzing GPU policy part");
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
    policy_end_summary(2); // Summary of optimization

    if (c != NULL) {
        return mpi_app_end(c);
    }

    return EAR_ERROR;
}

state_t policy_apply(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, int *ready)
{
    ulong *new_freq = freqs->cpu_freq;
    *new_freq       = DEF_FREQ(c->app->def_freq);

    for (uint i = 0; i < lib_shared_region->num_processes; i++) {
        sig_shared_region[i].new_freq = new_freq[0];
#if MPI_OPTIMIZED
        sig_shared_region[i].mpi_freq = new_freq[0];
#endif
    }

#if USE_GPUS
    if (gpus.node_policy_apply != NULL) {
        /* Basic support for GPUS */
        gpus.node_policy_apply(c, my_sig, freqs, (int *) &gpu_ready);
    } else {
        gpu_ready = EAR_POLICY_READY;
    }
    *ready = gpu_ready;
#else
    *ready = EAR_POLICY_READY;
#endif

    return EAR_SUCCESS;
}

state_t policy_ok(polctx_t *c, signature_t *curr_sig, signature_t *prev_sig, int *ok)
{
    if ((c == NULL) || (curr_sig == NULL) || (prev_sig == NULL)) {
        return EAR_ERROR;
    }
    *ok = 1;
    return EAR_SUCCESS;
}

state_t policy_restore_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs)
{
    state_t st = EAR_SUCCESS;

#if USE_GPUS
    if (gpus.restore_settings != NULL) {
        /* Basic support for GPUS */
        verbose(2, "%sRestoring GPU policy settings%s", COL_GRE, COL_CLR);
        st = gpus.restore_settings(c, my_sig, freqs);
    }
#endif
    return st;
}

state_t policy_get_default_freq(polctx_t *c, node_freqs_t *freq_set, signature_t *s)
{
    // TODO: The behaviour is the same done by policy.c
    // by default when a plugin does not have this symbol.

    freq_set->cpu_freq[0] = DEF_FREQ(c->app->def_freq);

#if USE_GPUS
    if (gpus.get_default_freq != NULL) {
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
    domain->cpu = POL_GRAIN_NODE;
    domain->mem = POL_NOT_SUPPORTED;
#if USE_GPUS
    if (c->num_gpus) {
        domain->gpu = POL_GRAIN_CORE;
    } else {
        domain->gpu = POL_NOT_SUPPORTED;
    }
#else
    domain->gpu = POL_NOT_SUPPORTED;
#endif
    domain->gpumem = POL_NOT_SUPPORTED;

    return EAR_SUCCESS;
}

state_t policy_mpi_init(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
    if (c != NULL) {
        state_t st = mpi_call_init(c, call_type);
        policy_mpi_init_optimize(c, call_type, freqs, process_id);
        return st;
    }
    return EAR_ERROR;
}

state_t policy_mpi_end(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
    if (c != NULL) {
        state_t st = mpi_call_end(c, call_type);
        policy_mpi_end_optimize(freqs, process_id);
        return st;
    }
    return EAR_ERROR;
}

static state_t policy_mpi_init_optimize(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
#if MPI_OPTIMIZED
    // You can implement optimization at MPI call entry here.
    /* CPUfreq mgt is not ok we will not optimize */
    if (!metrics_get(MGT_CPUFREQ)->ok)
        return EAR_SUCCESS;

    if (mpi_opt_symbols.init_mpi != NULL) {
        return mpi_opt_symbols.init_mpi(c, call_type, freqs, process_id);
    }
#endif
    return EAR_SUCCESS;
}

static void policy_end_summary(int verb_lvl)
{
    // You can show a summary of your optimization at MPI call level here.
#if MPI_OPTIMIZED
    if (mpi_opt_symbols.summary != NULL)
        mpi_opt_symbols.summary(VERB_GET_LV());
#endif
}

static state_t policy_mpi_end_optimize(node_freqs_t *freqs, int *process_id)
{
#if MPI_OPTIMIZED
    // You can implement optimization at MPI call exit here.
    if (mpi_opt_symbols.end_mpi != NULL) {

        return mpi_opt_symbols.end_mpi(freqs, process_id);
    }
#endif
    return EAR_SUCCESS;
}
