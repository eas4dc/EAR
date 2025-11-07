/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#if MPI
#include <mpi.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// #define SHOW_DEBUGS 1
#include <common/colors.h>
#include <common/config.h>
#include <common/math_operations.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/types/application.h>
#include <common/types/log.h>
#include <daemon/local_api/eard_api.h>
#include <library/common/externs.h>
#include <library/common/global_comm.h>
#include <library/common/verbose_lib.h>
#include <library/metrics/metrics.h>
#include <library/tracer/tracer.h>
#include <metrics/common/pstate.h>

#define GPU_SIG_VRB_LVL 2
#define SIG_VRB_LVL     1

extern masters_info_t masters_info;
extern float ratio_PPN;
extern unsigned long long int total_mpi;
extern uint using_verb_files;
extern uint per_proc_verb_file;
extern uint proc_ps_ok;
extern uint AID;

#ifdef SHOW_DEBUGS
static void debug_loop_signature(char *title, signature_t *loop);
#endif

void state_verbose_signature(loop_t *sig, int master_rank, char *aname, char *nname, int iterations, ulong prevf,
                             ulong new_freq, char *user_mode)
{
    double CPI, GBS, POWER, TIME, VPI, FLOPS;
    float AVGF = 0, AVGGPUF = 0, PFREQ = 0, NFREQ = 0;
    float IO_MBS;
    ulong GPU_UTIL = 0, GPU_FREQ = 0, gpu_used_cnt = 0, GPU_mem_util = 0;
    ulong elapsed;
    timestamp end;
    char mode[32];
    char gflops_str[128];
#if WF_SUPPORT
    float GPU_GFlops = 0;
    ulong GPU_temp   = 0;
#endif

    if (VERB_GET_LV() == 0)
        return;
    sprintf(mode, "%s", "");
    if (VERB_GET_LV() >= 2) {
        strcpy(mode, user_mode);
    }

    timestamp_getfast(&end);
    elapsed = timestamp_diff(&end, &ear_application_time_init, TIME_SECS);

    float imcf, sel_mem_f;
    char gpu_buff[1024];

    /* master_rank == 0: The master node rank (is who verboses if verb_level >= 1).
     * using_verb_files: FLAG_EARL_VERBOSE_PATH
     * per_proc_verb_file: HACK_PROCS_VERB_FILES (per_proc_verb_file == 1 => using_verb_files == 1)
     */
    if (master_rank == 0 || using_verb_files) {

        // MPI info
        sig_ext_t *sigex = (sig_ext_t *) sig->signature.sig_ext;
        // Only the master shows that information as is who computed it
        // TODO: Does the shared signature have these metrics after the master computed it?
        float max_mpi = (master_rank >= 0) ? (sigex->max_mpi) : 0.0;
        float min_mpi = (master_rank >= 0) ? (sigex->min_mpi) : 0.0;

        // If enabled, each process reports its %MPI
        float perc_mpi =
            (per_proc_verb_file) ? (sig_shared_region[my_node_id].mpi_info.perc_mpi) : sig->signature.perc_MPI;

        char pmpi_txt[256];
        if (perc_mpi > 0.0) {
            if (per_proc_verb_file) {
                // If enabled, each process reports its %MPI
                // TODO: Depending on how the above TODO is resolved, below code may must be changed
                snprintf(pmpi_txt, sizeof(pmpi_txt), "MPI_time/exec_Time: %.1f", perc_mpi);
            } else {
                // Since when only master verboses, it prints the average across all processes of the job
                snprintf(pmpi_txt, sizeof(pmpi_txt), "MPI_time/exec_Time (max/avg/min): %.1f/%.1f/%.1f %%", max_mpi,
                         perc_mpi, min_mpi);
            }
        } else {
            sprintf(pmpi_txt, "No MPI");
        } // MPI info

        // IMC
        imcf = (per_proc_verb_file) ? (float) lib_shared_region->job_signature.avg_imc_f / 1000000.0
                                    : (float) sig->signature.avg_imc_f / 1000000.0;
        // Slaves can't use their extended signature
        sel_mem_f = (sigex == NULL || master_rank < 0) ? imcf : ((float) sigex->sel_mem_freq_khz / 1000000.0);

        // CPI
        // If enabled, each process reports its CPI
        CPI = (per_proc_verb_file) ? (double) sig_shared_region[my_node_id].sig.CPI : sig->signature.CPI;

        // GB/s
        GBS = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.GBS : sig->signature.GBS;

        // I/O MB/s
#if 0
		io_data_t *io_data =
		    (per_proc_verb_file) ? &sig_shared_region[my_node_id].
		    sig.iod : &se->iod;
#endif
        // If enabled, each process has its own I/O data
        // TODO: Why we have an or clause here ? to avoid a crash?
        io_data_t *io_data =
            (per_proc_verb_file || master_rank < 0) ? &sig_shared_region[my_node_id].sig.iod : &sigex->iod;

        char io_info[256];
        if (sigex != NULL) {
            io_tostr(io_data, io_info, sizeof(io_info)); // io_data declared above
        } else {
            sprintf(io_info, "No IO data");
        }

        // TODO: period or time ? time is the time per iteration. This metric is computed periodically by a thread.
        // If enabled each process computes it I/O bandwidth, otherwise the node I/O bandwidth is already computed
        IO_MBS =
            (per_proc_verb_file)
                ? ((float) io_data->rchar / (float) (1024 * 1024) + (float) io_data->wchar / (float) (1024 * 1024)) /
                      sig_shared_region[my_node_id].period
                : (float) sig->signature.IO_MBS;

        // Proc OS statistics
        char proc_data_str[256];
        if (proc_ps_ok) {
            if (per_proc_verb_file) {
                snprintf(proc_data_str, sizeof(proc_data_str) - 1, "Process CPU util.: %u",
                         sig_shared_region[my_node_id].cpu_util);
            } else {
                snprintf(proc_data_str, sizeof(proc_data_str) - 1, "Aggregated application's node CPU util.: %u",
                         sig->signature.ps_sig.cpu_util);
            }
        } else {
            snprintf(proc_data_str, sizeof(proc_data_str) - 1, "No process OS stats.");
        }

        // DC Node Power
        POWER = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.DC_power : sig->signature.DC_power;

        // Time
        // TODO: What is this time?
        TIME = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.time : sig->signature.time;

        // GFlop/s
        // If enabled, each process reports its GFlop/s
        FLOPS = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.Gflops : sig->signature.Gflops;

        // CPU
        if (per_proc_verb_file) {
            // Each process computes its average CPU frequency based on its own affinity mask
            cpu_set_t *mask = &sig_shared_region[my_node_id].cpu_mask;
            ulong freq_avg, cpus_count;

            pstate_freqtoavg(*mask, lib_shared_region->avg_cpufreq, metrics_get(MET_CPUFREQ)->devs_count, &freq_avg,
                             &cpus_count);

            AVGF = (float) freq_avg / 1000000.0;
        } else {
            AVGF = (float) sig->signature.avg_f / 1000000.0;
        }

        // AVX
        if (per_proc_verb_file) {
            // Each process computes its VPI
            compute_ssig_vpi(&VPI, &sig_shared_region[my_node_id].sig);
        } else {
            compute_sig_vpi(&VPI, &sig->signature);
        }

        double GPU_POWER = 0;
        GPU_FREQ         = 0;
        GPU_UTIL         = 0;

        // Previous CPU frequency
        if (prevf > 0) {
            PFREQ = (float) prevf / 1000000.0;
        }
        // Next CPU frequency
        if (new_freq > 0) {
            NFREQ = (float) new_freq / 1000000.0;
        }
#if USE_GPUS
        double gpu_pwr_sum = 0;
        float gpu_freq_sum = 0;

        int gpu_cnt = sig->signature.gpu_sig.num_gpus;
        if (gpu_cnt > 0) {

            // By now, the below verboses are not showed with using_verb_files set to 1

            char gpu_sig_hdr[] = "----------- GPU signature -----------\nGPU Id. Power (W) Util (%) Freq (GHz)\0";
            verbose_master(GPU_SIG_VRB_LVL, "%s", gpu_sig_hdr);

            for (int i = 0; i < gpu_cnt; i++) {
#if WF_SUPPORT
                verbose_master(GPU_SIG_VRB_LVL, "%7d %-9.2f %-8lu %-10lu %-9.2f %lu)", i,
                               sig->signature.gpu_sig.gpu_data[i].GPU_power,
                               sig->signature.gpu_sig.gpu_data[i].GPU_util, sig->signature.gpu_sig.gpu_data[i].GPU_freq,
                               sig->signature.gpu_sig.gpu_data[i].GPU_GFlops,
                               sig->signature.gpu_sig.gpu_data[i].GPU_temp);
#else
                verbose_master(
                    GPU_SIG_VRB_LVL, "%7d %-9.2f %-8lu %-10lu)", i, sig->signature.gpu_sig.gpu_data[i].GPU_power,
                    sig->signature.gpu_sig.gpu_data[i].GPU_util, sig->signature.gpu_sig.gpu_data[i].GPU_freq);
#endif

                gpu_pwr_sum += sig->signature.gpu_sig.gpu_data[i].GPU_power;
                gpu_freq_sum += (float) sig->signature.gpu_sig.gpu_data[i].GPU_freq;

                if (sig->signature.gpu_sig.gpu_data[i].GPU_util) {

                    gpu_used_cnt += 1;
                    GPU_POWER += sig->signature.gpu_sig.gpu_data[i].GPU_power;
                    GPU_FREQ += sig->signature.gpu_sig.gpu_data[i].GPU_freq;
                    GPU_UTIL += sig->signature.gpu_sig.gpu_data[i].GPU_util;
                    GPU_mem_util += sig->signature.gpu_sig.gpu_data[i].GPU_mem_util;
#if WF_SUPPORT
                    GPU_GFlops += sig->signature.gpu_sig.gpu_data[i].GPU_GFlops;
                    GPU_temp += sig->signature.gpu_sig.gpu_data[i].GPU_temp;
#endif
                }
            }
            verbose_master(GPU_SIG_VRB_LVL, "-------------------------------------");

            if (gpu_used_cnt) {
                GPU_POWER /= (float) gpu_used_cnt;
                AVGGPUF      = (float) GPU_FREQ / (float) (gpu_used_cnt * 1000000.0);
                GPU_UTIL     = GPU_UTIL / gpu_used_cnt;
                GPU_mem_util = GPU_mem_util / gpu_used_cnt;
#if WF_SUPPORT
                GPU_temp = GPU_temp / gpu_used_cnt;
#endif
            }

            gpu_freq_sum /= gpu_cnt;
            snprintf(gpu_buff, sizeof(gpu_buff), "\n\t(total gpupower %.0lf avg_gfreq %.2fGHz)", gpu_pwr_sum,
                     gpu_freq_sum / 1000000.0);
        }
#endif

        debug("%swriting app info%s", COL_YLW, COL_CLR);
        verbose(2, "EAR%s[%u](%s) at %.2f in %s: LoopID=%lu, LoopSize=%lu-%lu, iterations=%d", mode, AID, aname, PFREQ,
                nname, sig->id.event, sig->id.size, sig->id.level, iterations);

        debug("%swriting nname at elapsed%s", COL_YLW, COL_CLR)
            verbosen(SIG_VRB_LVL, "EAR%s[%u](%s) at %.2f/%.2f in %s MR[%d] elapsed %lu sec.: ", mode, AID, aname, PFREQ,
                     sel_mem_f, nname, master_rank, elapsed);
        verbosen(SIG_VRB_LVL, "%s", COL_GRE);

        snprintf(gflops_str, sizeof(gflops_str), "%.2lf", FLOPS);

#if USE_GPUS
        char gpu_temp_str[128] = "";
        if (gpu_cnt > 0) {
#if WF_SUPPORT
            char gpu_glops[64];
            snprintf(gpu_glops, sizeof(gpu_glops), "/%.2f", GPU_GFlops);
            strcat(gflops_str, gpu_glops);
            sprintf(gpu_temp_str, "GPU temp %lu (avg)", GPU_temp);
#endif
            verbosen(SIG_VRB_LVL, "%s", gpu_buff);
        }
#endif

        strcpy(gpu_buff, " ");
        if (GPU_UTIL) {
            snprintf(gpu_buff, sizeof(gpu_buff),
                     "\n\t(Used GPUS %lu GPU_power %.2lfW (avg) "
                     "GPU_freq %.2fGHz (avg) GPU_util %lu (avg) GPU_mem_util %lu (avg) %s)",
                     gpu_used_cnt, GPU_POWER, AVGGPUF, GPU_UTIL, GPU_mem_util, gpu_temp_str);
        }

        debug("%swriting CPI GBS power...%s", COL_YLW, COL_CLR);

        verbose(SIG_VRB_LVL, "Loop time: %.3lfs DC Power: %s%.1lfW%s DRAM power: %.2lfW PCK power: %.2lfW", TIME,
                COL_MGT, POWER, COL_GRE, sig->signature.DRAM_power, sig->signature.PCK_power);

        verbosen(SIG_VRB_LVL, "\tavg. CPUFreq: %.2fGHz avg. IMCFreq: %.2fGHz", AVGF, imcf);
        if (new_freq > 0) {
            verbose(SIG_VRB_LVL, " Next Frequency %.2f", NFREQ);
        } else {
            verbose(SIG_VRB_LVL, "");
        }

        verbose(SIG_VRB_LVL, "\tCPI: %.3lf VPI: %.2f, Mem. bwidth: %.2lfGB/s TPI: %.2lf GFLOPS: %s I/O: %.2fMB/s %s %s",
                CPI, VPI, GBS, sig->signature.TPI, gflops_str, IO_MBS, pmpi_txt, gpu_buff);

        /* Cache metrics */
        /* Misses */
        ull l1d_misses =
            (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.l1d_misses : sig->signature.cache.l1d_misses;
        ull l2_misses =
            (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.l2_misses : sig->signature.cache.l2_misses;
        ull l3_misses =
            (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.l3_misses : sig->signature.cache.l3_misses;
        ull ll_misses =
            (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.ll_misses : sig->signature.cache.ll_misses;

        /* Giga-misses per second */
        float l1d_miss_sec = l1d_misses / (float) (TIME * 1000000000);
        float l2_miss_sec  = l2_misses / (float) (TIME * 1000000000);
        float l3_miss_sec  = l3_misses / (float) (TIME * 1000000000);
        float ll_miss_sec  = ll_misses / (float) (TIME * 1000000000);

        /* Hits */
        ull l1d_hits =
            (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.l1d_hits : sig->signature.cache.l1d_hits;
        ull l2_hits =
            (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.l2_hits : sig->signature.cache.l2_hits;
        ull l3_hits =
            (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.l3_hits : sig->signature.cache.l3_hits;
        ull ll_hits =
            (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.ll_hits : sig->signature.cache.ll_hits;

        /* Giga-hits per second */
        float l1d_hit_sec = l1d_hits / (float) (TIME * 1000000000);
        float l2_hit_sec  = l2_hits / (float) (TIME * 1000000000);
        float l3_hit_sec  = l3_hits / (float) (TIME * 1000000000);
        float ll_hit_sec  = ll_hits / (float) (TIME * 1000000000);

        /* Accesses */
        ull l1d_accesses = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.l1d_accesses
                                                : sig->signature.cache.l1d_accesses;
        ull l2_accesses  = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.l2_accesses
                                                : sig->signature.cache.l2_accesses;
        ull l3_accesses  = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.l3_accesses
                                                : sig->signature.cache.l3_accesses;
        ull ll_accesses  = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.cache.ll_accesses
                                                : sig->signature.cache.ll_accesses;

        /* Giga-accesses per second */
        float l1d_access_sec = l1d_accesses / (float) (TIME * 1000000000);
        float l2_access_sec  = l2_accesses / (float) (TIME * 1000000000);
        float l3_access_sec  = l3_accesses / (float) (TIME * 1000000000);
        float ll_access_sec  = ll_accesses / (float) (TIME * 1000000000);

        /* Miss and hit rates are not stored in per-process shared signature, so
         * in the case we report per-process signature, we must compute it */
        double l1d_miss_rate = (per_proc_verb_file && l1d_accesses) ? l1d_misses / (double) l1d_accesses
                                                                    : sig->signature.cache.l1d_miss_rate;
        double l2_miss_rate =
            (per_proc_verb_file && l2_accesses) ? l2_misses / (double) l2_accesses : sig->signature.cache.l2_miss_rate;
        double l3_miss_rate =
            (per_proc_verb_file && l3_accesses) ? l3_misses / (double) l3_accesses : sig->signature.cache.l3_miss_rate;
        double ll_miss_rate =
            (per_proc_verb_file && ll_accesses) ? ll_misses / (double) ll_accesses : sig->signature.cache.ll_miss_rate;

        double l1d_hit_rate =
            (per_proc_verb_file && l1d_accesses) ? l1d_hits / (double) l1d_accesses : sig->signature.cache.l1d_hit_rate;
        double l2_hit_rate =
            (per_proc_verb_file && l2_accesses) ? l2_hits / (double) l2_accesses : sig->signature.cache.l2_hit_rate;
        double l3_hit_rate =
            (per_proc_verb_file && l3_accesses) ? l3_hits / (double) l3_accesses : sig->signature.cache.l3_hit_rate;
        double ll_hit_rate =
            (per_proc_verb_file && ll_accesses) ? ll_hits / (double) ll_accesses : sig->signature.cache.ll_hit_rate;

        verbose(SIG_VRB_LVL,
                "\t          Cache: %-8s %-8s %-8s %-8s\n"
                "\t  giga-misses/s: %-8.2f %-8.2f %-8.2f %-8.2f\n"
                "\t    giga-hits/s: %-8.2f %-8.2f %-8.2f %-8.2f\n"
                "\tgiga-accesses/s: %-8.2f %-8.2f %-8.2f %-8.2f\n"
                "\t      miss rate: %-8.2f %-8.2f %-8.2f %-8.2f\n"
                "\t       hit rate: %-8.2f %-8.2f %-8.2f %-8.2f",
                "L1d", "L2", "L3", "LL", l1d_miss_sec, l2_miss_sec, l3_miss_sec, ll_miss_sec, l1d_hit_sec, l2_hit_sec,
                l3_hit_sec, ll_hit_sec, l1d_access_sec, l2_access_sec, l3_access_sec, ll_access_sec, l1d_miss_rate,
                l2_miss_rate, l3_miss_rate, ll_miss_rate, l1d_hit_rate, l2_hit_rate, l3_hit_rate, ll_hit_rate);

        ullong n_mpi_calls = (per_proc_verb_file) ? sig_shared_region[my_node_id].mpi_info.total_mpi_calls : total_mpi;

        verbose(SIG_VRB_LVL, "\tTotal MPI calls %llu", n_mpi_calls);
        verbose(SIG_VRB_LVL, "\tIO DATA: %s", io_info);
        verbosen(SIG_VRB_LVL, "\t%s", proc_data_str);

#if WF_SUPPORT
        verbosen(SIG_VRB_LVL, "\n\tSocket CPU temp. (celsius) CPU power (W) DRAM power (W)");
        for (uint s = 0; s < sig->signature.cpu_sig.devs_count; s++) {
            verbosen(SIG_VRB_LVL, "\n\t%6u %-19lld %-13.2lf %-14.2lf", s, sig->signature.cpu_sig.temp[s],
                     sig->signature.cpu_sig.cpu_power[s], sig->signature.cpu_sig.dram_power[s]);
        }
#endif
        verbose(SIG_VRB_LVL, "%s", COL_CLR);
    }
}

void state_report_traces(int master_rank, int my_rank, int lid, loop_t *lsig, ulong freq, ulong status)
{
    if (traces_are_on()) {
        traces_new_signature(my_rank, lid, &lsig->signature);
        // if (freq > 0 ) traces_frequency(my_rank, lid, freq);
        traces_policy_state(my_rank, lid, status);
    }
}

void state_report_traces_state(int master_rank, int my_rank, int lid, ulong status)
{
    if (traces_are_on())
        traces_policy_state(my_rank, lid, status);
}

/* Auxiliary functions */

void state_print_policy_state(int master_rank, int st)
{
#if 0
	if (master_rank >= 0) {
		switch (st) {
		case EAR_POLICY_READY:
			verbose(2, "Policy new state EAR_POLICY_READY");
			break;
		case EAR_POLICY_CONTINUE:
			verbose(2, "Policy new state EAR_POLICY_CONTINUE");
			break;
		case EAR_POLICY_GLOBAL_EV:
			verbose(2, "Policy new state EAR_POLICY_GLOBAL_EV");
			break;
		case EAR_POLICY_GLOBAL_READY:
			verbose(2, "Policy new state EAR_POLICY_GLOBAL_READY");
			break;
		}
	}
#endif
}

#ifdef SHOW_DEBUGS
static void debug_loop_signature(char *title, signature_t *loop)
{
    float avg_f = (float) loop->avg_f / 1000000.0;

    debug("(%s) Avg. freq: %.2lf (GHz), CPI/TPI: %0.3lf/%0.3lf, GBs: %0.3lf, DC power: %0.3lf, time: %0.3lf, GFLOPS: "
          "%0.3lf",
          title, avg_f, loop->CPI, loop->TPI, loop->GBS, loop->DC_power, loop->time, loop->Gflops);
}
#endif

void states_comm_configure_performance_accuracy(cluster_conf_t *cluster_conf, ulong *hw_perf_acc, uint *library_period)
{
    // Default: ear.conf
    *hw_perf_acc = (ulong) cluster_conf->min_time_perf_acc;
    debug("Default values for hw_perf_acc and library_period: %lu and %u", *hw_perf_acc, *library_period);

    // Try to change LibraryPeriod by reading an environment variable
    char *my_lib_period  = ear_getenv("EARL_TIME_LOOP_SIGNATURE");
    long my_lib_period_l = 0;

    if (my_lib_period) {
        my_lib_period_l = atol(my_lib_period);
    }

    if (my_lib_period_l > 0) {
        // *hw_perf_acc = (ulong) my_lib_period_l;
        *library_period = (ulong) my_lib_period_l;
        debug("library_period overwritten by environment variable: %u", *library_period);
    }
    // Hardware limit
    ulong architecture_min_perf_accuracy_time = 0;
    if (masters_info.my_master_rank >= 0 && eards_connected()) {
        // eards_node_energy_frequency returns the granularity at which the energy
        // plugin can read the power but we increment this period to minimize the
        // error in the power read.
        architecture_min_perf_accuracy_time = eards_node_energy_frequency() * 10;
        if (architecture_min_perf_accuracy_time == (ulong) EAR_ERROR) {
            architecture_min_perf_accuracy_time = *hw_perf_acc;
        }

        debug("eard hw_perf_acc: %lu", architecture_min_perf_accuracy_time);
    }
    // hw_perf_acc lower bounded by hardware
    *hw_perf_acc = ear_max(*hw_perf_acc, architecture_min_perf_accuracy_time);
    debug("hw_perf_acc bounded by energy plug-in: %lu", *hw_perf_acc);

    // library_period is in seconds, hw_perf_acc is in microseconds
    *library_period = ear_max(*library_period, *hw_perf_acc / 1000000);
    *hw_perf_acc    = *library_period * 1000000;
    debug("library_period: %u | hw_perf_acc %lu", *library_period, *hw_perf_acc);
}
