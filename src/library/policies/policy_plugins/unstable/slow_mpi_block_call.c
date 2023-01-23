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

#include <library/policies/policy_ctx.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/common/imc_policy_support.h>
#include <library/policies/common/cpu_support.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/metrics/metrics.h>

#include <metrics/common/pstate.h>

#define DEBUG_LVL 0
#define INFO_LVL  DEBUG_LVL

static uint policy_loaded;
static uint optimize_call;

static uint resume_cpufreq;

static ulong resume_cpufreq_khz;
static ullong mpi_freq_khz;

static node_freqs_t def_freqs;

extern uint *imc_max_pstate;
extern uint *imc_min_pstate;
extern uint imc_devices;

state_t policy_init(polctx_t *c)
{
    const metrics_t *mgt_cpufreq = metrics_get(MGT_CPUFREQ);

    if (c != NULL && !policy_loaded && mgt_cpufreq->ok) {

        /* Configuring default freqs */
        node_freqs_alloc(&def_freqs);

        for (int i = 0; i < lib_shared_region->num_processes; i++) {
            def_freqs.cpu_freq[i] = *(c->ear_frequency);
        }

        for (int i = 0; i < imc_devices; i++) {
            def_freqs.imc_freq[i*IMC_VAL+IMC_MAX] = imc_min_pstate[i];
            def_freqs.imc_freq[i*IMC_VAL+IMC_MIN] = imc_max_pstate[i];
        }

        if (is_mpi_enabled()) {

            mpi_app_init(c);

            uint block_type;
            is_blocking_busy_waiting(&block_type);

            verbose_master(INFO_LVL, "Busy waiting MPI: %u",
                           block_type == BUSY_WAITING_BLOCK);

            if (ear_my_rank % 4) {

                optimize_call = 1;

                char *mpi_pstate_env = getenv("SLURM_EAR_MPI_PSTATE");

                if (mpi_pstate_env) {

                    int mpi_pstate = (int) strtol(mpi_pstate_env, (char **)NULL, 10);
                    if (mpi_pstate > 0) {

                        if (state_fail(pstate_pstofreq((pstate_t *) mgt_cpufreq->avail_list,
                                       mgt_cpufreq->avail_count, (uint) mpi_pstate, &mpi_freq_khz))) {

                                return_msg(EAR_ERROR, "converting p-state to freq");
                        }

                        resume_cpufreq_khz = *(c->ear_frequency);
                    } else {
                        return_msg(EAR_ERROR, Generr.arg_outbounds);
                    }
                }

                verbose(INFO_LVL, "MPI rank %d will be optimized on MPI blocking calls.\nMPI call freq (kHz): %llu\nResume freq (kHz): %lu",
                        ear_my_rank, mpi_freq_khz, resume_cpufreq_khz);
            } else {
                verbose(INFO_LVL, "MPI rank %d won't be optimized on MPI blocking calls.", ear_my_rank);
            }

            policy_loaded = 1;
        }

        return EAR_SUCCESS;

    } else if (!c) {
        return_msg(EAR_ERROR, Generr.context_null);
    } else if (policy_loaded) {
        return_msg(EAR_ERROR, Generr.api_initialized);
    } else {
        return_msg(EAR_ERROR, Generr.api_uninitialized);
    }
}


state_t policy_end(polctx_t *c)
{
    if (policy_loaded) {
        if (c != NULL) {
            return mpi_app_end(c);
        } else {
            return_msg(EAR_ERROR, Generr.context_null);
        }
    } else {
        return_msg(EAR_ERROR, Generr.api_uninitialized);
    }
}


state_t policy_mpi_init(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{

    if (policy_loaded) {

        if (c) {

            state_t st;

            if ((st = mpi_call_init(c, call_type)) == EAR_SUCCESS) {

                if (optimize_call && is_mpi_blocking(call_type)) {
                    
                    freqs->cpu_freq[my_node_id] = (ulong) mpi_freq_khz;

                    resume_cpufreq = 1;

                    *process_id = my_node_id;
                }
            } else {
                return_msg(EAR_ERROR, "getting MPI stats.");
            }

        } else {
            return_msg(EAR_ERROR, Generr.context_null);
        }
    } else {
        return_msg(EAR_ERROR, Generr.api_uninitialized);
    }

    return EAR_SUCCESS;
}

state_t policy_mpi_end(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
    if (policy_loaded) {
        if (c) {
            if (resume_cpufreq) {

                freqs->cpu_freq[my_node_id] = resume_cpufreq_khz;
                
                *process_id = my_node_id;

                resume_cpufreq = 0;

                return mpi_call_end(c, call_type);
            }
        } else {
            return_msg(EAR_ERROR, Generr.context_null);
        }
    } else {
        return_msg(EAR_ERROR, Generr.api_uninitialized);
    }

    return EAR_SUCCESS;
}

state_t policy_domain(polctx_t *c, node_freq_domain_t *domain)
{
    if (domain) {

        domain->cpu = POL_GRAIN_CORE;
        domain->mem = POL_NOT_SUPPORTED;

        domain->gpu    = POL_NOT_SUPPORTED;
        domain->gpumem = POL_NOT_SUPPORTED;

    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}

state_t policy_get_default_freq(polctx_t *c, node_freqs_t *freq_set, signature_t *s)
{
    if (freq_set) {
        node_freqs_copy(freq_set, &def_freqs);
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}
