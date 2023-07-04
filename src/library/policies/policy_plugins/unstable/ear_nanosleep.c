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
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>

#define DEBUG_LVL 0
#define INFO_LVL  DEBUG_LVL

#define NANOSLEEP 0 // controls whether to use nanosleep vs usleep

static uint policy_loaded;
static uint optimize_call;
static ulong sleep_time_ns = 3000000; // 3ms

state_t policy_init(polctx_t *c)
{
    if (c != NULL && !policy_loaded) {

        if (is_mpi_enabled()) {

            mpi_app_init(c);

            uint block_type;
            is_blocking_busy_waiting(&block_type);
            verbose_master(INFO_LVL, "Busy waiting MPI: %u", block_type == BUSY_WAITING_BLOCK);

            if (ear_my_rank % 4) {

                optimize_call = 1;

                char *sleep_time_ns_env = ear_getenv("SLURM_EAR_SLEEP_TIME_NS");
                if (sleep_time_ns_env) {
                    sleep_time_ns = (ulong) strtol(sleep_time_ns_env, (char **)NULL, 10);
                }

                verbose(INFO_LVL, "MPI rank %d will be optimized on MPI blocking calls.\nSleep time (ns): %lu", ear_my_rank, sleep_time_ns);
            } else {
                verbose(INFO_LVL, "MPI rank %d won't be optimized on MPI blocking calls.", ear_my_rank);
            }

            policy_loaded = 1;
        }

        return EAR_SUCCESS;

    } else if (!c) {
        return_msg(EAR_ERROR, Generr.input_null);
    } else {
        return_msg(EAR_ERROR, Generr.api_initialized);
    }
}


state_t policy_end(polctx_t *c)
{
    if (c != NULL) {
        return mpi_app_end(c);
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }
}


state_t policy_mpi_init(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{

    if (c) {

        state_t st;

        if ((st = mpi_call_init(c, call_type)) == EAR_SUCCESS) {

            if (optimize_call && is_mpi_blocking(call_type)) {

#if NANOSLEEP
                struct timespec mpi_opt_timer = {.tv_nsec = sleep_time_ns};

                nanosleep(&mpi_opt_timer, NULL);
#else
                usleep(sleep_time_ns / 1000);
#endif
            }

        } else {
            return st;
        }

    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    return EAR_SUCCESS;
}

state_t policy_mpi_end(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{
    if (c) {
        return mpi_call_end(c, call_type);
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }
}
