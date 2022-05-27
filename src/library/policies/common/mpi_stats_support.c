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

// #define SHOW_DEBUGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

//#define COMP_CPI 1
#include <metrics/cpi/cpi.h>

#include <management/cpufreq/cpufreq.h>

#include <common/config.h>
#include <common/states.h>
#include <common/math_operations.h>
#include <common/hardware/hardware_info.h>
#include <common/output/verbose.h>
#include <common/system/time.h>
#include <common/config/config_dev.h>

#include <library/common/library_shared_data.h>
#include <library/api/mpi_support.h>
#include <library/policies/policy_ctx.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/common/library_shared_data.h>
#include <library/common/global_comm.h>

#include <report/report.h>

/* Verbose levels */
#define LB_VERB_LVL        3		
#define MPI_STATS_VERB_LVL 2
#define VAR_DEPRECATED     1

#define PER_MPI_STATS 1 // Used for testing overhead

static timestamp pol_time_init, start_no_mpi;
static mpi_information_t mpi_stats;
static ulong total_mpi_time_sync        = 0;
static ulong total_mpi_time_collectives = 0;
static ulong total_mpi_time_blocking    = 0;
static ulong total_blocking_calls       = 0;
static ulong total_collectives          = 0;
static ulong total_synchronizations     = 0;
static ulong total_sync_block           = 0;
static ulong time_sync_block            = 0;
static ulong max_sync_block             = 0;

#if NEW_MPI_STATS
/* Statistics of all call types */
static uint   mpi_call_count[N_CALL_TYPES];
static ullong mpi_call_time[N_CALL_TYPES];
#endif // NEW_MPI_STATS

#if COMP_CPI
static cpi_t cpi_1;
static cpi_t cpi_2;
static cpi_t cpi_diff;
static double comp_cpi;
static ullong comp_cycles;
static ullong comp_insts;
#endif

float LB_TH = EAR_LB_TH;
static float critical_path_th   = 1;


static state_t mpi_call_read_diff(mpi_information_t *diff,mpi_information_t *last,int i);

/** Copies MPI metrics from \p src to \p dst.
 * If `src` is a NULL pointer, `dst` is filled with the MPI metrics of the `i`-th process
 * (local rank) stored in the shared region.
 * If `i` is negative, `dst` is filled with the MPI metrics of the current process (my_node_id). */
static state_t mpi_call_get_stats(mpi_information_t *dst, mpi_information_t * src, int i);

static state_t mpi_call_get_types(mpi_calls_types_t *dst, shsignature_t * src, int i);

static state_t mpi_call_types_read_diff(mpi_calls_types_t *diff, mpi_calls_types_t *last, int i);
/*  Computes the the max and min process with perc mpi calls from percs_mpi */
static state_t mpi_support_perc_mpi_max_min(int nump, double *node_mpi_calls, int *max, int *min);
/**  Computes basic statistics of percs_mpi */
static state_t mpi_support_compute_mpi_lb_stats(double *percs_mpi, int num_procs, double *mean,
        double *sd, double *mag);
static state_t mpi_stats_get_only_percs_mpi(mpi_information_t *node_mpi_calls, int nump,
        double *percs_mpi);
static state_t mpi_stats_evaluate_sim_uint(uint *current_cp, uint *last_cp,
        size_t size, double *sim);
static state_t mpi_stats_evaluate_mean_sd_mag(int nump, double *percs_mpi, double *mean,
        double *sd, double *mag);
static state_t mpi_stats_evaluate_median(int nump, double *percs_mpi, double *median);

state_t mpi_app_init(polctx_t *c)
{
    char * clb_th;

    if (c != NULL) {
        if ((clb_th = getenv(FLAG_LOAD_BALANCE_TH)) != NULL) {
            LB_TH = (float)atoi(clb_th);
        } else if ((clb_th = getenv(SCHED_LOAD_BALANCE_TH)) != NULL) {
            // TODO: This check is for the transition to the new environment variables.
            // It will be removed when SCHED_TRY_TURBO will be removed, in the next release.
            verbose(VAR_DEPRECATED, "%sWARNING%s %s will be removed on the next EAR release. "
                    "Please, change it by %s in your submission scripts.",
                    COL_RED, COL_CLR, SCHED_LOAD_BALANCE_TH, FLAG_LOAD_BALANCE_TH);
            LB_TH = (float)atoi(clb_th);
        }
        verbose_master(LB_VERB_LVL, "Load Balance threshold: %.1f. Critical path proximity %.1f", LB_TH, critical_path_th);

        sig_shared_region[my_node_id].mpi_info.mpi_time=0;
        sig_shared_region[my_node_id].mpi_info.total_mpi_calls=0;
        sig_shared_region[my_node_id].mpi_info.exec_time=0;
        sig_shared_region[my_node_id].mpi_info.perc_mpi=0;

        sig_shared_region[my_node_id].mpi_types_info.mpi_sync_call_cnt = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_collec_call_cnt = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_block_call_cnt = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_sync_call_time = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_collec_call_time = 0;
        sig_shared_region[my_node_id].mpi_types_info.mpi_block_call_time = 0;

        /* Gobal mpi statistics */
        mpi_stats.mpi_time        = 0;
        mpi_stats.total_mpi_calls = 0;
        mpi_stats.exec_time       = 0;
        mpi_stats.perc_mpi        = 0;

#if NEW_MPI_STATS
        for (int i = 0; i < N_CALL_TYPES; i++) {
            mpi_call_count[i] = 0;
            mpi_call_time[i]  = 0;
        }
#endif

        // TODO: This is redundant since policies call this function only if mpi is enabled
        if (is_mpi_enabled()) {
            mpi_stats.rank = sig_shared_region[my_node_id].mpi_info.rank;
        } else {
            mpi_stats.rank = 0;
        }

        timestamp_getfast(&start_no_mpi);

#if COMP_CPI
        cpi_read(no_ctx, &cpi_1);
#endif

    } else {
        return EAR_ERROR;
    }

    return EAR_SUCCESS;

}

static state_t build_mpi_ear_stats_filenames(char *prefix, char *mpi_stats_fn, char *mpi_calls_stats_fn)
{
    if (sprintf(mpi_stats_fn, "%s.ear_mpi_stats.", prefix) < 0) {
        verbose(MPI_STATS_VERB_LVL + 1, "%sWARNING%s Writing <prefix>%s.ear_mpi_stats.", COL_RED, COL_CLR, prefix);
        return EAR_ERROR;
    }
    if (sprintf(mpi_calls_stats_fn, "%s.ear_mpi_calls_stats.", prefix) < 0) {
        verbose(MPI_STATS_VERB_LVL + 1, "%sWARNING%s Writing <prefix>%s.ear_mpi_calls_stats.", COL_RED, COL_CLR, prefix);
        return EAR_ERROR;
    }

    char nodename[GENERIC_NAME];
    if (gethostname(nodename, sizeof(nodename)) < 0) {
        verbose(MPI_STATS_VERB_LVL + 1, "%sWARNING%s Nodename could not be read: %d", COL_RED, COL_CLR, errno);
    }

    strcat(mpi_stats_fn, nodename);
    strcat(mpi_calls_stats_fn, nodename);

    char suffix[GENERIC_NAME] = ".csv";

    strcat(mpi_stats_fn, suffix);
    strcat(mpi_calls_stats_fn, suffix);

    return EAR_SUCCESS;
}

state_t mpi_app_end(polctx_t *c)
{
    timestamp end;
    ullong elap;
    int fd;
    char buff[256] ,buff2[300];
    char mpi_stats_filename[256];
    char mpi_stats_per_call_filename[256];

    /* Global statistics */
    timestamp_getfast(&end);
    elap = timestamp_diff(&end, &start_no_mpi, TIME_USECS);
    mpi_stats.exec_time += elap;

    if (mpi_stats.total_mpi_calls) {
        mpi_stats.perc_mpi = (float) mpi_stats.mpi_time / (float) mpi_stats.exec_time;

        char *stats = getenv(FLAG_GET_MPI_STATS);
        if (stats == NULL) {
            stats = getenv(SCHED_GET_EAR_STATS);
            if (stats != NULL) {
            verbose(VAR_DEPRECATED, "%sWARNING%s %s will be removed on the next EAR release. "
                    "Please, change it by %s in your submission scripts.",
                    COL_RED, COL_CLR, SCHED_GET_EAR_STATS, FLAG_GET_MPI_STATS);
            }
        }
        if (stats != NULL) {

            if (state_fail(build_mpi_ear_stats_filenames(stats, mpi_stats_filename,
                            mpi_stats_per_call_filename))) {
                verbose(MPI_STATS_VERB_LVL, "%sWARNING%s MPI stats files won't be created.", COL_RED, COL_CLR);
                return EAR_WARNING;
            }

            verbose_master(MPI_STATS_VERB_LVL,"      MPI stats file: %s\nMPI calls stats file: %s",
                    mpi_stats_filename, mpi_stats_per_call_filename);

            // Open mpi_stats_filename to store per process global MPI stats
            fd=open(mpi_stats_filename, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);

            if (fd < 0) {
                verbose(MPI_STATS_VERB_LVL, "%sWARNING%s MPI stats file couldn't have opened: %d",
                        COL_YLW, COL_CLR, errno);

                mpi_info_to_str(&mpi_stats, buff, sizeof(buff));
                verbose(MPI_STATS_VERB_LVL - 1, "MR[%d] %s",
                        lib_shared_region->master_rank, buff);
            } else {
                // Only the master writes the header
                if (masters_info.my_master_rank >= 0) {
                    mpi_info_head_to_str_csv(buff, sizeof(buff));
                    sprintf(buff2, "mrank;%s\n", buff);
                    write(fd, buff2, strlen(buff2));
                }
                mpi_info_to_str_csv(&mpi_stats, buff, sizeof(buff));
                sprintf(buff2, "%d;%s\n", lib_shared_region->master_rank, buff);
                write(fd, buff2, strlen(buff2));
                close(fd);
            }

            // Open mpi_stats_per_call_filename to store per process MPI calls stats
            fd = open(mpi_stats_per_call_filename, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);

            if (fd >= 0) {
#if NEW_MPI_STATS
                // Only the master writes the header
                if (mpi_stats.rank == 0) {
                    sprintf(buff, "Master;Rank;Total MPI calls;MPI_time/Exec_time;Exec_time;Sync_time;"
                            "Block_time;Collec_time;Total MPI sync calls;Total blocking calls;"
                            "Total collective calls;Gather;Reduce;All2all;Barrier;Bcast;Send;Comm;Receive;"
                            "Scan;Scatter;SendRecv;Wait;t_Gather;t_Reduce;t_All2all;t_Barrier;t_Bcast;t_Send;"
                            "t_Comm;t_Receive;t_Scan;t_Scatter;t_SendRecv;t_Wait;Block;t_Block\n");
                    write(fd, buff, strlen(buff));
                }

                sprintf(buff,"%d;%d;%u;%.4f;%llu;%lu;%lu;%lu;%lu;%lu;%lu;%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;%u;%u"
                        "%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu;%llu\n",
                        lib_shared_region->master_rank,
                        mpi_stats.rank,
                        mpi_stats.total_mpi_calls,
                        mpi_stats.perc_mpi,
                        mpi_stats.exec_time,
                        total_mpi_time_sync,
                        total_mpi_time_blocking,
                        total_mpi_time_collectives,
                        total_synchronizations,
                        total_blocking_calls,
                        total_collectives,
                        mpi_call_count[GATHER],
                        mpi_call_count[REDUCE],
                        mpi_call_count[ALL2ALL],
                        mpi_call_count[BARRIER],
                        mpi_call_count[BCAST],
                        mpi_call_count[SEND],
                        mpi_call_count[COMM],
                        mpi_call_count[RECEIVE],
                        mpi_call_count[SCAN],
                        mpi_call_count[SCATTER],
                        mpi_call_count[SENDRECV],
                        mpi_call_count[WAIT],
                        mpi_call_count[BLOCKING],
                        mpi_call_time[GATHER],
                        mpi_call_time[REDUCE],
                        mpi_call_time[ALL2ALL],
                        mpi_call_time[BARRIER],
                        mpi_call_time[BCAST],
                        mpi_call_time[SEND],
                        mpi_call_time[COMM],
                        mpi_call_time[RECEIVE],
                        mpi_call_time[SCAN],
                        mpi_call_time[SCATTER],
                        mpi_call_time[SENDRECV],
                        mpi_call_time[WAIT],
                        mpi_call_time[BLOCKING]
                        );
#else
                // Only the master writes the header
                if (mpi_stats.rank == 0) {
                    sprintf(buff, "Master;Rank;Total MPI calls;MPI_time/Exec_time;Exec_time;Sync_time;"
                            "Block_time;Collec_time;Total MPI sync calls;Total blocking calls;"
                            "Total collective calls\n");
                    write(fd, buff, strlen(buff));
                }

                sprintf(buff,"%d;%d;%u;%.4f;%llu;%lu;%lu;%lu;%lu;%lu;%lu\n",
                        lib_shared_region->master_rank,
                        mpi_stats.rank,
                        mpi_stats.total_mpi_calls,
                        mpi_stats.perc_mpi,
                        mpi_stats.exec_time,
                        total_mpi_time_sync,
                        total_mpi_time_blocking,
                        total_mpi_time_collectives,
                        total_synchronizations,
                        total_blocking_calls,
                        total_collectives
                        );
#endif // NEW_MPI_STATS
                write(fd, buff, strlen(buff));
            }
        }
    }
#if COMP_CPI
    cpi_read_diff(no_ctx, &cpi_2, &cpi_1, &cpi_diff, &comp_cpi);
    comp_cycles += cpi_diff.cycles;
    comp_insts += cpi_diff.instructions;
    // sig_shared_region[my_node_id].comp_cycles = comp_cycles;
    // sig_shared_region[my_node_id].comp_insts = comp_insts;
    verbose(0, "Comp. CPI (%u): %lf", my_node_id, (double) comp_cycles / (double) comp_insts);
#endif
    return EAR_SUCCESS;
}

state_t mpi_call_init(polctx_t *c, mpi_call call_type)
{
#if PER_MPI_STATS
    if (is_mpi_enabled()) {

        // Init the MPI call timer
        timestamp_getfast(&pol_time_init);

        // Get the time not been in MPI
        ullong time_no_mpi = timestamp_diff(&pol_time_init, &start_no_mpi, TIME_USECS);

        // Accumulate
        mpi_stats.exec_time += time_no_mpi;

#if COMP_CPI
    cpi_read_diff(no_ctx, &cpi_2, &cpi_1, &cpi_diff, &comp_cpi);
    comp_cycles += cpi_diff.cycles;
    comp_insts += cpi_diff.instructions;
    // sig_shared_region[my_node_id].comp_cycles = comp_cycles;
    // sig_shared_region[my_node_id].comp_insts = comp_insts;
#endif // COMP_CPI

        // Update
        // sig_shared_region[my_node_id].mpi_info.exec_time += time_no_mpi;
        sig_shared_region[my_node_id].mpi_info.exec_time = mpi_stats.exec_time;

#if SHOW_DEBUGS
        if ((sig_shared_region[my_node_id].mpi_info.total_mpi_calls % 100) == 0) {
            debug("P[%d] (sec %ld nsec %ld) - (sec %ld nsec %ld) %llu not_mpi_elap",
                    my_node_id, pol_time_init.tv_sec, pol_time_init.tv_nsec,
                    start_no_mpi.tv_sec, start_no_mpi.tv_nsec, time_no_mpi);
        }
#endif // SHOW_DEBUGS
    }
#endif // PER_MPI_STATS
    return EAR_SUCCESS;
}

state_t mpi_call_end(polctx_t *c, mpi_call call_type)
{
    timestamp_t end;
    mpi_call nbcall_type;
    ullong elap;
#if PER_MPI_STATS

    if (is_mpi_enabled()) {

        timestamp_getfast(&end);
        elap = timestamp_diff(&end, &pol_time_init, TIME_USECS); // Get the time been in the MPI call

        debug("MPI END P[%d] sec %ld nsec %ld", my_node_id, end.tv_sec, end.tv_nsec);

        // Only collect stats for blocking calls
        if (is_mpi_blocking(call_type)) {

            total_blocking_calls++;
            total_mpi_time_blocking += (ulong) elap;


            max_sync_block = ear_max(max_sync_block, (ulong)elap);

            // Get which type of blocking call is
            nbcall_type = (mpi_call) remove_blocking(call_type);
#if NEW_MPI_STATS
            mpi_call_count[BLOCKING]++;
            mpi_call_time[BLOCKING] += elap;

            mpi_call_count[nbcall_type]++;
            mpi_call_time[nbcall_type] += elap;
#endif // NEW_MPI_STATS
            if (is_collective(nbcall_type)) {
                // Collective
                total_collectives++;
                total_mpi_time_collectives += (ulong)elap;

            } else if (is_synchronization(nbcall_type)) {
                // Sync
                total_synchronizations++;
                total_mpi_time_sync += (ulong)elap;
            } else {
                total_sync_block++;
                time_sync_block += (ulong)elap;
            }
        }
        /* Global statistics */
        mpi_stats.mpi_time  += elap;
        mpi_stats.exec_time += elap;
        mpi_stats.total_mpi_calls++;

        sig_shared_region[my_node_id].mpi_info.mpi_time  = mpi_stats.mpi_time;
        sig_shared_region[my_node_id].mpi_info.exec_time = mpi_stats.exec_time;
        sig_shared_region[my_node_id].mpi_info.total_mpi_calls = mpi_stats.total_mpi_calls;

        sig_shared_region[my_node_id].mpi_types_info.mpi_sync_call_cnt            = total_synchronizations;
        sig_shared_region[my_node_id].mpi_types_info.mpi_collec_call_cnt          = total_collectives;
        sig_shared_region[my_node_id].mpi_types_info.mpi_block_call_cnt        = total_blocking_calls;
        sig_shared_region[my_node_id].mpi_types_info.mpi_sync_call_time       = total_mpi_time_sync;
        sig_shared_region[my_node_id].mpi_types_info.mpi_collec_call_time     = total_mpi_time_collectives;
        sig_shared_region[my_node_id].mpi_types_info.mpi_block_call_time   = total_mpi_time_blocking;
        sig_shared_region[my_node_id].mpi_types_info.sync_block      = total_sync_block;
        sig_shared_region[my_node_id].mpi_types_info.time_sync_block = time_sync_block;
        sig_shared_region[my_node_id].mpi_types_info.max_sync_block  = max_sync_block;

#if SHOW_DEBUGS
        if ((sig_shared_region[my_node_id].mpi_info.total_mpi_calls % 100) == 0) {
            debug("P[%d] MPI %llu exec %llu %llu elap (sec %ld nsec %ld)",
                    my_node_id, sig_shared_region[my_node_id].mpi_info.mpi_time,
                    sig_shared_region[my_node_id].mpi_info.exec_time, elap,
                    start_no_mpi.tv_sec, start_no_mpi.tv_nsec);
        }
#endif // SHOW_DEBUGS
        timestamp_getfast(&start_no_mpi);
#if COMP_CPI
        cpi_read(no_ctx, &cpi_1);
#endif
    }
#endif // PER_MPI_STATS
    return EAR_SUCCESS;
}

static state_t mpi_call_get_stats(mpi_information_t *dst, mpi_information_t * src, int i)
{

    if (dst == NULL) return EAR_ERROR;
    if (src == NULL) {
        if (i < 0) {
            src = &sig_shared_region[my_node_id].mpi_info;
        }
        else {
            src = &sig_shared_region[i].mpi_info;
        }
    }
    memcpy(dst, src, sizeof(mpi_information_t));
    return EAR_SUCCESS;
}

/*
static void mpi_call_copy(mpi_information_t *dst, mpi_information_t * src)
{
    if ((dst == NULL) || (src == NULL)) return;
    memcpy(dst, src, sizeof(mpi_information_t));
}
*/

static state_t mpi_call_diff(mpi_information_t *diff, mpi_information_t *end, mpi_information_t *init)
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

    debug("Process %d: mpi %llu = (%llu - %llu) exec %llu = (%llu - %llu)",
            i, diff->mpi_time, current.mpi_time, last->mpi_time,
            diff->exec_time, current.exec_time, last->exec_time);

    diff->perc_mpi = (double) diff->mpi_time * 100.0 / (double) diff->exec_time;

    // mpi_call_copy(last, &current);
    mpi_call_get_stats(last, &current, -1); // Copy from current to last
    last->perc_mpi =  (double) last->mpi_time * 100.0 / (double) last->exec_time;

    //  Check wether values are correctly bounded
    if (diff->perc_mpi < 0 || diff->perc_mpi > 100 || last->perc_mpi < 0 || last->perc_mpi > 100) {
        verbose_master(2, "%s[WARNING] Bad perc_mpi computation%s: diff = %lf / %lf = %lf, last = %lf / %lf = %lf",
                COL_RED, COL_CLR, (double) diff->mpi_time * 100.0, (double) diff->exec_time,
                diff->perc_mpi, (double) last->mpi_time * 100.0, (double) last->exec_time, last->perc_mpi);
        return EAR_WARNING;
    }
    return EAR_SUCCESS;
}

static state_t mpi_call_get_types(mpi_calls_types_t *dst, shsignature_t * src, int i)
{
    mpi_information_t *src_stat;
    mpi_calls_types_t *src_types;

    if (dst == NULL) return EAR_ERROR;
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
        src_stat = &src->mpi_info;
    }

    memcpy(dst, src_types, sizeof(mpi_calls_types_t));

    dst->mpi_time 	  = src_stat->mpi_time;
    dst->exec_time 	  = src_stat->exec_time;
    dst->mpi_call_cnt = src_stat->total_mpi_calls;

    return EAR_SUCCESS;
}

static state_t mpi_call_types_diff(mpi_calls_types_t * diff, mpi_calls_types_t *current,
        mpi_calls_types_t *last)
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
    diff->sync_block           = current->sync_block - last->sync_block;
    diff->time_sync_block      = current->time_sync_block - last->time_sync_block;
    diff->max_sync_block       = ear_max(current->max_sync_block , last->max_sync_block);

    return EAR_SUCCESS;
}

static state_t mpi_call_types_read_diff(mpi_calls_types_t *diff, mpi_calls_types_t *last, int i)
{
    mpi_calls_types_t current;

    mpi_call_get_types(&current, NULL, i);

    mpi_call_types_diff(diff, &current, last);
    memcpy(last, &current, sizeof(mpi_calls_types_t));
    return EAR_SUCCESS;
}

void verbose_mpi_data(int verb_lvl, mpi_information_t *mc)
{
    verbose_master(verb_lvl, "Total mpi calls %u MPI time %llu exec time %llu perc mpi %.2lf",
            mc->total_mpi_calls, mc->mpi_time, mc->exec_time, mc->perc_mpi);
}

float compute_mpi_in_period(mpi_information_t *mc)
{
    float mpi_call_cnt  = mc[my_node_id].total_mpi_calls;
    ullong exec_time_sec = mc[my_node_id].exec_time / 1000000;
    return mpi_call_cnt / (float) exec_time_sec;
}


void verbose_mpi_calls_types(int verb_lvl, mpi_calls_types_t *t)
{
    char perc_sync_time[16], perc_collec_time[16], perc_block_time[16], mpi_calls_per_sec[16], perc_mpi_time[16];

    if (t->exec_time == 0) {
        sprintf(perc_sync_time,    "NAV");
        sprintf(perc_collec_time,  "NAV");
        sprintf(mpi_calls_per_sec, "NAV");
        sprintf(perc_mpi_time,     "NAV");
    } else {
        sprintf(perc_sync_time, "%.2lf", (float) t->mpi_sync_call_time * 100.0 / (float) t->exec_time);
        sprintf(perc_collec_time, "%.2lf", (float) t->mpi_collec_call_time * 100.0 / (float) t->exec_time);
        sprintf(perc_block_time, "%.2lf", (float) t->mpi_block_call_time * 100.0 / (float) t->exec_time);
        sprintf(mpi_calls_per_sec, "%.2lf", (float) t->mpi_call_cnt * 1000000.0 / (float) t->exec_time);
        sprintf(perc_mpi_time, "%.2lf", (float) t->mpi_time * 100.0 / (float) t->exec_time);
    }

    verbose_master(verb_lvl, "MPI types\n---------\n\t# MPI calls: %lu\n\tMPI time (us): %lu\n\tExecution time (us): %lu\n"
            "\t# Synchronization calls: %lu\n\t# Collective calls: %lu\n\t# Blocking calls: %lu\n\t%% Time synchronization: %s\n"
            "\t%% Time collective: %s\n\t%% Time blocking: %s\n\t# MPI calls/s: %s\n\t%% MPI time: %s\n"
            "sblock=%lu/%lu/%lu",
            t->mpi_call_cnt, t->mpi_time, t->exec_time,
            t->mpi_sync_call_cnt, t->mpi_collec_call_cnt, t->mpi_block_call_cnt, perc_sync_time,
            perc_collec_time, perc_block_time, mpi_calls_per_sec, perc_mpi_time,
            t->sync_block, t->time_sync_block, t->max_sync_block);
}

state_t read_diff_node_mpi_info(lib_shared_data_t *data, shsignature_t *sig,
        mpi_information_t *node_mpi_calls,mpi_information_t *last_node)
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

state_t read_diff_node_mpi_types_info(lib_shared_data_t *data, shsignature_t *sig,
        mpi_calls_types_t *node_mpi_calls, mpi_calls_types_t *last_node)
{
    int i = 0;
    if ((data == NULL) || (sig == NULL) || (node_mpi_calls == NULL) || (last_node == NULL)) {
        return EAR_ERROR;
    }
    for (i=0;i<data->num_processes;i++) {
        mpi_call_types_read_diff(&node_mpi_calls[i], &last_node[i], i);
    }
    return EAR_SUCCESS;
}

static state_t mpi_support_perc_mpi_max_min(int nump, double *percs_mpi, int *max, int *min)
{
    *max = 0;
    *min = 0;
    verbosen_master(LB_VERB_LVL, "Load balance data ");
    for (int i = 1; i < nump; i++) {
        verbosen_master(LB_VERB_LVL, "MR[%d]= %.2lf ", i, percs_mpi[i]);
        if (percs_mpi[i] > percs_mpi[*max])	*max = i;
        if (percs_mpi[i] < percs_mpi[*min])	*min = i;
    }
    verbose_master(LB_VERB_LVL, " ");
    return EAR_SUCCESS;
}

static state_t mpi_support_compute_mpi_lb_stats(double *percs_mpi, int num_procs,
        double *mean, double *sd, double *mag) {

    /*  Compute mean, standard deviation and magnitude of percs_mpi vector */
    mpi_stats_evaluate_mean_sd_mag(num_procs, percs_mpi, mean,
            sd, mag);

    return EAR_SUCCESS;
}

/* This function sets the values to be shared with other nodes */
static mpi_summary_t my_mpi_summary;

state_t mpi_support_set_mpi_summary(mpi_summary_t *mpisumm, double *percs_mpi, int num_procs,
        double mean, double sd, double mag)
{
    double max, min;
    max = 0.0;
    min = 100.0; 
    int i;
    for (i=0;i<num_procs;i++) {
        min = ear_min(min,percs_mpi[i]);
        max = ear_max(max,percs_mpi[i]);
    }
    mpisumm->max  = (float) max;
    mpisumm->min  = (float) min;
    mpisumm->sd   = (float) sd;
    mpisumm->mean = (float) mean;
    mpisumm->mag  = (float) mag;
    return EAR_SUCCESS;
}

state_t mpi_support_evaluate_lb(mpi_information_t *node_mpi_calls, int num_procs,
        double *percs_mpi, double *mean, double *sd, double *mag, uint *lb)
{
    /* Get only the percentage time spent in MPI blocking calls. */
    state_t ret;
    // Get only perc_mpi info.
    ret = mpi_stats_get_only_percs_mpi(node_mpi_calls, num_procs, percs_mpi); // Check whether perc_mpi is ok

    // Compute statistics for determinate whether application is load balanced.
    mpi_support_compute_mpi_lb_stats(percs_mpi, num_procs, mean, sd, mag);

    mpi_support_set_mpi_summary(&my_mpi_summary, percs_mpi, num_procs, *mean, *sd, *mag);

#if NODE_LB
#if NEW_LB_EFF
    ullong max_tcomp = node_mpi_calls[0].exec_time - node_mpi_calls[0].mpi_time;
    double avg_tcomp = (double) max_tcomp;

    for (int i = 1; i < num_procs; i++) {
        ullong current_tcomp = node_mpi_calls[i].exec_time - node_mpi_calls[i].mpi_time;
        verbose_master(LB_VERB_LVL, "tcomp (l. rank) %d %llu - %llu = %llu | max_tcomp %llu ", i,
                node_mpi_calls[i].exec_time, node_mpi_calls[i].mpi_time, current_tcomp, max_tcomp);

        max_tcomp = ear_max(current_tcomp, max_tcomp);
        /*
        if (max_tcomp < current_tcomp) {
            max_tcomp = current_tcomp;
        }
        */
        avg_tcomp += current_tcomp;
    }
    avg_tcomp /= num_procs;

    double lb_eff = avg_tcomp / (double) max_tcomp;

    verbose_master(LB_VERB_LVL, "%sLoad Balance efficiency%s %lf %smax tcomp%s %llu %savg tcomp%s %lf %sLB_TH%s %lf",
            COL_YLW, COL_CLR, lb_eff, COL_YLW, COL_CLR, max_tcomp, COL_YLW, COL_CLR, avg_tcomp, COL_YLW, COL_CLR, LB_TH);

    *lb = (lb_eff < LB_TH);
#endif // 0
    *lb = (*sd > LB_TH);
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

    memset(critical_path, 0, sizeof(uint)*num_procs);
    verbose_master(LB_VERB_LVL+1,"%sPer-core CPU freq. selection%s", COL_RED, COL_CLR);

    for (uint i=0; i < num_procs; i++) {
        /*  Decides whether process i is part of the critical path */
        if (abs(percs_mpi[i] - percs_mpi[*min_mpi]) < (abs(percs_mpi[i] - *median) * critical_path_th)) {
            verbose_master(LB_VERB_LVL, "Process %d selected as part of the critical path", i);
            critical_path[i] = 1;
						total_cp++;
        } else if (abs(percs_mpi[i] - percs_mpi[*min_mpi]) < abs(percs_mpi[i] - mean)) {
            debug("%sWARNING%s Proc %d is closer to the minimum than the mean, but not selected as part of the critical path",
                    COL_RED, COL_CLR, i);
        }
    }
		verbose_master(2, "Total processes selected as critical path %u. Max MPI %.2f Min MPI %.2f Mean %.2f", total_cp, percs_mpi[*max_mpi], percs_mpi[*min_mpi], mean);

    return EAR_SUCCESS;
}

state_t mpi_support_verbose_perc_mpi_stats(int verb_lvl, double *percs_mpi, int num_procs,
        double mean, double median, double sd, double mag)
{
    if (verb_lvl == 3){
        debug("Previous percs_mpi:");
        for (int i = 0; i < num_procs; i++){
            debug("MR[%d]=%lf", i, percs_mpi[i]);
        }
    }
    else if(verb_lvl <= 3){
        debug("PREVIOUS mean %lf median %lf standard deviation %lf magnitude %lf",
                mean, median, sd, mag);
    }

    return EAR_SUCCESS;
}

state_t mpi_support_mpi_changed(double current_mag, double prev_mag, uint *cp, uint *prev_cp,
        int num_procs, double *similarity, uint *mpi_changed)
{
    *mpi_changed = 0;

    /*  Check whether perc_mpi vector magnitude has increased with respect to previous one */
    if (!equal_with_th(prev_mag, current_mag, 0.05))
    {
        if (prev_mag < current_mag){
            debug("%sMore MPI than before%s (%.2lf/%.2lf)", COL_MGT, COL_CLR,
                    prev_mag, current_mag);
            *mpi_changed = 1;
        }
    } else {
        /*  Check similarity between critical paths */
        mpi_stats_evaluate_sim_uint(cp, prev_cp, num_procs, similarity);
        verbose_master(LB_VERB_LVL,
                "Similarity between last and current critical path selection %lf", *similarity);
        if (*similarity < 0.5)
        {
            *mpi_changed = 1;
        }
    }
    return EAR_SUCCESS;
}

static state_t mpi_stats_get_only_percs_mpi(mpi_information_t *node_mpi_calls, int nump,
        double *percs_mpi)
{
    mpi_information_t **mpi_calls = calloc(nump, sizeof(mpi_information_t*));

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
    return (*(double * const *) p1 < *(double * const *) p2) ? -1 : (*(double * const *) p1 > *(double * const *) p2) ? 1 : 0;
}

static state_t mpi_stats_evaluate_mean_sd_mag(int nump, double *percs_mpi, double *mean, double *sd, double *mag)
{
    if (nump == 0 || percs_mpi == NULL) {
        return EAR_ERROR;
    }

    mean_sd_t mean_sd_mag = ear_math_mean_sd(percs_mpi, nump);
    *mean = mean_sd_mag.mean;
    *sd = mean_sd_mag.sd;
    *mag = mean_sd_mag.mag;

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
    if ((env = getenv("I_MPI_WAIT_MODE")) != NULL){
        verbose_master(2,"I_MPI_WAIT_MODE %s",env);
        if (atoi(env) != 0 ) *block_type = YIELD_BLOCK;
    }
    if ((env = getenv("I_MPI_THREAD_YIELD")) != NULL){
        verbose_master(2,"I_MPI_THREAD_YIELD %s",env);
        if (atoi(env) != 0 ) *block_type = YIELD_BLOCK;
    }
    if ((env = getenv("OMPI_MCA_mpi_yield_when_idle")) != NULL){
        verbose_master(2,"OMPI_MCA_mpi_yield_when_idle %s",env);
        if (atoi(env) != 0 ) *block_type = YIELD_BLOCK;
    }
    return EAR_SUCCESS;
}
