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

//#define SHOW_DEBUGS 1
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <common/hardware/architecture.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/shared_configuration.h>
#include <library/common/library_shared_data.h>

#include <metrics/flops/flops.h>

#include <library/models/cpu_power_model_default.h>

static intel_skl_t my_node_coeffs;
static uint arch_model_loaded = 0;
static uint cpu_count;
static uint cache_line_size;

state_t cpu_power_model_init_arch(settings_conf_t *libconf, conf_install_t *conf, architecture_t *arch)
{
	int fd, ret;
	debug("cpu_power_models_init_arch. Using tag %s etc %s", libconf->tag, conf->dir_conf);
	char model_path[MAX_PATH_SIZE];
	xsnprintf(model_path, sizeof(model_path), "%s/ear/coeffs/island%d/coeffs.cpumodel.%s", conf->dir_conf,libconf->island, libconf->tag);
	debug("cpu_power_models_init_arch coeffs path %s", model_path);

	#if 0
	my_node_coeffs.ipc 		= IPC_COEFF;
	my_node_coeffs.gbs 		=	GBS_COEFF;
	my_node_coeffs.vpi 		=	VPI_COEFF;
	my_node_coeffs.f   		=	F_COEFF;
	my_node_coeffs.inter 	=	INTERCEPT;
	#endif
	fd = open(model_path, O_RDONLY);
	if (fd < 0){ 
		debug("Error opening cpu model file %s", strerror(errno));
		return EAR_ERROR;
	}
	if ((ret = read(fd, &my_node_coeffs, sizeof(intel_skl_t))) != sizeof(intel_skl_t)){
		if (ret < 0){ 	debug("Error reading cpu model data %s", strerror(errno));}
		else{ 					debug("Error reading cpu model data %d bytes expected %lu", ret, sizeof(intel_skl_t));}
		close(fd);
		return EAR_ERROR;
	}
	close(fd);
	cpu_count = arch->top.cpu_count;

	arch_model_loaded = 1;	
  cache_line_size = arch->top.cache_line_size;
	
	return EAR_SUCCESS;
}

state_t cpu_power_model_status_arch()
{
	debug("cpu_power_model_status_arch");
	if (!arch_model_loaded) return EAR_ERROR;
	return EAR_SUCCESS;
}

state_t cpu_power_model_project_arch(lib_shared_data_t *data, shsignature_t *sig,
        node_mgr_sh_data_t *nmgr, uint node_mgr_index)
{
    debug("cpu_power_model_project_arch");

    if (!arch_model_loaded) return EAR_ERROR;

	if (data == NULL) return EAR_SUCCESS;

    ull  node_L3 = 0;

    int cpus_in_node = cpu_count;	

	double power_estimations[MAX_CPUS_SUPPORTED];
	double dram_power[MAX_CPUS_SUPPORTED];
	double pck_power[MAX_CPUS_SUPPORTED];
	double ipc[MAX_CPUS_SUPPORTED];
	double tpi[MAX_CPUS_SUPPORTED];
	double gbs[MAX_CPUS_SUPPORTED];
	double vpi[MAX_CPUS_SUPPORTED];

	ulong  f[MAX_CPUS_SUPPORTED];
  ullong inst[MAX_CPUS_SUPPORTED];
	ull    L3[MAX_CPUS_SUPPORTED];
	uint   job_in_process[MAX_CPUS_SUPPORTED];

	double job_power_ratio, process_power_ratio[MAX_CPUS_SUPPORTED];;

	uint   node_process = 0;

	signature_t *lsig;

	double my_job_GBS, my_job_DRAM_power, my_job_TPI;

	uint used_cpus = 0;

	lsig = &data->node_signature;

	memset(ipc,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(process_power_ratio,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(tpi,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(vpi,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(gbs,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(f,0,sizeof(ulong)*MAX_CPUS_SUPPORTED);
	memset(L3,0,sizeof(ull)*MAX_CPUS_SUPPORTED);
	memset(power_estimations,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(dram_power,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(pck_power,0,sizeof(double)*MAX_CPUS_SUPPORTED);
  memset(inst, 0, sizeof(ullong)*MAX_CPUS_SUPPORTED);

  verbose(2, "CPU power model default ...............");

	if ((lsig == NULL) || (sig == NULL) || (nmgr == NULL)) return EAR_SUCCESS;
	
	/* compute_job_node_XXX compute the metrics only for the calling job. It is needed to iterate over the jobs to have per node */
	
    for (uint j = 0; j < MAX_CPUS_SUPPORTED; j ++) {
        /* Power condition is to do only in case the signature is ready */
        if (nmgr[j].creation_time
                && nmgr[j].libsh != NULL
                && nmgr[j].shsig != NULL
                && nmgr[j].libsh->node_signature.DC_power) {

            ull avx = 0;

            used_cpus += nmgr[j].libsh->num_cpus;

            for (uint lp = 0; lp < nmgr[j].libsh->num_processes; lp++) {
                if (nmgr[j].shsig[lp].sig.DC_power > 0) {

                    job_in_process[node_process] = j;

                    L3[node_process]             = nmgr[j].shsig[lp].sig.L3_misses;
                    inst[node_process]           = nmgr[j].shsig[lp].sig.instructions;

                    ipc[node_process]            = 1 / nmgr[j].shsig[lp].sig.CPI;

                    avx = nmgr[j].shsig[lp].sig.FLOPS[INDEX_256F]/WEIGHT_256F
                        + nmgr[j].shsig[lp].sig.FLOPS[INDEX_256D]/WEIGHT_256D
                        + nmgr[j].shsig[lp].sig.FLOPS[INDEX_512F]/WEIGHT_512F
                        + nmgr[j].shsig[lp].sig.FLOPS[INDEX_512D]/WEIGHT_512D;

                    vpi[node_process] = (double) avx / (double) nmgr[j].shsig[lp].sig.instructions;

                    f[node_process]   = nmgr[j].shsig[lp].sig.avg_f;

                    node_L3          += L3[node_process];
                }

                node_process++;
            }
        }
    }

	uint lp           = 0;
	uint cj           = 0;
	my_job_GBS        = 0;
	my_job_TPI        = 0;
	my_job_DRAM_power = 0;

    for (uint j = 0; j < node_process; j ++) {
        /* GBs is per-node, we can use our own GBs */
        if (node_L3) {
            float mem_ratio = (float)L3[j]/(float)node_L3;
            gbs[j]        = mem_ratio * lsig->GBS;
            dram_power[j] = mem_ratio * lsig->DRAM_power;
            tpi[j]        = (mem_ratio * data->cas_counters * cache_line_size) / inst[j];
            debug("case 1[%d]: Ratio %.3f cas %llu inst %llu", j, mem_ratio, data->cas_counters, inst[j]);
        } else {
            /* If there is no cache misses, we distribute the GBs proportionally to the number of cpus, naive approach */
            cj = job_in_process[j];
            float mem_ratio = ((float)nmgr[cj].libsh->num_cpus/(float)used_cpus)/(float)nmgr[cj].libsh->num_processes;
            gbs[j] = lsig->GBS * mem_ratio;
            tpi[j] = lsig->TPI * mem_ratio;
            dram_power[j] = (gbs[j]/lsig->GBS) * lsig->DRAM_power;
            debug("case 2[%d]: Ratio %.3f Node TPI %.2lf", j, mem_ratio, lsig->TPI);
        }
        if ((L3[j] || gbs[j])  && (job_in_process[j] == node_mgr_index)){
            my_job_GBS        += gbs[j];
            my_job_DRAM_power += dram_power[j];
            my_job_TPI        += tpi[j];
            sig[lp].sig.GBS = gbs[j];	
            sig[lp].sig.DRAM_power = dram_power[j];	
            sig[lp].sig.TPI = tpi[j];
            debug("Process %d/%d GBS %.2lf TPI %.2lf", job_in_process[j], lp, sig[lp].sig.GBS, sig[lp].sig.TPI);
            lp++;
        }
    }
	data->job_signature.GBS = my_job_GBS;
	data->job_signature.TPI = my_job_TPI;
	data->job_signature.DRAM_power = my_job_DRAM_power;


	verbose(2, "My job[%d] GB/s is %.2lf TPI %.2lf DRAM %lf", node_mgr_index, data->job_signature.GBS, data->job_signature.TPI, data->job_signature.DRAM_power);


	/* Power : Power is estimated based on CPU activity and Memory activity. Less CPI means more CPU activity and more L3 means more Mem actitivy */

	/* Per job in node loop */
	double total_power_estimated = 0, CPU_power, GPU_power = 0;
	lp = 0;
	for (uint j = 0; j < MAX_CPUS_SUPPORTED; j ++){
		/* If job is active we compute its power estimated */
		if (nmgr[j].creation_time && (nmgr[j].libsh != NULL) && (nmgr[j].shsig != NULL) && f[lp]){
			for (uint proc = 0;proc < nmgr[j].libsh->num_processes; proc++){
				double P;
				P = (ipc[lp] * my_node_coeffs.ipc) + (gbs[lp] * my_node_coeffs.gbs) + (vpi[lp] * my_node_coeffs.vpi) + (f[lp] * my_node_coeffs.f ) + my_node_coeffs.inter;
				power_estimations[lp] = P*nmgr[j].shsig[proc].num_cpus/cpus_in_node;
				debug("Power estimated for job %u/%u --> %.2lf/with %u cpus = %.3lf (ipc %.3lf,vpi %.3lf, gbs %.3lf f %.2lf",j,proc, P,nmgr[j].shsig[proc].num_cpus, power_estimations[lp],ipc[lp],vpi[lp],gbs[lp],(double)f[lp]/1000000.0);
				total_power_estimated += power_estimations[lp];
			
				lp++;
			}
		}
	}

  CPU_power = lsig->DC_power;
  #if USE_GPUS
  for (uint gid = 0; gid < lsig->gpu_sig.num_gpus; gid++){
    GPU_power +=  lsig->gpu_sig.gpu_data[gid].GPU_power;
  }
  #endif
  CPU_power -= GPU_power;

	debug("Baseline CPU power %lf", CPU_power);

	job_power_ratio = 0.0;
	double GPU_process_power = 0;
	lp = 0;
	for (uint proc =0; proc < node_process; proc++){
	  if ((job_in_process[proc] == node_mgr_index) && (power_estimations[proc] > 0)){ 
		  job_power_ratio += power_estimations[proc]/total_power_estimated;
		  process_power_ratio[proc] = power_estimations[proc]/total_power_estimated;
			if (GPU_power) GPU_process_power = GPU_power / data->num_processes;
			sig[lp].sig.DC_power = (CPU_power * process_power_ratio[proc]) + GPU_process_power; // what about GPU power in processes	
			sig[lp].sig.PCK_power = process_power_ratio[proc] * lsig->PCK_power;
			debug("Power process %d/%d is %lf. PCK power %lf", job_in_process[proc], lp, sig[lp].sig.DC_power, sig[lp].sig.PCK_power);
			lp++;
		}
	}

	/* Extra power should be equally distributed: PENDING */
	data->job_signature.DC_power = (CPU_power	* job_power_ratio) + GPU_power;
	data->job_signature.PCK_power = job_power_ratio * lsig->PCK_power;
	debug("My job[%d] power is %lf (node (CPU %lf + GPU %lf) x ratio %lf) PCK %lf", node_mgr_index, data->job_signature.DC_power, CPU_power, GPU_power, job_power_ratio, data->job_signature.PCK_power);

	return EAR_SUCCESS;
}
