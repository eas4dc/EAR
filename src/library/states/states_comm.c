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

#if MPI
#include <mpi.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

// #define SHOW_DEBUGS 1
#include <metrics/common/pstate.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/colors.h>
#include <common/math_operations.h>
#include <common/types/log.h>
#include <common/types/application.h>
#include <library/common/externs.h>
#include <library/common/global_comm.h>
#include <library/metrics/metrics.h>
#include <library/tracer/tracer.h>
#include <library/common/verbose_lib.h>

extern masters_info_t masters_info;
extern float ratio_PPN;
extern unsigned long long int total_mpi;
extern uint using_verb_files;
extern uint per_proc_verb_file;

#ifdef SHOW_DEBUGS
static void debug_loop_signature(char *title, signature_t *loop);
#endif

void state_verbose_signature(loop_t *sig, int master_rank, int show, char *aname, char *nname,
        int iterations, ulong prevf, ulong new_freq, char *user_mode)
{
    double CPI, GBS, POWER, TIME, VPI, FLOPS;
    float AVGF = 0, AVGGPUF = 0, PFREQ = 0, NFREQ = 0;
    float l1_sec=0,l2_sec=0,l3_sec=0;
    float IO_MBS;
    ulong GPU_UTIL = 0, GPU_FREQ = 0, gpu_used_cnt=0, GPU_mem_util = 0;
    ulong elapsed;
    timestamp end;
    char mode[32];
#if 0
    sig_ext_t *se = (sig_ext_t *)sig->signature.sig_ext;
#endif

    if (VERB_GET_LV() == 0) return;
    sprintf(mode, "%s", "");
    if (VERB_GET_LV() >= 2){
      strcpy(mode, user_mode);
    }

    timestamp_getfast(&end);
    elapsed=timestamp_diff(&end, &ear_application_time_init, TIME_SECS);

    float imcf;
    char gpu_buff[256];

    /* master_rank == 0: The master node rank (is who verboses if verb_level >= 1).
     * show: FLAG_VERBOSE_SIGNATURE (?)
     * using_verb_files: FLAG_EARL_VERBOSE_PATH
     * per_proc_verb_file: HACK_PROCS_VERB_FILES (per_proc_verb_file == 1 => using_verb_files == 1)
     */
    if (master_rank == 0 || show || using_verb_files) {
        
        // MPI info
		sig_ext_t *sigex      = (sig_ext_t *) sig->signature.sig_ext;
        // Only the master shows that information as is who computed it
        // TODO: Does the shared signature have these metrics after the master computed it?
        float      max_mpi    = (master_rank >= 0) ? (sigex->max_mpi) : 0.0;
        float      min_mpi    = (master_rank >= 0) ? (sigex->min_mpi) : 0.0;

        // If enabled, each process reports its %MPI
        float perc_mpi = (per_proc_verb_file) ? (sig_shared_region[my_node_id].mpi_info.perc_mpi) : sig->signature.perc_MPI;

        char pmpi_txt[256];
        if (perc_mpi > 0.0) {
            if (per_proc_verb_file) {
                // If enabled, each process reports its %MPI
                // TODO: Depending on how the above TODO is resolved, below code may must be changed
                snprintf(pmpi_txt, sizeof(pmpi_txt), "MPI_time/exec_Time: %.1f", perc_mpi);
            } else {
                // Since when only master verboses, it prints the average across all processes of the job
                snprintf(pmpi_txt, sizeof(pmpi_txt), "MPI_time/exec_Time (max/avg/min): %.1f/%.1f/%.1f %%",
                         max_mpi, perc_mpi, min_mpi);
            }
        } else {
            sprintf(pmpi_txt, "No MPI");
        } // MPI info

        // IMC
        imcf = (per_proc_verb_file) ? (float) lib_shared_region->job_signature.avg_imc_f / 1000000.0 : (float) sig->signature.avg_imc_f / 1000000.0;

        // CPI
        // If enabled, each process reports its CPI
        CPI = (per_proc_verb_file) ? (double) sig_shared_region[my_node_id].sig.CPI : sig->signature.CPI;

        // GB/s
        GBS = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.GBS : sig->signature.GBS;

        // I/O MB/s
#if 0
        io_data_t *io_data = (per_proc_verb_file) ? &sig_shared_region[my_node_id].sig.iod : &se->iod;
#endif
        // If enabled, each process has its own I/O data
        io_data_t *io_data = (per_proc_verb_file) ? &sig_shared_region[my_node_id].sig.iod : &sigex->iod;

        char io_info[256];
        if (sigex != NULL) {
            io_tostr(io_data, io_info, sizeof(io_info)); // io_data declared above
        } else {
            sprintf(io_info, "No IO data");
        }

        // TODO: period or time ? time is the time per iteration. This metric is computed periodically by a thread.
        // If enabled each process computes it I/O bandwidth, otherwise the node I/O bandwidth is already computed
        IO_MBS = (per_proc_verb_file) ? ((float) io_data->rchar / (float) (1024 * 1024) + (float) io_data->wchar / (float) (1024 * 1024)) / sig_shared_region[my_node_id].period : (float) sig->signature.IO_MBS;

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

            pstate_freqtoavg(*mask, lib_shared_region->avg_cpufreq,
                             metrics_get(MET_CPUFREQ)->devs_count, &freq_avg, &cpus_count);

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
        GPU_FREQ=0; GPU_UTIL=0; 

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
            verbose_master(3, "%s", gpu_sig_hdr);

            for(int i = 0; i < gpu_cnt; i++) { 

                verbose_master(3, "%7d %-9.2f %-8lu %-10lu)", i,
                        sig->signature.gpu_sig.gpu_data[i].GPU_power,
                        sig->signature.gpu_sig.gpu_data[i].GPU_util,
                        sig->signature.gpu_sig.gpu_data[i].GPU_freq);

                gpu_pwr_sum  += sig->signature.gpu_sig.gpu_data[i].GPU_power;
                gpu_freq_sum += (float) sig->signature.gpu_sig.gpu_data[i].GPU_freq;

                if (sig->signature.gpu_sig.gpu_data[i].GPU_util) {

                    gpu_used_cnt += 1;
                    GPU_POWER    += sig->signature.gpu_sig.gpu_data[i].GPU_power; 
                    GPU_FREQ     += sig->signature.gpu_sig.gpu_data[i].GPU_freq; 
                    GPU_UTIL     += sig->signature.gpu_sig.gpu_data[i].GPU_util; 
                    GPU_mem_util     += sig->signature.gpu_sig.gpu_data[i].GPU_mem_util; 
                }
            } 
            verbose_master(3, "-------------------------------------");

            if (gpu_used_cnt) {
                GPU_POWER /= (float) gpu_used_cnt;
                AVGGPUF = (float) GPU_FREQ / (float) (gpu_used_cnt * 1000000.0); 
                GPU_UTIL = GPU_UTIL / gpu_used_cnt; 
                GPU_mem_util = GPU_mem_util / gpu_used_cnt; 
            }

            gpu_freq_sum /= gpu_cnt;
            snprintf(gpu_buff, sizeof(gpu_buff), "\n\t(total gpupower %.0lf avg_gfreq %.2fGHz)",
                    gpu_pwr_sum, gpu_freq_sum / 1000000.0);
        }
#endif

        debug("%swriting app info%s", COL_YLW, COL_CLR);
        verbose(2, "EAR%s(%s) at %.2f in %s: LoopID=%lu, LoopSize=%lu-%lu, iterations=%d",
                mode, aname, PFREQ, nname, sig->id.event, sig->id.size, sig->id.level, iterations); 

        debug("%swriting nname at elapsed%s", COL_YLW, COL_CLR)
        verbosen(1, "EAR%s(%s) at %.3f in %s MR[%d] elapsed %lu sec.: ",
                mode, aname, PFREQ, nname, master_rank, elapsed); 
        verbosen(1, "%s", COL_GRE);

#if USE_GPUS
        if (gpu_cnt > 0 ) verbosen(2,"%s", gpu_buff);
#endif

        strcpy(gpu_buff," ");
        if (GPU_UTIL) {
            snprintf(gpu_buff, sizeof(gpu_buff), "\n\t(Used GPUS %lu GPU_power %.2lfW (avg) "
                    "GPU_freq %.2fGHz (avg) GPU_util %lu (avg) GPU_mem_util %lu (avg))", gpu_used_cnt, GPU_POWER, AVGGPUF, GPU_UTIL, GPU_mem_util);
        }

        debug("%swriting CPI GBS power...%s", COL_YLW, COL_CLR);
        verbosen(1,"(CPI=%.3lf GBS=%.2lf Power=%.1lfW Time=%.3lfs\n\tAvgCPUF=%.2fGHz/AvgIMCF=%.2fGHz GFlop/s=%.2lf IO=%.2fMB/s %s) %s", CPI, GBS, POWER, TIME,  AVGF, imcf, FLOPS, IO_MBS, pmpi_txt, gpu_buff);

        if (new_freq > 0){ verbosen(1,"\n\t Next Frequency %.2f",NFREQ);}

        verbosen(2,"\n\t DRAM power %.2lf PCK power %.2lf",sig->signature.DRAM_power,sig->signature.PCK_power);

        ullong l1_miss = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.L1_misses : sig->signature.L1_misses;
        ullong l2_miss = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.L2_misses : sig->signature.L2_misses;
        ullong l3_miss = (per_proc_verb_file) ? sig_shared_region[my_node_id].sig.L3_misses : sig->signature.L3_misses;

        l1_sec = l1_miss/(TIME * 1024*1024*1024);
        l2_sec = l2_miss/(TIME * 1024*1024*1024);
        l3_sec = l3_miss/(TIME * 1024*1024*1024);

        verbosen(2,"\n\t L1GB/sec %.2f L2GB/sec %.2f L3GB/sec %.2f TPI %.2lf ",l1_sec,l2_sec,l3_sec, sig->signature.TPI);

        ullong n_mpi_calls = (per_proc_verb_file) ? sig_shared_region[my_node_id].mpi_info.total_mpi_calls : total_mpi;

        verbosen(2," VPI %.2f Total MPI calls %llu",VPI, n_mpi_calls);
        verbosen(2,"\n\t IO DATA: %s ",io_info);

        verbose(1,"%s",COL_CLR);
    }
}

void state_report_traces(int master_rank, int my_rank,int lid, loop_t *lsig,ulong freq,ulong status)
{
    #if SINGLE_CONNECTION
    if (master_rank >= 0 && traces_are_on()) {
        traces_new_signature(my_rank, lid, &lsig->signature);
        //if (freq > 0 ) traces_frequency(my_rank, lid, freq); 
        traces_policy_state(my_rank, lid, status);
    }
    #else
    if (traces_are_on()){
        traces_new_signature(my_rank, lid, &lsig->signature);
        //if (freq > 0 ) traces_frequency(my_rank, lid, freq); 
        traces_policy_state(my_rank, lid, status);
      
    }
    #endif
}

void state_report_traces_state(int master_rank, int my_rank, int lid, ulong status)
{
    #if SINGLE_CONNECTION
    if (master_rank >= 0 && traces_are_on()) traces_policy_state(my_rank, lid, status);
    #else
    if (traces_are_on()) traces_policy_state(my_rank, lid, status);
    #endif

}

/* Auxiliary functions */

void state_print_policy_state(int master_rank,int st)
{
#if 0
    if (master_rank >= 0){
        switch(st){
            case EAR_POLICY_READY:verbose(2,"Policy new state EAR_POLICY_READY");break;
            case EAR_POLICY_CONTINUE:verbose(2,"Policy new state EAR_POLICY_CONTINUE");break;
            case EAR_POLICY_GLOBAL_EV:verbose(2,"Policy new state EAR_POLICY_GLOBAL_EV");break;
            case EAR_POLICY_GLOBAL_READY: verbose(2,"Policy new state EAR_POLICY_GLOBAL_READY"); break;
        }
    }
#endif
}


#ifdef SHOW_DEBUGS
static void debug_loop_signature(char *title, signature_t *loop)
{
    float avg_f = (float) loop->avg_f / 1000000.0;

    debug("(%s) Avg. freq: %.2lf (GHz), CPI/TPI: %0.3lf/%0.3lf, GBs: %0.3lf, DC power: %0.3lf, time: %0.3lf, GFLOPS: %0.3lf",
            title, avg_f, loop->CPI, loop->TPI, loop->GBS, loop->DC_power, loop->time, loop->Gflops);
}
#endif

