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


#include <common/types.h>


#define MON_ERROR_LVL 2
#define MON_INFO_LVL  3

extern uint cpu_ready, gpu_ready;

static polsym_t gpus;

static cpi_t cpi_1;
static cpi_t cpi_2;
static cpi_t cpi_diff;
static double comp_cpi;
static ullong comp_cycles;
static ullong comp_insts;

state_t policy_init(polctx_t *c)
{

    if (is_mpi_enabled()) {
        cpi_read(no_ctx, &cpi_1);
        mpi_app_init(c);
    }

    return EAR_SUCCESS;
}


state_t policy_end(polctx_t *c)
{
    if (is_mpi_enabled()) {
        cpi_read_diff(no_ctx, &cpi_2, &cpi_1, &cpi_diff, &comp_cpi);
        comp_cycles += cpi_diff.cycles;
        comp_insts += cpi_diff.instructions;

        verbose(0, "Comp. CPI (%u): %lf", my_node_id, (double) comp_cycles / (double) comp_insts);
    }

    if (c != NULL) {
        return mpi_app_end(c);
    }

    return EAR_ERROR;
}


state_t policy_apply(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, int *ready)
{
    ulong *new_freq = freqs->cpu_freq;
    *new_freq = DEF_FREQ(c->app->def_freq);

    for (uint i = 0 ; i < lib_shared_region->num_processes; i++) {
        sig_shared_region[i].new_freq = new_freq[0];
    }

    // TODO: Should the below code be inside USE_GPUS?
    if (gpus.node_policy_apply != NULL) {
        /* Basic support for GPUS */
        gpus.node_policy_apply(c, my_sig, freqs, (int *) &gpu_ready);
    }

    cpu_ready = EAR_POLICY_READY; // TODO: This variable is not used.
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
    domain->cpu     = POL_GRAIN_NODE;
    domain->mem     = POL_NOT_SUPPORTED;

    domain->gpu     = POL_NOT_SUPPORTED;

    domain->gpumem  = POL_NOT_SUPPORTED;

    return EAR_SUCCESS;
}


state_t policy_mpi_init(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
    if (is_mpi_enabled()) {
        cpi_read_diff(no_ctx, &cpi_2, &cpi_1, &cpi_diff, &comp_cpi);
        comp_cycles += cpi_diff.cycles;
        comp_insts += cpi_diff.instructions;
    }
    if (c != NULL) {
        return mpi_call_init(c, call_type);
    }
    return EAR_ERROR;
}


state_t policy_mpi_end(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
    if (is_mpi_enabled()) {
        cpi_read(no_ctx, &cpi_1);
    }
    if (c != NULL) {
        return mpi_call_end(c,call_type);
    }
    return EAR_ERROR;
}
