/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if !SHOW_DEBUGS
#define NDEBUG // Disable asserts
#endif

#include <assert.h>

#include <common/config.h>
#include <common/config/config_dev.h>
#include <common/environment.h>
#include <common/hardware/hardware_info.h>
#include <common/math_operations.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/execute.h>
#include <common/system/monitor.h>
#include <common/system/time.h>
#include <management/cpufreq/cpufreq.h>
#include <metrics/cpi/cpi.h>

#include <common/system/lock.h>

#include <library/api/mpi_support.h>
#include <library/common/externs.h>
#include <library/common/global_comm.h>
#include <library/common/library_shared_data.h>
#include <library/common/verbose_lib.h>
#include <library/loader/module_mpi.h>
#include <library/metrics/metrics.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/policies/policy_ctx.h>
#include <library/tracer/tracer.h>
#include <library/tracer/tracer_paraver_comp.h>

#include <report/report.h>

#define MASTER_ID masters_info.my_master_rank

/* Verbose levels */
#define DEBUG_LVL          3
#define ERROR_LVL          1
#define WARN_LVL           2
#define INFO_LVL           WARN_LVL
#define MPI_SAMPLING_LVL   2

#define LB_VERB_LVL        3
#define MPI_STATS_VERB_LVL INFO_LVL
#define VAR_DEPRECATED     WARN_LVL - 1 // It's a warning, but more prioritary

#define PER_MPI_CALL_STATS                                                                                             \
    1 // This macro enables the stats computation on each MPI call.
      // You can disable it for overhead testing.

#if MPI && PER_MPI_CALL_STATS
static timestamp pol_time_init;
#endif

static timestamp start_no_mpi;

static mpi_information_t mpi_stats;

#define MPI_SAMPLING_IDLE 0
#if MPI_SAMPLING
#define OFF                  0
#define ON                   1
#define MPI_SAMPLING_ENABLED 1
#define MPI_MONITOR_DISABLED 2
static unsigned long long int mpis_per_sample      = 1;
static unsigned long long int curr_mpis_per_sample = 0;
static uint dynamic_monitor_state                  = MPI_SAMPLING_IDLE;
static uint mpi_sampling_enabled                   = 1;
// static unsigned long long int next_mpi_call_to_enable = 0;
// static unsigned long long int mpis_calls_before_reevaluation = 0;

#else
static uint dynamic_monitor_state = MPI_SAMPLING_IDLE;
// static uint mpi_sampling_enabled = 0;
#endif
static uint monitor_mpi                       = 1;
static uint verb                              = 1;
static unsigned long long int total_mpi_stats = 0;

#if MPI_STATS
static ulong total_mpi_time_blocking    = 0;
static ulong total_mpi_time_sync        = 0;
static ulong total_mpi_time_collectives = 0;
static ulong total_blocking_calls       = 0;
static ulong total_collectives          = 0;
static ulong total_synchronizations     = 0;
static ulong max_sync_block             = 0;

/* Statistics of all call types */
static uint mpi_call_count[N_CALL_TYPES];
static ullong mpi_call_time[N_CALL_TYPES];

static state_t mpi_call_types_read_diff(mpi_calls_types_t *diff, mpi_calls_types_t *last, int i);
static uint get_mpi_stats = 0;
static char mpi_stats_prefix[MAX_PATH_SIZE];
#endif // MPI_STATS

#if PER_MPI_CALL_STATS && MPI
static int in_mpi_call;
#endif

float LB_TH = EAR_LB_TH;

static float critical_path_th = 1;

extern uint limit_exceeded;
#if MPI_OPTIMIZED

static uint barrier_optimize     = 0;
static uint must_be_restored     = 0;
static uint num_steps            = 0;
static uint selected_steps       = 0;
static uint num_pstates          = 0;
static uint block_ready          = 0;
static uint curr_mpi_opt_index   = 0;
static uint target_mpi_opt_index = 0;
extern uint ear_mpi_opt_num_nodes;
extern sem_t *lib_shared_lock_sem;
extern suscription_t *earl_mpi_opt;

// TODO: Below macros should be defined at config_def or config_dev
#define MAX_MPI_TO_OPT   50    // Max number of calls to try to optimize
#define DEF_MIN_USEC_MPI 50000 // Default minimum time of a call to try to optimize
pthread_mutex_t ear_mpi_opt_lock = PTHREAD_MUTEX_INITIALIZER;

ulong local_max_sync_block = 0;

extern uint ear_mpi_opt; // Whether this module must to try to optimize some MPI calls.
#if MPI
extern p2i last_buf, last_dest; // Defined at src/library/api/ear_mpi.c
#endif

typedef struct ear_calls {
    mpi_call call;
    p2i last_buf;
    p2i last_dest;
    ulong elap;
    uint cnt;
} ear_calls_t;

static ear_calls_t list_mpi_to_optimize[MAX_MPI_TO_OPT];

static uint num_mpi_to_optimize = 0;

static uint index_last_optimized, last_optimized = 0;
static ulong ear_min_usec_for_mpi_opt = DEF_MIN_USEC_MPI;
#endif // MPI_OPTIMIZED

/*  Computes the the max and min process with perc mpi calls from percs_mpi */
static state_t mpi_support_perc_mpi_max_min(int nump, double *node_mpi_calls, int *max, int *min);

/**  Computes basic statistics of percs_mpi */
static state_t mpi_stats_get_only_percs_mpi(const mpi_information_t *node_mpi_calls, int nump, double *percs_mpi);

static state_t mpi_stats_evaluate_sim_uint(uint *current_cp, uint *last_cp, size_t size, double *sim);

static state_t mpi_stats_evaluate_mean_sd_mag(int nump, double *percs_mpi, double *mean, double *sd, double *mag);

static state_t mpi_stats_evaluate_median(int nump, double *percs_mpi, double *median);

#if MPI_STATS
static state_t build_mpi_ear_stats_filenames(char *prefix, char *mpi_stats_fn, char *mpi_calls_stats_fn);
#endif

state_t mpi_app_init(polctx_t *c)
{
#if MPI_OPTIMIZED
    char *cear_min_usec_for_mpi_opt = ear_getenv(
        FLAG_MIN_USEC_MPI); // Modifies the minimum time a call must spend in order to be a candidate to be optimized.

    if (cear_min_usec_for_mpi_opt != NULL)
        ear_min_usec_for_mpi_opt = (ulong) atoi(cear_min_usec_for_mpi_opt);
    verbose_master(2, "Minimum MPI call time to be optimized: %lu us", ear_min_usec_for_mpi_opt);
#endif

    if (c != NULL) {

        char *clb_th; // Modifies the default Load Balance efficiency threshold.

        if ((clb_th = ear_getenv(FLAG_LOAD_BALANCE_TH)) != NULL) {
            LB_TH = strtof(clb_th, NULL); // (float)atoi(clb_th);
        }

        verbose_master(MPI_STATS_VERB_LVL, "Load Balance threshold: %.1f. Critical path proximity %.1f", LB_TH,
                       critical_path_th);

        sig_shared_region[my_node_id].mpi_info.mpi_time        = 0;
        sig_shared_region[my_node_id].mpi_info.total_mpi_calls = 0;
        sig_shared_region[my_node_id].mpi_info.exec_time       = 0;
        sig_shared_region[my_node_id].mpi_info.perc_mpi        = 0;

        sig_shared_region[my_node_id].mpi_types_info.mpi_sync_call_cnt    = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_collec_call_cnt  = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_block_call_cnt   = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_sync_call_time   = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_collec_call_time = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_block_call_time  = 0;

        /* Gobal mpi statistics */
        mpi_stats.mpi_time        = 0;
        mpi_stats.total_mpi_calls = 0;
        mpi_stats.exec_time       = 0;
        mpi_stats.perc_mpi        = 0;

#if MPI_STATS
        char *stats = ear_getenv(FLAG_GET_MPI_STATS);
        if (stats != NULL) {
            get_mpi_stats = 1;
            strncpy(mpi_stats_prefix, stats, sizeof(mpi_stats_prefix));
        }
#endif

        // TODO: This is redundant since policies call this function only if mpi is enabled
        if (module_mpi_is_enabled()) {
            mpi_stats.rank = sig_shared_region[my_node_id].mpi_info.rank;
        } else {
            mpi_stats.rank = 0;
        }
#if MPI_SAMPLING
        char *env_mpi_sampling_enabled;
        if (module_mpi_is_enabled() && ((env_mpi_sampling_enabled = ear_getenv(FLAG_MPI_SAMPLING_ENABLED)) != NULL)) {
            mpi_sampling_enabled = atoi(env_mpi_sampling_enabled);
        }
        verbose_master(2, "MPI support: MPI sampling set to %s by env var ",
                       (mpi_sampling_enabled ? "Enabled" : "Disabled"));
#endif

#if MPI_OPTIMIZED
        timestamp_getprecise(&start_no_mpi);
#else
        timestamp_getfast(&start_no_mpi);
#endif

    } else {
        return EAR_ERROR;
    }

    return EAR_SUCCESS;
}

state_t mpi_app_end(polctx_t *c)
{
    timestamp end;

#if MPI_OPTIMIZED
    if (num_mpi_to_optimize) {

        for (uint mo = 0; mo < num_mpi_to_optimize; mo++) {

            verbose(2, "MPI_OPTIMI[%u-%u de %u] call %lu buff %lu dest %lu elap %lu", my_node_id, mo,
                    num_mpi_to_optimize, (ulong) list_mpi_to_optimize[mo].call,
                    (ulong) list_mpi_to_optimize[mo].last_buf, (ulong) list_mpi_to_optimize[mo].last_dest,
                    list_mpi_to_optimize[mo].elap);
        }
    }
#endif

#if MPI_STATS
    if (VERB_ON(INFO_LVL + 1)) {

        for (int i = 0; i < N_CALL_TYPES; i++) {

            if (mpi_call_count[i]) {

                verbose(INFO_LVL + 1, "[%d] CALL %d Time %llu instances %d avg %lf", my_node_id, i, mpi_call_time[i],
                        mpi_call_count[i], (double) mpi_call_time[i] / (double) mpi_call_count[i]);
            }
        }
    }
#endif

    /* Global statistics */
#if MPI_OPTIMIZED
    timestamp_getprecise(&end);
#else
    timestamp_getfast(&end);
#endif

#if MPI_SAMPLING
    if (dynamic_monitor_state == MPI_SAMPLING_IDLE) {
        ullong elap = timestamp_diff(&end, &start_no_mpi, TIME_USECS);
        mpi_stats.exec_time += elap;
    }
#endif

    if (mpi_stats.total_mpi_calls && mpi_stats.exec_time) {
        mpi_stats.perc_mpi = (float) mpi_stats.mpi_time / (float) mpi_stats.exec_time;

#if MPI_STATS
        if (get_mpi_stats) {

            char mpi_stats_per_call_filename[256];
            char mpi_stats_filename[256];

            if (state_fail(
                    build_mpi_ear_stats_filenames(mpi_stats_prefix, mpi_stats_filename, mpi_stats_per_call_filename))) {
                verbose(MPI_STATS_VERB_LVL, "%sWARNING%s MPI stats files won't be created.", COL_RED, COL_CLR);
                return EAR_WARNING;
            }

            verbose_master(MPI_STATS_VERB_LVL, "      MPI stats file: %s\nMPI calls stats file: %s", mpi_stats_filename,
                           mpi_stats_per_call_filename);

            // Open mpi_stats_filename to store per process global MPI stats
            int fd = open(mpi_stats_filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);

            char mpi_stats_buff[512];
            char mpi_stats_buff_aux[64];

            if (fd < 0) {
                verbose(MPI_STATS_VERB_LVL, "%sWARNING%s MPI stats file couldn't have opened: %d", COL_YLW, COL_CLR,
                        errno);

                mpi_info_to_str(&mpi_stats, mpi_stats_buff, sizeof(mpi_stats_buff));
                verbose(MPI_STATS_VERB_LVL - 1, "MR[%d] %s", lib_shared_region->master_rank, mpi_stats_buff);
            } else {
                // Only the master writes the header
                if (masters_info.my_master_rank >= 0) {
                    char mpi_stats_hdr_buff_aux[64];
                    mpi_info_head_to_str_csv(mpi_stats_hdr_buff_aux, sizeof(mpi_stats_hdr_buff_aux));
                    snprintf(mpi_stats_buff, sizeof mpi_stats_buff, "mrank;pid;%s\n", mpi_stats_hdr_buff_aux);
                    write(fd, mpi_stats_buff, strlen(mpi_stats_buff));
                }

                mpi_info_to_str_csv(&mpi_stats, mpi_stats_buff_aux, sizeof(mpi_stats_buff_aux));

                sprintf(mpi_stats_buff, "%d;%d;%s\n", lib_shared_region->master_rank, sig_shared_region[my_node_id].pid,
                        mpi_stats_buff_aux);

                write(fd, mpi_stats_buff, strlen(mpi_stats_buff));
                close(fd);
            }

            // Open mpi_stats_per_call_filename to store per process MPI calls stats
            fd = open(mpi_stats_per_call_filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);

            if (fd >= 0) {
                // Only the master writes the header
                if (mpi_stats.rank == 0) {
                    snprintf(mpi_stats_buff, sizeof(mpi_stats_buff),
                             "Master;Rank;Total MPI calls;MPI_time/Exec_time;Exec_time;Sync_time;"
                             "Block_time;Collec_time;Total MPI sync calls;Total blocking calls;"
                             "Total collective calls;Gather;Reduce;All2all;Barrier;Bcast;Send;Comm;Receive;"
                             "Scan;Scatter;SendRecv;Wait;t_Gather;t_Reduce;t_All2all;t_Barrier;t_Bcast;"
                             "t_Send;t_Comm;t_Receive;t_Scan;t_Scatter;t_SendRecv;t_Wait\n");
                    write(fd, mpi_stats_buff, strlen(mpi_stats_buff));
                }

                sprintf(mpi_stats_buff,
                        "%d;%d;%llu;%.4f;%llu;%lu;%lu;%lu;%lu;%lu;%lu;%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;"
                        "%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu\n",
                        lib_shared_region->master_rank, mpi_stats.rank, mpi_stats.total_mpi_calls, mpi_stats.perc_mpi,
                        mpi_stats.exec_time, total_mpi_time_sync, total_mpi_time_blocking, total_mpi_time_collectives,
                        total_synchronizations, total_blocking_calls, total_collectives, mpi_call_count[GATHER],
                        mpi_call_count[REDUCE], mpi_call_count[ALL2ALL], mpi_call_count[BARRIER], mpi_call_count[BCAST],
                        mpi_call_count[SEND], mpi_call_count[COMM], mpi_call_count[RECEIVE], mpi_call_count[SCAN],
                        mpi_call_count[SCATTER], mpi_call_count[SENDRECV], mpi_call_count[WAIT], mpi_call_time[GATHER],
                        mpi_call_time[REDUCE], mpi_call_time[ALL2ALL], mpi_call_time[BARRIER], mpi_call_time[BCAST],
                        mpi_call_time[SEND], mpi_call_time[COMM], mpi_call_time[RECEIVE], mpi_call_time[SCAN],
                        mpi_call_time[SCATTER], mpi_call_time[SENDRECV], mpi_call_time[WAIT]);
                write(fd, mpi_stats_buff, strlen(mpi_stats_buff));
            }
        }
#endif // MPI_STATS
    }

    return EAR_SUCCESS;
}

void monitor_mpi_calls(uint on)
{
    if (on) {
        monitor_mpi = 1;
        timestamp_getfast(&start_no_mpi);
        verb = 1;
    } else {
        if (verb) {
            verb = 0;
        }
        monitor_mpi = 0;
    }
}

uint is_monitor_mpi_calls_enabled()
{
#if PER_MPI_CALL_STATS && MPI

#if MPI_SAMPLING
    return (dynamic_monitor_state != MPI_MONITOR_DISABLED);
#else
    return 1;
#endif // MPI_SAMPLING
#else
    return 0;
#endif // PER_MPI_CALL_STATS && MPI
}

uint is_low_mpi_activity()
{
    if (dynamic_monitor_state == MPI_SAMPLING_IDLE) {
        if (avg_mpi_calls_per_second() < MIN_MPI_CALLS_SECOND)
            return 1;
    }
    return 0;
}

#if MPI
float avg_mpi_calls_per_second()
{

    float seconds = (float) (mpi_stats.exec_time / 1000000);
    if (seconds == 0)
        return (float) mpi_stats.total_mpi_calls;
    float curr_mpi_per_second = mpi_stats.total_mpi_calls / seconds;

    return curr_mpi_per_second;
}
#else
float avg_mpi_calls_per_second()
{
    return 0.0;
}
#endif // MPI

unsigned long long int total_mpi_call_statistics()
{
    return total_mpi_stats;
}

state_t mpi_call_init(polctx_t *c, mpi_call call_type)
{
#if PER_MPI_CALL_STATS && MPI

#if MPI_STATS
    int collect_stats = (module_mpi_is_enabled() && (monitor_mpi || get_mpi_stats));
#else
    int collect_stats = (module_mpi_is_enabled() && monitor_mpi);
#endif // MPI_STATS

    if (collect_stats) {
        // mpis_calls_before_reevaluation++;

        // Init the MPI call timer
#if MPI_OPTIMIZED
        timestamp_getprecise(&pol_time_init);
#else
        timestamp_getfast(&pol_time_init);
#endif

        // Get the time not been in MPI
        ullong time_no_mpi = timestamp_diff(&pol_time_init, &start_no_mpi, TIME_USECS);

        // Accumulate
        mpi_stats.exec_time += time_no_mpi;

        in_mpi_call = 1;

#if SHOW_DEBUGS
        if ((sig_shared_region[my_node_id].mpi_info.total_mpi_calls % 100) == 0) {
            debug("P[%d] (sec %ld nsec %ld) - (sec %ld nsec %ld) %llu not_mpi_elap", my_node_id, pol_time_init.tv_sec,
                  pol_time_init.tv_nsec, start_no_mpi.tv_sec, start_no_mpi.tv_nsec, time_no_mpi);
        }
#endif // SHOW_DEBUGS
    }
#endif // PER_MPI_CALL_STATS
    return EAR_SUCCESS;
}

uint is_bbarrier(mpi_call call)
{
    if (is_mpi_blocking(call)) {

        mpi_call nbcall_type = (mpi_call) remove_blocking(call);

        return (is_barrier(nbcall_type));

        // return (is_synchronization(nbcall_type));
    }

    return 0;
}

uint is_bgather(mpi_call call)
{
    if (is_mpi_blocking(call)) {

        mpi_call nbcall_type = (mpi_call) remove_blocking(call);

        return is_gather(nbcall_type);
    }

    return 0;
}

state_t mpi_call_end(polctx_t *c, mpi_call call_type)
{
#if PER_MPI_CALL_STATS && MPI
    timestamp_t end;
    if (module_mpi_is_enabled() && in_mpi_call) {

        ullong elap = 0;

#if MPI_OPTIMIZED
        timestamp_getprecise(&end);
#else
        timestamp_getfast(&end);
#endif

        total_mpi_stats++;

        elap        = timestamp_diff(&end, &pol_time_init, TIME_USECS); // Get the time been in the MPI call
        in_mpi_call = 0;

        /* Global statistics */
        mpi_stats.mpi_time += elap;
        mpi_stats.exec_time += elap;
        mpi_stats.total_mpi_calls++;

        sig_shared_region[my_node_id].mpi_info.mpi_time        = mpi_stats.mpi_time;
        sig_shared_region[my_node_id].mpi_info.exec_time       = mpi_stats.exec_time;
        sig_shared_region[my_node_id].mpi_info.total_mpi_calls = mpi_stats.total_mpi_calls;

#if MPI_STATS
        if (get_mpi_stats) {
            debug("MPI END P[%d] sec %ld nsec %ld", my_node_id, end.tv_sec, end.tv_nsec);

            mpi_call nbcall_type;

            // Only collect stats for blocking calls
            if (is_mpi_blocking(call_type)) {

                total_blocking_calls++;
                total_mpi_time_blocking += (ulong) elap;

                max_sync_block = ear_max(max_sync_block, (ulong) elap);
#if MPI_OPTIMIZED
                local_max_sync_block = ear_max(local_max_sync_block, (ulong) elap);
#endif

                // Get which type of blocking call is
                nbcall_type = (mpi_call) remove_blocking(call_type);

                mpi_call_count[nbcall_type]++;
                mpi_call_time[nbcall_type] += elap;

                assert(mpi_call_count[nbcall_type] != 0);

                if (is_collective(nbcall_type)) {
                    // Collective
                    total_collectives++;
                    total_mpi_time_collectives += (ulong) elap;

                } else if (is_synchronization(nbcall_type)) {
                    // Sync
                    total_synchronizations++;
                    total_mpi_time_sync += (ulong) elap;
                }

                /* We check if the call must be on the list */
#if MPI_OPTIMIZED
                if (ear_mpi_opt) {
                    if (last_optimized) {
                        list_mpi_to_optimize[index_last_optimized].elap += (ulong) elap;
                        list_mpi_to_optimize[index_last_optimized].cnt++;
                        if ((ulong) elap < ear_min_usec_for_mpi_opt) {
                            list_mpi_to_optimize[index_last_optimized].elap = (ulong) elap;
                            list_mpi_to_optimize[index_last_optimized].call = (mpi_call) 0;
                            list_mpi_to_optimize[index_last_optimized].cnt  = 0;
                        }
                    } else {
                        /* Otherwise, we check if we should add it */
#define FIRST_NO_EAR_SYMBOL 4
                        if ((ulong) elap > ear_min_usec_for_mpi_opt && num_mpi_to_optimize < MAX_MPI_TO_OPT) {
                            if (already_optimized(nbcall_type, last_buf, last_dest) == EAR_ERROR) {

                                list_mpi_to_optimize[num_mpi_to_optimize].call = nbcall_type;

                                list_mpi_to_optimize[num_mpi_to_optimize].last_buf  = last_buf;
                                list_mpi_to_optimize[num_mpi_to_optimize].last_dest = last_dest;
                                list_mpi_to_optimize[num_mpi_to_optimize].elap      = (ulong) elap;
                                list_mpi_to_optimize[num_mpi_to_optimize].cnt       = 1;

                                num_mpi_to_optimize++;
                                // print_stack(verb_channel);
                            }
                        }
                    }

                    last_optimized = 0;
                }
#endif // MPI_OPTIMIZED
            }

            sig_shared_region[my_node_id].mpi_types_info.mpi_sync_call_cnt    = total_synchronizations;
            sig_shared_region[my_node_id].mpi_types_info.mpi_collec_call_cnt  = total_collectives;
            sig_shared_region[my_node_id].mpi_types_info.mpi_block_call_cnt   = total_blocking_calls;
            sig_shared_region[my_node_id].mpi_types_info.mpi_sync_call_time   = total_mpi_time_sync;
            sig_shared_region[my_node_id].mpi_types_info.mpi_collec_call_time = total_mpi_time_collectives;
            sig_shared_region[my_node_id].mpi_types_info.mpi_block_call_time  = total_mpi_time_blocking;
            sig_shared_region[my_node_id].mpi_types_info.max_sync_block       = max_sync_block;
        }
#endif // MPI_STATS

#if SHOW_DEBUGS
        if ((sig_shared_region[my_node_id].mpi_info.total_mpi_calls % 100) == 0) {
            debug("P[%d] MPI %llu exec %llu %llu elap (sec %ld nsec %ld)", my_node_id,
                  sig_shared_region[my_node_id].mpi_info.mpi_time, sig_shared_region[my_node_id].mpi_info.exec_time,
                  elap, start_no_mpi.tv_sec, start_no_mpi.tv_nsec);
        }
#endif // SHOW_DEBUGS
    } else {
#if !MPI_OPTIMIZED
        timestamp_getfast(&end);
#endif
    }

#if MPI_OPTIMIZED
    timestamp_getprecise(&start_no_mpi);
#else
    start_no_mpi = end;
#endif

#if MPI_SAMPLING
    /* Dynamic MPI monitoring */

#if MPI_STATS
    int mpi_sampling_check =
        !get_mpi_stats && mpi_sampling_enabled && (dynamic_monitor_state == MPI_SAMPLING_IDLE) && limit_exceeded;
#else
    int mpi_sampling_check = mpi_sampling_enabled && (dynamic_monitor_state == MPI_SAMPLING_IDLE) && limit_exceeded;
#endif

    if (mpi_sampling_check) {
        float mpi_sec = avg_mpi_calls_per_second();

        if (mpi_sec > (MAX_MPI_CALLS_SECOND * 2)) {
            dynamic_monitor_state = MPI_MONITOR_DISABLED;
        } else {
            if (mpi_sec > 0) {
                mpis_per_sample = ear_max(1, mpi_sec / 10);
            } else {
                mpis_per_sample = MAX_MPI_CALLS_SECOND / 10;
            }

            curr_mpis_per_sample  = 0;
            dynamic_monitor_state = MPI_SAMPLING_ENABLED;
        }

        monitor_mpi_calls(OFF);

        verbose_master(MPI_SAMPLING_LVL, "%s: MPI limit exceeded %f > %f, dynamic monitor set to %s", node_name,
                       mpi_sec, (float) MAX_MPI_CALLS_SECOND,
                       (dynamic_monitor_state == MPI_SAMPLING_ENABLED) ? "SAMPLING" : "DISABLED");
    }
    if (dynamic_monitor_state == MPI_SAMPLING_ENABLED) {
        curr_mpis_per_sample++;

        mpis_per_sample = ear_max(mpis_per_sample, 1); // Ensure a value != 0

        if (((curr_mpis_per_sample / mpis_per_sample) % 10) == 0) {
            if (!monitor_mpi)
                monitor_mpi_calls(ON);
        } else {
            if (monitor_mpi) {
                monitor_mpi_calls(OFF);

                if (avg_mpi_calls_per_second() > (MAX_MPI_CALLS_SECOND * 2)) {
                    dynamic_monitor_state = MPI_MONITOR_DISABLED;

                    verbose_master(MPI_SAMPLING_LVL, "%s: MPI limit exceeded %f > %f, dynamic monitor set to %s",
                                   node_name, avg_mpi_calls_per_second(), (float) MAX_MPI_CALLS_SECOND,
                                   (dynamic_monitor_state == MPI_SAMPLING_ENABLED) ? "SAMPLING" : "DISABLED");
                }
            }
        }
    }
#endif
#endif // PER_MPI_CALL_STATS

    return EAR_SUCCESS;
}

state_t mpi_call_get_stats(mpi_information_t *dst, mpi_information_t *src, int i)
{
    if (dst == NULL)
        return EAR_ERROR;

    if (src == NULL) {
        if (i < 0) {
            src = &sig_shared_region[my_node_id].mpi_info;
        } else {
            src = &sig_shared_region[i].mpi_info;
        }
    }
    memcpy(dst, src, sizeof(mpi_information_t));

    return EAR_SUCCESS;
}

state_t mpi_call_diff(mpi_information_t *diff, mpi_information_t *end, mpi_information_t *init)
{
    diff->total_mpi_calls = end->total_mpi_calls - init->total_mpi_calls;
    diff->exec_time       = end->exec_time - init->exec_time;
    diff->mpi_time        = end->mpi_time - init->mpi_time;
    return EAR_SUCCESS;
}

static state_t mpi_call_read_diff(mpi_information_t *diff, mpi_information_t *last, int i)
{
    mpi_information_t current;

    mpi_call_get_stats(&current, NULL, i);
    mpi_call_diff(diff, &current, last);

    debug("Process %d: mpi %llu = (%llu - %llu) exec %llu = (%llu - %llu)", i, diff->mpi_time, current.mpi_time,
          last->mpi_time, diff->exec_time, current.exec_time, last->exec_time);

    diff->perc_mpi = ((diff->exec_time > 0) ? (double) diff->mpi_time * 100.0 / (double) diff->exec_time : 0);

    // mpi_call_copy(last, &current);
    mpi_call_get_stats(last, &current, -1); // Copy from current to last
    last->perc_mpi = ((last->exec_time > 0) ? (double) last->mpi_time * 100.0 / (double) last->exec_time : 0);

    //  Check wether values are correctly bounded
    if (diff->perc_mpi < 0 || diff->perc_mpi > 100 || last->perc_mpi < 0 || last->perc_mpi > 100) {
        verbose_master(2, "%s[WARNING] Bad perc_mpi computation%s: diff = %lf / %lf = %lf, last = %lf / %lf = %lf",
                       COL_RED, COL_CLR, (double) diff->mpi_time * 100.0, (double) diff->exec_time, diff->perc_mpi,
                       (double) last->mpi_time * 100.0, (double) last->exec_time, last->perc_mpi);
        return EAR_WARNING;
    }
    return EAR_SUCCESS;
}

state_t mpi_call_get_types(mpi_calls_types_t *dst, shsignature_t *src, int i)
{
    mpi_information_t *src_stat;
    mpi_calls_types_t *src_types;

    if (dst == NULL)
        return EAR_ERROR;
    if (src == NULL) {
        if (i < 0) {
            src_types = &sig_shared_region[my_node_id].mpi_types_info;
            src_stat  = &sig_shared_region[my_node_id].mpi_info;
        } else {
            src_types = &sig_shared_region[i].mpi_types_info;
            src_stat  = &sig_shared_region[i].mpi_info;
        }
    } else {
        src_types = &src->mpi_types_info;
        src_stat  = &src->mpi_info;
    }

    memcpy(dst, src_types, sizeof(mpi_calls_types_t));

    dst->mpi_time     = src_stat->mpi_time;
    dst->exec_time    = src_stat->exec_time;
    dst->mpi_call_cnt = src_stat->total_mpi_calls;

    return EAR_SUCCESS;
}

state_t mpi_call_types_diff(mpi_calls_types_t *diff, mpi_calls_types_t *current, mpi_calls_types_t *last)
{

    diff->mpi_call_cnt         = current->mpi_call_cnt - last->mpi_call_cnt;
    diff->mpi_time             = current->mpi_time - last->mpi_time;
    diff->exec_time            = current->exec_time - last->exec_time;
    diff->mpi_sync_call_cnt    = current->mpi_sync_call_cnt - last->mpi_sync_call_cnt;
    diff->mpi_collec_call_cnt  = current->mpi_collec_call_cnt - last->mpi_collec_call_cnt;
    diff->mpi_block_call_cnt   = current->mpi_block_call_cnt - last->mpi_block_call_cnt;
    diff->mpi_sync_call_time   = current->mpi_sync_call_time - last->mpi_sync_call_time;
    diff->mpi_collec_call_time = current->mpi_collec_call_time - last->mpi_collec_call_time;
    diff->mpi_block_call_time  = current->mpi_block_call_time - last->mpi_block_call_time;
    diff->max_sync_block       = ear_max(current->max_sync_block, last->max_sync_block);

    return EAR_SUCCESS;
}

#if MPI_STATS
static state_t mpi_call_types_read_diff(mpi_calls_types_t *diff, mpi_calls_types_t *last, int i)
{
    mpi_calls_types_t current;

    mpi_call_get_types(&current, NULL, i);

    mpi_call_types_diff(diff, &current, last);
    memcpy(last, &current, sizeof(mpi_calls_types_t));
    return EAR_SUCCESS;
}
#endif // MPI_STATS

void verbose_mpi_data(int verb_lvl, mpi_information_t *mc)
{
    verbose_master(verb_lvl, "Total mpi calls %llu MPI time %llu exec time %llu perc mpi %.2lf", mc->total_mpi_calls,
                   mc->mpi_time, mc->exec_time, mc->perc_mpi);
}

void verbose_mpi_calls_types(int verb_lvl, mpi_calls_types_t *t)
{
#if MPI_STATS
    if (verb_lvl <= VERB_GET_LV() && t) {
        char perc_sync_time[16], perc_collec_time[16], perc_block_time[16], mpi_calls_per_sec[16], perc_mpi_time[16];
        if (t->exec_time == 0) {
            sprintf(perc_sync_time, "NAV");
            sprintf(perc_collec_time, "NAV");
            sprintf(mpi_calls_per_sec, "NAV");
            sprintf(perc_mpi_time, "NAV");
        } else {
            sprintf(perc_sync_time, "%.2lf", (float) t->mpi_sync_call_time * 100.0 / (float) t->exec_time);
            sprintf(perc_collec_time, "%.2lf", (float) t->mpi_collec_call_time * 100.0 / (float) t->exec_time);
            sprintf(perc_block_time, "%.2lf", (float) t->mpi_block_call_time * 100.0 / (float) t->exec_time);
            sprintf(mpi_calls_per_sec, "%.2lf", (float) t->mpi_call_cnt * 1000000.0 / (float) t->exec_time);
            sprintf(perc_mpi_time, "%.2lf", (float) t->mpi_time * 100.0 / (float) t->exec_time);
        }

        verbose_master(verb_lvl,
                       "MPI types\n---------\n\t# MPI calls: %lu\n\tMPI time (us): %lu\n\tExecution time (us): %lu\n"
                       "\t# Synchronization calls: %lu\n\t# Collective calls: %lu\n\t# Blocking calls: %lu\n\t%% Time "
                       "synchronization: %s\n"
                       "\t%% Time collective: %s\n\t%% Time blocking: %s\n\t# MPI calls/s: %s\n\t%% MPI time: %s\n"
                       "Max synch=%lu",
                       t->mpi_call_cnt, t->mpi_time, t->exec_time, t->mpi_sync_call_cnt, t->mpi_collec_call_cnt,
                       t->mpi_block_call_cnt, perc_sync_time, perc_collec_time, perc_block_time, mpi_calls_per_sec,
                       perc_mpi_time, t->max_sync_block);
    }
#endif // MPI_STATS
}

state_t read_diff_node_mpi_info(lib_shared_data_t *data, shsignature_t *sig, mpi_information_t *node_mpi_calls,
                                mpi_information_t *last_node)
{
    int i;
    int bad_read_diff = 0;
    if ((data == NULL) || (sig == NULL) || (node_mpi_calls == NULL) || (last_node == NULL)) {
        return EAR_ERROR;
    }
    for (i = 0; i < data->num_processes; i++) {
        if (state_fail(mpi_call_read_diff(&node_mpi_calls[i], &last_node[i], i))) {
            bad_read_diff = 1;
        }
    }
    if (bad_read_diff) {
        return EAR_WARNING;
    }
    return EAR_SUCCESS;
}

state_t read_diff_node_mpi_types_info(lib_shared_data_t *data, shsignature_t *sig, mpi_calls_types_t *node_mpi_calls,
                                      mpi_calls_types_t *last_node)
{
#if MPI_STATS

    if ((data == NULL) || (sig == NULL) || (node_mpi_calls == NULL) || (last_node == NULL)) {
        return EAR_ERROR;
    }
    for (int i = 0; i < data->num_processes; i++) {
        mpi_call_types_read_diff(&node_mpi_calls[i], &last_node[i], i);
    }
#endif
    return EAR_SUCCESS;
}

static state_t mpi_support_perc_mpi_max_min(int nump, double *percs_mpi, int *max, int *min)
{
    *max = 0;
    *min = 0;
    verbosen_master(LB_VERB_LVL, "Load balance data ");
    for (int i = 1; i < nump; i++) {
        verbosen_master(LB_VERB_LVL, "MR[%d]= %.2lf ", i, percs_mpi[i]);
        if (percs_mpi[i] > percs_mpi[*max])
            *max = i;
        if (percs_mpi[i] < percs_mpi[*min])
            *min = i;
    }
    verbose_master(LB_VERB_LVL, " ");
    return EAR_SUCCESS;
}

#if SHARED_MPI_SUMMARY
/* This function sets the values to be shared with other nodes */
static mpi_summary_t my_mpi_summary;

static state_t mpi_support_set_mpi_summary(mpi_summary_t *mpisumm, const double *percs_mpi, int num_procs, double mean,
                                           double sd, double mag)
{
    double max, min;
    max = 0.0;
    min = 100.0;
    int i;
    for (i = 0; i < num_procs; i++) {
        min = ear_min(min, percs_mpi[i]);
        max = ear_max(max, percs_mpi[i]);
    }
    mpisumm->max  = (float) max;
    mpisumm->min  = (float) min;
    mpisumm->sd   = (float) sd;
    mpisumm->mean = (float) mean;
    mpisumm->mag  = (float) mag;
    return EAR_SUCCESS;
}
#endif // SHARED_MPI_SUMMARY

state_t mpi_support_evaluate_lb(const mpi_information_t *node_mpi_calls, int num_procs, double *percs_mpi, double *mean,
                                double *sd, double *mag, uint *lb)
{
    /* Get only the percentage time spent in MPI blocking calls. */
    state_t ret;

    if (!is_monitor_mpi_calls_enabled()) {
        *lb = 0;
        /* Add verbose here */
        return EAR_SUCCESS;
    }

    // Get only perc_mpi info.
    ret = mpi_stats_get_only_percs_mpi(node_mpi_calls, num_procs, percs_mpi); // Check whether perc_mpi is ok

    /*  Compute mean, standard deviation and magnitude of percs_mpi vector */
    mpi_stats_evaluate_mean_sd_mag(num_procs, percs_mpi, mean, sd, mag);

#if SHARED_MPI_SUMMARY
    mpi_support_set_mpi_summary(&my_mpi_summary, percs_mpi, num_procs, *mean, *sd, *mag);
#endif

#if NODE_LB
    ullong max_tcomp = node_mpi_calls[0].exec_time - node_mpi_calls[0].mpi_time;
    double avg_tcomp = (double) max_tcomp;

    for (int i = 1; i < num_procs; i++) {
        ullong current_tcomp =
            node_mpi_calls[i].exec_time - node_mpi_calls[i].mpi_time; // Current time doing computation
        verbose_master(LB_VERB_LVL, "tcomp (l. rank) %d %llu - %llu = %llu | max_tcomp %llu ", i,
                       node_mpi_calls[i].exec_time, node_mpi_calls[i].mpi_time, current_tcomp, max_tcomp);

        max_tcomp = ear_max(current_tcomp, max_tcomp);

        avg_tcomp += current_tcomp;
    }
    avg_tcomp /= num_procs;

    double lb_eff = avg_tcomp / (double) max_tcomp; // Compute the load balance efficiency following the POP model.

    *lb = (lb_eff < LB_TH);
    verbose_master(LB_VERB_LVL,
                   "%sLoad Balance efficiency%s %lf %smax tcomp%s %llu %savg tcomp%s %lf %sLB_TH%s %lf (unbalanced %u)",
                   COL_YLW, COL_CLR, lb_eff, COL_YLW, COL_CLR, max_tcomp, COL_YLW, COL_CLR, avg_tcomp, COL_YLW, COL_CLR,
                   LB_TH, *lb);
#else
    *lb = 0;
#endif // NODE_LB

    return ret;
}

state_t mpi_support_select_critical_path(uint *critical_path, double *percs_mpi, int num_procs, double mean,
                                         double *median, int *max_mpi, int *min_mpi)
{
    uint total_cp = 0;

    /* Computes max and min perc_mpi processes */
    mpi_support_perc_mpi_max_min(num_procs, percs_mpi, max_mpi, min_mpi);

    /* Computes median */
    mpi_stats_evaluate_median(num_procs, percs_mpi, median);

    memset(critical_path, 0, sizeof(uint) * num_procs);
    verbose_master(LB_VERB_LVL + 1, "%sPer-core CPU freq. selection%s", COL_RED, COL_CLR);

    for (uint i = 0; i < num_procs; i++) {
        /*  Decides whether process i is part of the critical path */
        if (fabs(percs_mpi[i] - percs_mpi[*min_mpi]) < (fabs(percs_mpi[i] - *median) * critical_path_th)) {
            verbose_master(LB_VERB_LVL, "Process %d selected as part of the critical path", i);
            critical_path[i] = 1;
            total_cp++;
        } else if (fabs(percs_mpi[i] - percs_mpi[*min_mpi]) < fabs(percs_mpi[i] - mean)) {
            debug("%sWARNING%s Proc %d is closer to the minimum than the mean, but not selected as part of the "
                  "critical path",
                  COL_RED, COL_CLR, i);
        }
    }
    verbose_master(2, "Total processes selected as critical path %u. Max MPI %.2f Min MPI %.2f Mean %.2f", total_cp,
                   percs_mpi[*max_mpi], percs_mpi[*min_mpi], mean);

    return EAR_SUCCESS;
}

state_t mpi_support_verbose_perc_mpi_stats(int verb_lvl, double *percs_mpi, int num_procs, double mean, double median,
                                           double sd, double mag)
{
    if (verb_lvl == 3) {
        debug("Previous percs_mpi:");
        for (int i = 0; i < num_procs; i++) {
            debug("MR[%d]=%lf", i, percs_mpi[i]);
        }
    } else if (verb_lvl <= 3) {
        debug("PREVIOUS mean %lf median %lf standard deviation %lf magnitude %lf", mean, median, sd, mag);
    }

    return EAR_SUCCESS;
}

state_t mpi_support_mpi_changed(double current_mag, double prev_mag, uint *cp, uint *prev_cp, int num_procs,
                                double *similarity, uint *mpi_changed)
{
    *mpi_changed = 0;

    /*  Check whether perc_mpi vector magnitude has increased with respect to previous one */
    if (!equal_with_th(prev_mag, current_mag, 0.05)) {
        if (prev_mag < current_mag) {
            debug("%sMore MPI than before%s (%.2lf/%.2lf)", COL_MGT, COL_CLR, prev_mag, current_mag);
            *mpi_changed = 1;
        }
    } else {
        /*  Check similarity between critical paths */
        mpi_stats_evaluate_sim_uint(cp, prev_cp, num_procs, similarity);

        char msg[] =
            "Similarity between last and current critical path selection %lf"; // The NULL character is appended by
                                                                               // default by the compiler.
        verbose_master(LB_VERB_LVL, msg, *similarity);

        if (*similarity < 0.5) {
            *mpi_changed = 1;
        }
    }
    return EAR_SUCCESS;
}

static state_t mpi_stats_get_only_percs_mpi(const mpi_information_t *node_mpi_calls, int nump, double *percs_mpi)
{
    double perc_mpi_check = 0; // Used ONLY for checking if perc_mpi > 0
    for (int i = 0; i < nump; i++) {
        percs_mpi[i] = node_mpi_calls[i].perc_mpi;
        perc_mpi_check += percs_mpi[i];
    }

    if (!perc_mpi_check) {
        return EAR_WARNING;
    }
    return EAR_SUCCESS;
#if 0
    const mpi_information_t **mpi_calls = (const mpi_information_t **) malloc(nump * sizeof(mpi_information_t*));

    for (int i = 0; i < nump; i++) {
        mpi_calls[i] = &node_mpi_calls[i];
    }
    void** percs_mpi_v = ear_math_apply((void**) mpi_calls, nump, mpi_info_get_perc_mpi);
    free(mpi_calls);

    double perc_mpi_check = 0; // Used ONLY for checking if perc_mpi > 0

    for (int i = 0; i < nump; i++) {
        percs_mpi[i] = *(double*)percs_mpi_v[i];
        perc_mpi_check += percs_mpi[i];
    }
    free(percs_mpi_v);

    if (perc_mpi_check == 0) {
        return EAR_WARNING;
    }
#endif
    return EAR_SUCCESS;
}

state_t mpi_stats_evaluate_mean(int nump, double *percs_mpi, double *mn)
{
    *mn = ear_math_mean(percs_mpi, nump);
    return EAR_SUCCESS;
}

state_t mpi_stats_evaluate_similarity(double *current_perc_mpi, double *last_perc_mpi, size_t size, double *similarity)
{
    *similarity = ear_math_cosine_similarity(current_perc_mpi, last_perc_mpi, size);
    return EAR_SUCCESS;
}

static state_t mpi_stats_evaluate_sim_uint(uint *current_cp, uint *last_cp, size_t size, double *sim)
{
    *sim = ear_math_cosine_sim_uint(current_cp, last_cp, size);
    return EAR_SUCCESS;
}

int compare_perc_mpi(const void *p1, const void *p2)
{
    return (*(double *const *) p1 < *(double *const *) p2)   ? -1
           : (*(double *const *) p1 > *(double *const *) p2) ? 1
                                                             : 0;
}

static state_t mpi_stats_evaluate_mean_sd_mag(int nump, double *percs_mpi, double *mean, double *sd, double *mag)
{
    if (nump == 0 || percs_mpi == NULL) {
        return EAR_ERROR;
    }

    mean_sd_t mean_sd_mag = ear_math_mean_sd(percs_mpi, nump);
    *mean                 = mean_sd_mag.mean;
    *sd                   = mean_sd_mag.sd;
    *mag                  = mean_sd_mag.mag;

    return EAR_SUCCESS;
}

static state_t mpi_stats_evaluate_median(int nump, double *percs_mpi, double *median)
{
    *median = ear_math_median(percs_mpi, nump);
    return EAR_SUCCESS;
}

state_t is_blocking_busy_waiting(uint *block_type)
{
    char *env;
    *block_type = BUSY_WAITING_BLOCK;
    if ((env = ear_getenv("I_MPI_WAIT_MODE")) != NULL) {
        verbose_master(2, "I_MPI_WAIT_MODE %s", env);
        if (atoi(env) != 0)
            *block_type = YIELD_BLOCK;
    }
    if ((env = ear_getenv("I_MPI_THREAD_YIELD")) != NULL) {
        verbose_master(2, "I_MPI_THREAD_YIELD %s", env);
        if (atoi(env) != 0)
            *block_type = YIELD_BLOCK;
    }
    if ((env = ear_getenv("OMPI_MCA_mpi_yield_when_idle")) != NULL) {
        verbose_master(2, "OMPI_MCA_mpi_yield_when_idle %s", env);
        if (atoi(env) != 0)
            *block_type = YIELD_BLOCK;
    }
    return EAR_SUCCESS;
}

/* Support form MPI optimization */
state_t already_optimized(mpi_call call_type, p2i buf, p2i dest)
{
#if MPI_OPTIMIZED
    for (uint i = 0; i < num_mpi_to_optimize; i++) {
        if (list_mpi_to_optimize[i].call == call_type && list_mpi_to_optimize[i].last_buf == buf &&
            list_mpi_to_optimize[i].last_dest == dest) {
            return EAR_SUCCESS;
        }
    }
    return EAR_ERROR;
#endif
    return EAR_SUCCESS;
}

#if MPI_OPTIMIZED
state_t must_be_optimized(mpi_call call_type, p2i buf, p2i dest, ulong *elapsed)
{
    last_optimized = 0;

    if (!num_mpi_to_optimize)
        return EAR_ERROR;

    for (uint i = 0; i < num_mpi_to_optimize; i++) {
        if (list_mpi_to_optimize[i].call == call_type && list_mpi_to_optimize[i].last_buf == buf &&
            list_mpi_to_optimize[i].last_dest == dest) {

            last_optimized = 1;

            index_last_optimized = i;

            *elapsed = list_mpi_to_optimize[i].elap / list_mpi_to_optimize[i].cnt;

            return EAR_SUCCESS;
        }
    }
    return EAR_ERROR;
}
#endif

#if MPI_STATS
static state_t build_mpi_ear_stats_filenames(char *prefix, char *mpi_stats_fn, char *mpi_calls_stats_fn)
{
    if (sprintf(mpi_stats_fn, "%s.ear_mpi_stats.", prefix) < 0) {
        verbose(MPI_STATS_VERB_LVL + 1, "%sWARNING%s Writing <prefix>%s.ear_mpi_stats.", COL_RED, COL_CLR, prefix);
        return EAR_ERROR;
    }
    if (sprintf(mpi_calls_stats_fn, "%s.ear_mpi_calls_stats.", prefix) < 0) {
        verbose(MPI_STATS_VERB_LVL + 1, "%sWARNING%s Writing <prefix>%s.ear_mpi_calls_stats.", COL_RED, COL_CLR,
                prefix);
        return EAR_ERROR;
    }

    char nodename[GENERIC_NAME];
    if (gethostname(nodename, sizeof(nodename)) < 0) {
        verbose(MPI_STATS_VERB_LVL + 1, "%sWARNING%s Nodename could not be read: %d", COL_RED, COL_CLR, errno);
    }

    strcat(mpi_stats_fn, nodename);
    strcat(mpi_calls_stats_fn, nodename);

    char suffix[] = ".csv";

    strcat(mpi_stats_fn, suffix);
    strcat(mpi_calls_stats_fn, suffix);

    return EAR_SUCCESS;
}
#endif

/* These functions are used to control peaks of power */
#define DEF_MAX_TOTAL_POWER    500
#define DEF_MAX_POWER_PER_UNIT 60
#define DEF_POWER_PER_PSTATE   20
#define DEF_IDLE_POWER         100

#if MPI_OPTIMIZED
/* Default values */
static double MAX_TOTAL_POWER;
static double MAX_POWER_PER_UNIT;
static double POWER_PER_PSTATE;
static double IDLE_POWER;
#endif

extern double max_app_power;

void configure_processes_tobe_restored()
{
#if MPI_OPTIMIZED
    double delta_node = (max_app_power > 0 ? (max_app_power - IDLE_POWER) : 0);
    double delta_job  = (max_app_power > 0 ? (max_app_power - IDLE_POWER) * ear_mpi_opt_num_nodes : 0);
    /* This is a simple test where half of the processes will be reduced */
    uint tobe_opt = 0;

    verbose(2, "EAR-PP[%d] num nodes %d max_app_power %.2lf, delta_node %.2lf delta_job %.2lf ", ear_my_rank,
            ear_mpi_opt_num_nodes, max_app_power, delta_node, delta_job);

    /* If the application is big OR delta is big enough we apply peak power control */
    if ((delta_node > MAX_POWER_PER_UNIT) || (delta_job > MAX_TOTAL_POWER)) {
        /* If we exceed the limit per node we must be optimized */
        if (delta_node > MAX_POWER_PER_UNIT) {
            verbose(2, "EAR-PP[%d] node exceeds the limit %.2lf node %.2lf", ear_my_rank, MAX_POWER_PER_UNIT,
                    delta_node);
            tobe_opt = 1;
        } else {
            /* We exceeed the total limit */

            double delta_per_node = delta_job / ear_mpi_opt_num_nodes;

            delta_node = ear_max(delta_node, delta_per_node);
            verbose(2, "EAR-PP[%d] job exceeds the limit %.2lf job %.2lf delta_node %.2lf", ear_my_rank,
                    MAX_TOTAL_POWER, delta_job, delta_node);
        }
    }

    if (barrier_optimize == 0)
        tobe_opt = 0;

    if (tobe_opt) {
        must_be_restored = 1;
        num_pstates      = delta_node / POWER_PER_PSTATE; /* Number of steps to be applied */
        num_steps        = selected_steps =
            ear_min(ceil(MAX_POWER_PER_UNIT / POWER_PER_PSTATE), 2); /* Number of steps to be applied */
        block_ready = 0;
        verbose(2, "EAR-PP[%d] num_steps %u num_pstates %u", ear_my_rank, num_steps, num_pstates);

    } else {
        must_be_restored = 0;
    }
#endif // MPI_OPTIMIZED
}

#if MPI_OPTIMIZED
uint process_must_be_restored()
{
    return must_be_restored;
}

uint process_block_is_ready()
{
    return block_ready;
}

void process_block_ready()
{
    block_ready = 1;
}

void process_already_restored()
{
    must_be_restored = 0;
}

void process_reset_steps()
{
    num_steps = selected_steps;
}

void process_new_step()
{
    if (num_steps)
        num_steps--;
}

uint process_last_step()
{
    return (num_steps == 0);
}

void process_set_target_pstate(uint i)
{
    target_mpi_opt_index = i;
}

uint process_get_target_pstate()
{
    return target_mpi_opt_index;
}

uint process_get_num_pstates()
{
    return num_pstates;
}

void process_set_curr_mpi_pstate(uint i)
{
    curr_mpi_opt_index = i;
}

uint process_get_curr_mpi_pstate()
{
    return curr_mpi_opt_index;
}
#endif // MPI_OPTIMIZED

/* ADD here the specific functions for MPI optimzation */

/*
 * state_t mpi_opt_init_optimize(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id);
 * state_t mpi_opt_end_optimize(node_freqs_t *freqs, int *process_id);
 * void mpi_opt_end_summary(int verb_lvl);
 * state_t mpi_opt_periodic_action();
 */

/*
 * PEAK POWER CONTROL
 */
#if MPI_OPTIMIZED

static uint in_barrier = 0;

state_t peak_power_mpi_opt_init_optimize(polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id)
{

    mpi_call nbcall_type = (mpi_call) remove_blocking(call_type);

    /* Specific optimization for barriers */
    if (is_barrier(nbcall_type)) {
        in_barrier = 1;

        if (traces_are_on()) {
            traces_generic_event(ear_my_rank, my_node_id, TRA_BARRIER, 1);
        }

        /* Each process is configured independently */
        if (state_ok(ear_trylock(&ear_mpi_opt_lock))) {
            configure_processes_tobe_restored();

            /* This function returns true when the CPU freq must be modified */
            if (process_must_be_restored()) {
                pstate_t *sel_pstate, *pstate_list;
                uint currindex;
                pstate_list = metrics_get(MGT_CPUFREQ)->avail_list;
                verbose(2, "EAR-PP[%d]: MPI freq is %lu", ear_my_rank, sig_shared_region[my_node_id].mpi_freq);
                mgt_cpufreq_get_index(no_ctx, sig_shared_region[my_node_id].mpi_freq, &currindex, 1);
                if ((currindex + process_get_num_pstates()) > (metrics_get(MGT_CPUFREQ)->avail_count - 1)) {
                    num_pstates = (metrics_get(MGT_CPUFREQ)->avail_count - 1) - currindex;
                }
                sel_pstate = &pstate_list[currindex + process_get_num_pstates()];

                process_set_target_pstate(currindex);
                process_set_curr_mpi_pstate(currindex + process_get_num_pstates());
                /* Increments in num_pstates pstates */
                freqs->cpu_freq[my_node_id] = sel_pstate->khz;
                *process_id                 = my_node_id;
                verbose(2, "EAR-PP[%d]:Reduce CPU freq in barrier for process %d to CPu freq %lu", ear_my_rank,
                        my_node_id, (ulong) sel_pstate->khz);
                if (traces_are_on()) {
                    traces_generic_event(ear_my_rank, my_node_id, TRA_CPUF_RAMP_UP, sel_pstate->khz);
                }
            }
            ear_unlock(&ear_mpi_opt_lock);
        }
    } else
        in_barrier = 0;
    return EAR_SUCCESS;
}

state_t peak_power_mpi_opt_end_optimize(node_freqs_t *freqs, int *process_id)
{
    if (!in_barrier)
        return EAR_SUCCESS;
    verbose(2, "EAR-PP: END MPI");

    if (traces_are_on()) {
        traces_generic_event(ear_my_rank, my_node_id, TRA_BARRIER, 0);
    }

#if MPI_OPTIMIZED
    /* Double validation to avoid getting the lock if not needed */
    if (process_must_be_restored()) {
        if (state_ok(ear_trylock(&ear_mpi_opt_lock))) {

            if (process_must_be_restored()) {
                monitor_burst(earl_mpi_opt, 0);
                process_block_ready();
            }

            ear_unlock(&ear_mpi_opt_lock);
        }
    }
#endif
    return EAR_SUCCESS;
}

state_t peak_power_mpi_opt_periodic_action(uint *to_be_changed_pstate)
{
    uint new_pstate = 0;
#if MPI_OPTIMIZED
    /* Double validation to avoid getting the lock if not needed */
    if (process_must_be_restored()) {
        if (state_ok(ear_trylock(&ear_mpi_opt_lock))) {
            if (process_must_be_restored() && process_block_is_ready()) {
                verbose(2, "EAR-PP: Process %d must be restored", my_node_id);
                process_new_step();

                /* We must increment 1 pstate */
                if (process_last_step()) {
                    /* Reset number of steps per pstate */
                    process_reset_steps();

                    /* If we are below the target */
                    if (process_get_target_pstate() < process_get_curr_mpi_pstate()) {
                        new_pstate = process_get_curr_mpi_pstate() - 1;

                        verbose(2, "EAR-PP[%d] incrementing 1 pstate (%d)", ear_my_rank, new_pstate);

                        process_set_curr_mpi_pstate(new_pstate);

                        /* We have restored the target */
                        if (new_pstate == process_get_target_pstate()) {
                            process_already_restored();
                            monitor_relax(earl_mpi_opt);
                        }
                    }
                }
            }
            ear_unlock(&ear_mpi_opt_lock);
        }
    }
#endif
    *to_be_changed_pstate = new_pstate;
    return EAR_SUCCESS;
}
#endif // MPI_OPTIMIZED

void peak_power_mpi_opt_end_summary(int verb_lvl)
{
}

state_t mpi_opt_load(mpi_opt_policy_t *mpi_opt_func)
{
    memset(mpi_opt_func, 0, sizeof(mpi_opt_policy_t));
#if MPI_OPTIMIZED
    if (ear_mpi_opt && lib_shared_region->num_processes > 1) {
        char *cmax_tota_power, *cmax_power_per_unit, *cpower_per_pstate, *cidle_power;
        MAX_TOTAL_POWER =
            ((cmax_tota_power = ear_getenv("MAX_TOTAL_POWER")) != NULL ? atof(cmax_tota_power) : DEF_MAX_TOTAL_POWER);
        MAX_POWER_PER_UNIT =
            ((cmax_power_per_unit = ear_getenv("MAX_POWER_PER_UNIT")) != NULL ? atof(cmax_power_per_unit)
                                                                              : DEF_MAX_POWER_PER_UNIT);
        POWER_PER_PSTATE = ((cpower_per_pstate = ear_getenv("POWER_PER_PSTATE")) != NULL ? atof(cpower_per_pstate)
                                                                                         : DEF_POWER_PER_PSTATE);
        IDLE_POWER       = ((cidle_power = ear_getenv("IDLE_POWER")) != NULL ? atof(cidle_power) : DEF_IDLE_POWER);

        if (MAX_TOTAL_POWER == 0)
            MAX_TOTAL_POWER = DEF_MAX_TOTAL_POWER;
        if (MAX_POWER_PER_UNIT == 0)
            MAX_POWER_PER_UNIT = DEF_MAX_POWER_PER_UNIT;
        if (POWER_PER_PSTATE == 0)
            POWER_PER_PSTATE = DEF_POWER_PER_PSTATE;
        if (IDLE_POWER == 0)
            IDLE_POWER = DEF_IDLE_POWER;

        verbose_master(2,
                       "EAR-PP MAX_TOTAL_POWER %.2lf MAX_POWER_PER_UNIT %.2lf POWER_PER_PSTATE %.2lf IDLE_POWER %.2lf",
                       MAX_TOTAL_POWER, MAX_POWER_PER_UNIT, POWER_PER_PSTATE, IDLE_POWER);

        barrier_optimize = 1;

        char *ramp_strategy = ear_getenv("RAMP_UP_STRATEGY");
        if ((ramp_strategy == NULL) || ((ramp_strategy != NULL) && (strcmp(ramp_strategy, "default") == 0))) {
            mpi_opt_func->init_mpi = peak_power_mpi_opt_init_optimize;
            mpi_opt_func->end_mpi  = peak_power_mpi_opt_end_optimize;
            mpi_opt_func->summary  = peak_power_mpi_opt_end_summary;
            mpi_opt_func->periodic = peak_power_mpi_opt_periodic_action;
        } else if ((ramp_strategy != NULL) && (strcmp(ramp_strategy, "monitoring") == 0)) {
            /* Same code, for now, but no optimization will be performed */
            barrier_optimize       = 0;
            mpi_opt_func->init_mpi = peak_power_mpi_opt_init_optimize;
            mpi_opt_func->end_mpi  = peak_power_mpi_opt_end_optimize;
            mpi_opt_func->summary  = peak_power_mpi_opt_end_summary;
            mpi_opt_func->periodic = peak_power_mpi_opt_periodic_action;
        }
    } else
        barrier_optimize = 0;
#endif // MPI_OPTIMIZED
    return EAR_SUCCESS;
}

uint is_mpi_intensive()
{
#if MPI_SAMPLING
    if (mpi_sampling_enabled && (dynamic_monitor_state == MPI_SAMPLING_IDLE))
        return 0;
#endif
    if (avg_mpi_calls_per_second() > (MAX_MPI_CALLS_SECOND * 2))
        return 1;
    return 0;
}
