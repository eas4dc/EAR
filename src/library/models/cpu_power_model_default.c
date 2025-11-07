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

#include <metrics/flops/flops.h>

#include <library/common/verbose_lib.h>
#include <library/models/cpu_power_model_common.h>
#include <library/models/cpu_power_model_default.h>

static intel_skl_t my_node_coeffs;
static uint arch_model_loaded = 0;
static uint cpu_count;
static uint cache_line_size;
static node_sharing_strategy_t
    node_sharing_strategy; // 'node' strategy is the default since it is what we had until now.

state_t cpu_power_model_init_arch(settings_conf_t *libconf, conf_install_t *conf, architecture_t *arch,
                                  node_sharing_strategy_t strategy)
{
    int fd, ret;

    verbose_info2_master("Default CPU power model. Tag %s, EAR etc %s", libconf->tag, conf->dir_conf);

    char model_path[MAX_PATH_SIZE];
    xsnprintf(model_path, sizeof(model_path), "%s/ear/coeffs/island%d/coeffs.cpumodel.%s", conf->dir_conf,
              libconf->island, libconf->tag);
    verbose_info_master("Default CPU power model coeffs: %s", model_path);

    fd = open(model_path, O_RDONLY);
    if (fd < 0) {
        verbose_error_master("Opening CPU power model coefficients file: %s", strerror(errno));
        return EAR_ERROR;
    }

    if ((ret = read(fd, &my_node_coeffs, sizeof(intel_skl_t))) != sizeof(intel_skl_t)) {
        if (ret < 0) {
            verbose_error_master("Reading CPU power model data: %s", strerror(errno));
        } else {
            verbose_error_master("Reading CPU power model data (%d): Expected %lu bytes.", ret, sizeof(intel_skl_t));
        }
        close(fd);
        return EAR_ERROR;
    }

    close(fd);
    cpu_count = arch->top.cpu_count;

    arch_model_loaded     = 1;
    cache_line_size       = arch->top.cache_line_size;
    node_sharing_strategy = strategy;
    verbose_info_master("Default CPU power model loaded. Power assignment strategy: %s",
                        (node_sharing_strategy == node) ? "node" : "job");

    return EAR_SUCCESS;
}

state_t cpu_power_model_status_arch()
{
    debug("cpu_power_model_status_arch");
    if (!arch_model_loaded)
        return EAR_ERROR;
    return EAR_SUCCESS;
}

state_t cpu_power_model_project_arch(lib_shared_data_t *data, shsignature_t *sig, node_mgr_sh_data_t *nmgr,
                                     uint node_mgr_index)
{
    verbose_info2_master("Default CPU power model projection");

    if (!arch_model_loaded)
        return EAR_ERROR;

    if (data == NULL)
        return EAR_SUCCESS;

    ull node_L3 = 0;

    int cpus_in_node = cpu_count;

    double power_estimations[MAX_CPUS_SUPPORTED];
    double dram_power[MAX_CPUS_SUPPORTED];
    double pck_power[MAX_CPUS_SUPPORTED];
    double ipc[MAX_CPUS_SUPPORTED];
    double tpi[MAX_CPUS_SUPPORTED];
    double gbs[MAX_CPUS_SUPPORTED];
    double vpi[MAX_CPUS_SUPPORTED];
    double cpu_power_ratio;
#if SHOW_DEBUGS
    double oldTPI;
#endif

    ulong f[MAX_CPUS_SUPPORTED];
    ullong inst[MAX_CPUS_SUPPORTED], totalinst = 0;
    ull L3[MAX_CPUS_SUPPORTED];
    uint job_in_process[MAX_CPUS_SUPPORTED];

    double job_power_ratio, process_power_ratio[MAX_CPUS_SUPPORTED];

    uint node_process = 0;

    signature_t *lsig;

    double my_job_GBS, my_job_DRAM_power, my_job_TPI;

    uint used_cpus    = 0;
    uint my_used_cpus = 0;

    if (node_mgr_earl_apps() > MAX_CPUS_SUPPORTED) {
        verbose_error_master("More applications (%d) than CPUs supported (%d)", node_mgr_earl_apps(),
                             MAX_CPUS_SUPPORTED);
        return EAR_ERROR;
    }

    lsig = &data->node_signature;

    memset(ipc, 0, sizeof(double) * MAX_CPUS_SUPPORTED);
    memset(process_power_ratio, 0, sizeof(double) * MAX_CPUS_SUPPORTED);
    memset(tpi, 0, sizeof(double) * MAX_CPUS_SUPPORTED);
    memset(vpi, 0, sizeof(double) * MAX_CPUS_SUPPORTED);
    memset(gbs, 0, sizeof(double) * MAX_CPUS_SUPPORTED);
    memset(f, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);
    memset(L3, 0, sizeof(ull) * MAX_CPUS_SUPPORTED);
    memset(power_estimations, 0, sizeof(double) * MAX_CPUS_SUPPORTED);
    memset(dram_power, 0, sizeof(double) * MAX_CPUS_SUPPORTED);
    memset(pck_power, 0, sizeof(double) * MAX_CPUS_SUPPORTED);
    memset(inst, 0, sizeof(ullong) * MAX_CPUS_SUPPORTED);

    if ((lsig == NULL) || (sig == NULL) || (nmgr == NULL))
        return EAR_SUCCESS;

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

    /* Normalize with the average of instructions */
    for (uint lp = 0; lp < data->num_processes; lp++) {
        totalinst += sig[lp].sig.instructions;
    }
    if (data->num_processes)
        totalinst /= data->num_processes;

#if SHOW_DEBUGS
    oldTPI = lsig->TPI;
#endif

    if (totalinst)
        lsig->TPI = (data->cas_counters * cache_line_size) / (double) totalinst;

    /* compute_job_node_XXX compute the metrics only for the calling job. It is
     * needed to iterate over the jobs to have per node. */
    debug("Node signature GBS %lf TPI (old %lf) new %lf cas counters %llu linst %llu", lsig->GBS, oldTPI, lsig->TPI,
          data->cas_counters, lsig->instructions);

    for (uint j = 0; j < node_mgr_earl_apps(); j++) {
        /* DC_power condition means the signature is ready.
         * estimate_node_metrics is used to not give neither power nor GB/s.
         * It is usually used for the master application in a master/worker workflow
         * use case. */
        if (nmgr[j].creation_time && nmgr[j].libsh != NULL && nmgr[j].shsig != NULL &&
            nmgr[j].libsh->node_signature.DC_power && nmgr[j].libsh->estimate_node_metrics) {

            ull avx = 0;
            used_cpus += nmgr[j].libsh->num_cpus;

            if (j == node_mgr_index)
                my_used_cpus += nmgr[j].libsh->num_cpus;

            for (uint lp = 0; lp < nmgr[j].libsh->num_processes; lp++) {

                if (nmgr[j].shsig[lp].sig.instructions > 0) {

                    L3[node_process]   = nmgr[j].shsig[lp].sig.L3_misses;
                    inst[node_process] = nmgr[j].shsig[lp].sig.instructions;

                    if (nmgr[j].shsig[lp].sig.CPI)
                        ipc[node_process] = 1 / nmgr[j].shsig[lp].sig.CPI;
                    else
                        ipc[node_process] = 1;

                    avx = nmgr[j].shsig[lp].sig.FLOPS[INDEX_256F] / WEIGHT_256F +
                          nmgr[j].shsig[lp].sig.FLOPS[INDEX_256D] / WEIGHT_256D +
                          nmgr[j].shsig[lp].sig.FLOPS[INDEX_512F] / WEIGHT_512F +
                          nmgr[j].shsig[lp].sig.FLOPS[INDEX_512D] / WEIGHT_512D;

                    if (nmgr[j].shsig[lp].sig.instructions) {
                        vpi[node_process] = (double) avx / (double) nmgr[j].shsig[lp].sig.instructions;
                    } else
                        vpi[node_process] = 0;

                    f[node_process] = nmgr[j].shsig[lp].sig.avg_f;

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

    for (uint j = 0; j < node_process; j++) {
        /* GBs is per-node, we can use our own GBs */
        if (node_L3) {
            float mem_ratio = (float) L3[j] / (float) node_L3;
            gbs[j]          = mem_ratio * lsig->GBS;
            dram_power[j]   = mem_ratio * lsig->DRAM_power;
            /* TPI should be total accesses / total instructions, we are computing in this
             * way for compatibility */
            if (inst[j]) {
                tpi[j] = (mem_ratio * data->cas_counters * cache_line_size) / inst[j];
                debug("case 1[%d]: Ratio %.3f cas %llu inst %llu", j, mem_ratio, data->cas_counters, inst[j]);
            } else {
                tpi[j] = (double) 0;
            }
            verbose_info2_master("L3 misses ratio %f | GB/s %lf | DRAM power %lf | TPI %lf", mem_ratio, gbs[j],
                                 dram_power[j], tpi[j]);
        } else {
            /* If there is no cache misses, we distribute the GBs proportionally to the number of cpus, naive approach
             */
            cj              = job_in_process[j];
            float mem_ratio = 0;
            if (used_cpus && nmgr[cj].libsh->num_processes) {
                mem_ratio =
                    ((float) nmgr[cj].libsh->num_cpus / (float) used_cpus) / (float) nmgr[cj].libsh->num_processes;
                gbs[j] = lsig->GBS * mem_ratio;
                tpi[j] = lsig->TPI * mem_ratio;
                verbose_info2_master("L3 misses ratio (based on CPU ratio) %f | GB/s %lf | TPI %lf", mem_ratio, gbs[j],
                                     tpi[j]);
                debug("case 2[%d]:Using lGBS %.0lf lTPI %.0lf mem ratio %lf", j, lsig->GBS, lsig->TPI, mem_ratio);
            } else {
                gbs[j] = lsig->GBS;
                tpi[j] = lsig->TPI;
                debug("case 3[%d]:Using lGBS %.0lf lTPI %.0lf ", j, lsig->GBS, lsig->TPI);
            }
            if (lsig->GBS)
                dram_power[j] = (gbs[j] / lsig->GBS) * lsig->DRAM_power;
            else
                dram_power[j] = lsig->DRAM_power;
            debug("case 2[%d]: Ratio %.3f Node TPI %.2lf", j, mem_ratio, lsig->TPI);
        }

        // DRAM power and memory bandwidth is proportional to the number of processes.
        if ((L3[j] || gbs[j]) && (job_in_process[j] == node_mgr_index)) {
            my_job_GBS += gbs[j];
            my_job_DRAM_power += dram_power[j];
            my_job_TPI += tpi[j];
            sig[lp].sig.GBS        = gbs[j];
            sig[lp].sig.DRAM_power = dram_power[j];
            sig[lp].sig.TPI        = tpi[j];
            debug("Process %d/%d GBS %.2lf TPI %.2lf", job_in_process[j], lp, sig[lp].sig.GBS, sig[lp].sig.TPI);
            lp++;
        }
    }

    data->job_signature.GBS        = my_job_GBS;
    data->job_signature.TPI        = my_job_TPI;
    data->job_signature.DRAM_power = my_job_DRAM_power;

    debug("My job[%d] GB/s is %.2lf TPI %.2lf DRAM %lf total inst %llu", node_mgr_index, data->job_signature.GBS,
          data->job_signature.TPI, data->job_signature.DRAM_power, totalinst);

    /* Power : Power is estimated based on CPU activity and Memory activity.
     * Less CPI means more CPU activity and more
     * L3 means more Mem actitivy */

    /* Per job in node loop */
    double total_power_estimated = 0, CPU_power, GPU_power = 0;
    lp = 0;
    for (uint j = 0; j < node_mgr_earl_apps(); j++) {
        /* If job is active we compute its power estimated */
        if (nmgr[j].creation_time && nmgr[j].libsh != NULL && nmgr[j].shsig != NULL && f[lp] &&
            nmgr[j].libsh->estimate_node_metrics) {
            for (uint proc = 0; proc < nmgr[j].libsh->num_processes; proc++) {
                double P = (ipc[lp] * my_node_coeffs.ipc) + (gbs[lp] * my_node_coeffs.gbs) +
                           (vpi[lp] * my_node_coeffs.vpi) + (f[lp] * my_node_coeffs.f) + my_node_coeffs.inter;

                power_estimations[lp] = P * nmgr[j].shsig[proc].num_cpus / cpus_in_node;

                debug("Power estimated for job %u/%u --> %.2lf/with %u cpus = %.3lf (ipc %.3lf,vpi %.3lf, gbs %.3lf f "
                      "%.2lf",
                      j, proc, P, nmgr[j].shsig[proc].num_cpus, power_estimations[lp], ipc[lp], vpi[lp], gbs[lp],
                      (double) f[lp] / 1000000.0);

                total_power_estimated += power_estimations[lp];

                lp++;
            }
        }
    }

    CPU_power = lsig->DC_power;
#if USE_GPUS
    for (uint gid = 0; gid < lsig->gpu_sig.num_gpus; gid++) {
        GPU_power += lsig->gpu_sig.gpu_data[gid].GPU_power;
    }
#endif
#if EARL_LIGHT
    if (CPU_power == 0)
        CPU_power = lsig->PCK_power + lsig->DRAM_power + GPU_power;
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

    debug("Baseline CPU power %lf", CPU_power);

    job_power_ratio          = 0.0;
    cpu_power_ratio          = (double) my_used_cpus / (double) cpu_count;
    double GPU_process_power = 0;
    lp                       = 0;
    for (uint proc = 0; proc < node_process; proc++) {
        GPU_process_power = 0;
        if ((job_in_process[proc] == node_mgr_index) && (power_estimations[proc] > 0) && (total_power_estimated > 0)) {
            job_power_ratio += power_estimations[proc] / total_power_estimated;
            process_power_ratio[proc] = power_estimations[proc] / total_power_estimated;
            if (GPU_power)
                GPU_process_power = GPU_power / data->num_processes;
            sig[lp].sig.DC_power =
                (CPU_power * process_power_ratio[proc]) + GPU_process_power; // what about GPU power in processes
            sig[lp].sig.PCK_power = process_power_ratio[proc] * lsig->PCK_power;
            debug("Power process %d/%d is %lf. PCK power %lf", job_in_process[proc], lp, sig[lp].sig.DC_power,
                  sig[lp].sig.PCK_power);
            lp++;
        }
    }

    /* Extra power should be equally distributed: PENDING */
    data->job_signature.DC_power  = (CPU_power * job_power_ratio) + GPU_power;
    data->job_signature.PCK_power = job_power_ratio * lsig->PCK_power;

    data->job_signature.DC_job_power  = (CPU_power * cpu_power_ratio) + GPU_power;
    data->job_signature.PCK_job_power = cpu_power_ratio * lsig->PCK_power;
    verbose_info2_master("Job %u | DC node power (node ratio) %lf | CPU pwr %lf GPU pwr %lf ratio %lf | PCK pwr %lf",
                         node_mgr_index, data->job_signature.DC_power, CPU_power, GPU_power, job_power_ratio,
                         data->job_signature.PCK_power);
    verbose_info2_master("Job %u | DC node power (job ratio) %lf | CPU pwr %lf GPU pwr %lf ratio %lf | PCK pwr %lf",
                         node_mgr_index, data->job_signature.DC_job_power, CPU_power, GPU_power, cpu_power_ratio,
                         data->job_signature.PCK_job_power);

    if (node_sharing_strategy == job) {
        data->job_signature.DC_power  = data->job_signature.DC_job_power;
        data->job_signature.PCK_power = data->job_signature.PCK_job_power;
    }

    return EAR_SUCCESS;
}
