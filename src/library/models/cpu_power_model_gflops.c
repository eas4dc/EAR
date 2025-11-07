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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/hardware/architecture.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/shared_configuration.h>
#include <library/common/library_shared_data.h>
#include <library/common/verbose_lib.h>
#include <library/models/cpu_power_model_common.h>

#include <metrics/flops/flops.h>

static uint arch_model_loaded = 0;
static uint cpu_count, cache_line_size;
static node_sharing_strategy_t
    node_sharing_strategy; // 'node' strategy is the default since it is what we had until now.

state_t cpu_power_model_init_arch(settings_conf_t *libconf, conf_install_t *conf, architecture_t *arch,
                                  node_sharing_strategy_t strategy)
{
    cpu_count             = arch->top.cpu_count;
    cache_line_size       = arch->top.cache_line_size;
    arch_model_loaded     = 1;
    node_sharing_strategy = strategy;
    verbose_info_master("GFLOPS CPU power model loaded. Power assignment strategy: %s",
                        (node_sharing_strategy == node) ? "node" : "job");
    return EAR_SUCCESS;
}

state_t cpu_power_model_status_arch()
{
    debug("cpu_power_model_status_arch");
    // TODO: Can we check whether the model is loaded?
    return EAR_SUCCESS;
}

state_t cpu_power_model_project_arch(lib_shared_data_t *data, shsignature_t *sig, node_mgr_sh_data_t *nmgr,
                                     uint node_mgr_index)
{
    verbose_info2_master("GFLOPS CPU power model projection");

    if (!arch_model_loaded)
        return EAR_ERROR;

    if (data == NULL)
        return EAR_SUCCESS;

    int cpus_in_node = cpu_count;
    double job_power_ratio;
    double dram_power[MAX_CPUS_SUPPORTED];

    double my_job_GBS, my_job_DRAM_power, my_job_TPI;
    double cpu_power_ratio;
    uint used_cpus    = 0;
    uint my_used_cpus = 0;

    signature_t *lsig = &data->node_signature;

    if ((lsig == NULL) || (sig == NULL) || (nmgr == NULL))
        return EAR_SUCCESS;

    double process_power_ratio[MAX_CPUS_SUPPORTED];
    memset(process_power_ratio, 0, sizeof(double) * MAX_CPUS_SUPPORTED);

    double gbs[MAX_CPUS_SUPPORTED];
    memset(gbs, 0, sizeof(double) * MAX_CPUS_SUPPORTED);

    double tpi[MAX_CPUS_SUPPORTED];
    memset(tpi, 0, sizeof(double) * MAX_CPUS_SUPPORTED);

    ull L3[MAX_CPUS_SUPPORTED];
    memset(L3, 0, sizeof(ull) * MAX_CPUS_SUPPORTED);

    double power_estimations[MAX_CPUS_SUPPORTED];
    memset(power_estimations, 0, sizeof(double) * MAX_CPUS_SUPPORTED);

    float gflops[MAX_CPUS_SUPPORTED];
    memset(gflops, 0, sizeof(float) * MAX_CPUS_SUPPORTED);

    ullong inst[MAX_CPUS_SUPPORTED];
    memset(inst, 0, sizeof(ullong) * MAX_CPUS_SUPPORTED);

    if (node_mgr_earl_apps() > MAX_CPUS_SUPPORTED) {
        verbose(0, "EARL ERROR, more apps than CPUS--> return");
        return EAR_ERROR;
    }

    /* This flag is explicitly requested by users */
    if (!data->estimate_node_metrics) {
        data->job_signature.PCK_power  = 0;
        data->job_signature.GBS        = 0;
        data->job_signature.DC_power   = 0;
        data->job_signature.DRAM_power = 0;
        data->job_signature.TPI        = 0;
        for (uint lp = 0; lp < data->num_processes; lp++) {
            sig[lp].sig.GBS        = 0;
            sig[lp].sig.TPI        = 0;
            sig[lp].sig.DRAM_power = 0;
            sig[lp].sig.DC_power   = 0;
            sig[lp].sig.PCK_power  = 0;
        }
        verbose_info2_master("Node sharing metrics not estimated for job %d.", node_mgr_index);
        return EAR_SUCCESS;
    }

    /* compute_total_node_fff compute the metrics only for the calling job. It is needed to iterate over the jobs to
     * have per node */
    uint job_in_process[MAX_CPUS_SUPPORTED];
    uint node_process = 0;

    float total_gflops = 0.0;

    ull node_L3 = 0;

    for (uint j = 0; j < node_mgr_earl_apps(); j++) {
        /* DC_power condition means the signature is ready.
         * estimate_node_metrics is used to not give neither power nor GB/s.
         * It is usually used for the master application in a master/worker workflow
         * use case. */
        if (nmgr[j].creation_time && nmgr[j].libsh != NULL && nmgr[j].shsig != NULL &&
            nmgr[j].libsh->node_signature.DC_power && nmgr[j].libsh->estimate_node_metrics) {
            used_cpus += nmgr[j].libsh->num_cpus;

            if (j == node_mgr_index)
                my_used_cpus += nmgr[j].libsh->num_cpus;

            for (uint lp = 0; lp < nmgr[j].libsh->num_processes; lp++) {
                if (nmgr[j].shsig[lp].sig.DC_power > 0) {
                    L3[node_process]     = nmgr[j].shsig[lp].sig.L3_misses;
                    inst[node_process]   = nmgr[j].shsig[lp].sig.instructions;
                    gflops[node_process] = nmgr[j].shsig[lp].sig.Gflops;
                    total_gflops += gflops[node_process];
                    node_L3 += L3[node_process];
                }
                job_in_process[node_process] = j;
                node_process++;
            }
        }
    }

    uint lp           = 0;
    uint cj           = 0;
    my_job_GBS        = 0;
    my_job_TPI        = 0;
    my_job_DRAM_power = 0;

    /* DRAM power and GBS computation **/
    for (uint j = 0; j < node_process; j++) {
        /* GBs is per-node, we can use our own GBs */
        if (node_L3) {
            float mem_ratio = (float) L3[j] / (float) node_L3;
            gbs[j]          = mem_ratio * lsig->GBS;
            dram_power[j]   = mem_ratio * lsig->DRAM_power;
            if (inst[j])
                tpi[j] = (mem_ratio * data->cas_counters * cache_line_size) / inst[j];
            else
                tpi[j] = (double) 0;
        } else {
            /* If there is no cache misses, we distribute the GBs proportionally to the number of cpus, naive approach
             */
            cj = job_in_process[j];
            if ((used_cpus == 0) || (nmgr[cj].libsh->num_processes == 0) || (lsig->GBS == 0)) {
                gbs[j]        = 0;
                dram_power[j] = 0;
                tpi[j]        = 0;
            } else {
                float mem_ratio =
                    ((float) nmgr[cj].libsh->num_cpus / (float) used_cpus) / (float) nmgr[cj].libsh->num_processes;
                gbs[j]        = lsig->GBS * mem_ratio;
                dram_power[j] = (gbs[j] / lsig->GBS) * lsig->DRAM_power;
                tpi[j]        = lsig->TPI * mem_ratio;
            }
        }

        if ((L3[j] || gbs[j]) && (job_in_process[j] == node_mgr_index)) {
            my_job_GBS += gbs[j];
            my_job_DRAM_power += dram_power[j];
            my_job_TPI += tpi[j];

            sig[lp].sig.GBS        = gbs[j];
            sig[lp].sig.TPI        = tpi[j];
            sig[lp].sig.DRAM_power = dram_power[j];

            debug("Process %d/%d GBS %lf", job_in_process[j], lp, sig[lp].sig.GBS);

            lp++;
        }
    }

    data->job_signature.GBS        = my_job_GBS;
    data->job_signature.TPI        = my_job_TPI;
    data->job_signature.DRAM_power = my_job_DRAM_power;

    debug("My job[%d] GB/s is %.2lf TPI %.2lf DRAM %lf", node_mgr_index, data->job_signature.GBS,
          data->job_signature.TPI, data->job_signature.DRAM_power);

    /* Power : Power is estimated based on CPU activity and Memory activity.
     * Less CPI means more CPU activity and more
     * L3 means more Mem actitivy */

    /* Per job in node loop */
    double CPU_power;
    double GPU_power = 0;
    CPU_power        = lsig->DC_power;
#if USE_GPUS
    for (uint gid = 0; gid < lsig->gpu_sig.num_gpus; gid++) {
        GPU_power += lsig->gpu_sig.gpu_data[gid].GPU_power;
    }
#endif
    if (CPU_power > GPU_power) {
        CPU_power -= GPU_power;
    } else {
        /* This case should not happen */
        if (lsig->PCK_power > 0)
            CPU_power = lsig->PCK_power + lsig->DRAM_power;
        else
            CPU_power = 300;
    }

    float C = CPU_power - (lsig->DRAM_power + lsig->PCK_power);

    debug("Baseline CPU power %lf Constant C = %f", CPU_power, C);

    /* Per job in node loop */
    double total_power_estimated = 0;

    lp = 0;

    if (cpus_in_node == 0)
        return EAR_ERROR;

    for (uint j = 0; j < node_mgr_earl_apps(); j++) {

        /* If job is active we compute its power estimated */
        if (nmgr[j].creation_time && (nmgr[j].libsh != NULL) && (nmgr[j].shsig != NULL) &&
            nmgr[j].libsh->estimate_node_metrics) {

            for (uint proc = 0; proc < nmgr[j].libsh->num_processes; proc++) {
                float ratio_p = (float) nmgr[j].shsig[proc].num_cpus / cpus_in_node;

                if (total_gflops)
                    power_estimations[lp] =
                        (gflops[j] / total_gflops) * ratio_p * lsig->PCK_power + dram_power[j] + C * ratio_p;
                else
                    power_estimations[lp] = CPU_power / ratio_p + C * ratio_p;

                total_power_estimated += power_estimations[lp];

                lp++;
            }
        }
    }

    job_power_ratio          = 0.0;
    cpu_power_ratio          = (double) my_used_cpus / (double) cpu_count;
    double GPU_process_power = 0;

    lp = 0;
    for (uint proc = 0; proc < node_process; proc++) {
        GPU_process_power = 0;
        if ((job_in_process[proc] == node_mgr_index) && (power_estimations[proc] > 0)) {

            if (total_power_estimated) {
                job_power_ratio += power_estimations[proc] / total_power_estimated;
                process_power_ratio[proc] = power_estimations[proc] / total_power_estimated;
            }

            if (GPU_power && data->num_processes)
                GPU_process_power = GPU_power / data->num_processes;

            sig[lp].sig.DC_power =
                (CPU_power * process_power_ratio[proc]) + GPU_process_power; // what about GPU power in processes
            sig[lp].sig.PCK_power = process_power_ratio[proc] * lsig->PCK_power;

            debug("Power process %d/%d is %lf. PCK power %lf", job_in_process[proc], lp, sig[lp].sig.DC_power,
                  sig[lp].sig.PCK_power);

            lp++;
        }
    }

    data->job_signature.DC_power  = (CPU_power * job_power_ratio) + GPU_power;
    data->job_signature.PCK_power = job_power_ratio * lsig->PCK_power;

    data->job_signature.DC_job_power  = (CPU_power * cpu_power_ratio) + GPU_power;
    data->job_signature.PCK_job_power = cpu_power_ratio * lsig->PCK_power;

    verbose_info2_master("Job %u | DC node power (node ratio) %lf | CPU pwr %lf GPU pwr %lf ratio %lf PCK pwr %lf",
                         node_mgr_index, data->job_signature.DC_power, CPU_power, GPU_power, job_power_ratio,
                         data->job_signature.PCK_power);

    verbose_info2_master("Job %u | DC node power (job ratio) %lf | CPU pwr %lf GPU pwr %lf ratio %lf PCK pwr %lf",
                         node_mgr_index, data->job_signature.DC_job_power, CPU_power, GPU_power, cpu_power_ratio,
                         data->job_signature.PCK_power);

    if (node_sharing_strategy == job) {
        data->job_signature.DC_power  = data->job_signature.DC_job_power;
        data->job_signature.PCK_power = data->job_signature.PCK_job_power;
    }

    return EAR_SUCCESS;
}
