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

static uint arch_model_loaded = 0;
static uint cpu_count;

state_t cpu_power_model_init_arch(settings_conf_t *libconf, conf_install_t *conf, architecture_t *arch)
{
	cpu_count = arch->top.cpu_count;
	arch_model_loaded = 1;	
	
	return EAR_SUCCESS;
}

state_t cpu_power_model_status_arch()
{
	debug("cpu_power_model_status_arch");
	if (!arch_model_loaded) return EAR_ERROR;
	return EAR_SUCCESS;
}

state_t cpu_power_model_project_arch(lib_shared_data_t *data,shsignature_t *sig, node_mgr_sh_data_t *nmgr, uint node_mgr_index)
{
	debug("cpu_power_model_project_arch");
	if (!arch_model_loaded) return EAR_ERROR;

	ull  node_L3 = 0;
	double power_estimations[MAX_CPUS_SUPPORTED];
	double dram_power[MAX_CPUS_SUPPORTED];
	double pck_power[MAX_CPUS_SUPPORTED];
	double gbs[MAX_CPUS_SUPPORTED];
	double tpi[MAX_CPUS_SUPPORTED];
  ullong inst[MAX_CPUS_SUPPORTED];
	ulong  f[MAX_CPUS_SUPPORTED];
	ull    L3[MAX_CPUS_SUPPORTED];
	uint   job_in_process[MAX_CPUS_SUPPORTED];
	double job_power_ratio, process_power_ratio[MAX_CPUS_SUPPORTED];;
	uint   node_process = 0;
	signature_t *lsig = &data->node_signature;
	double my_job_GBS, my_job_DRAM_power, my_job_TPI;
	uint used_cpus = 0;




	memset(process_power_ratio,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(gbs, 0, sizeof(double)*MAX_CPUS_SUPPORTED);
  memset(tpi, 0, sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(f,0,sizeof(ulong)*MAX_CPUS_SUPPORTED);
	memset(L3,0,sizeof(ull)*MAX_CPUS_SUPPORTED);
	memset(power_estimations,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(dram_power,0,sizeof(double)*MAX_CPUS_SUPPORTED);
	memset(pck_power,0,sizeof(double)*MAX_CPUS_SUPPORTED);
  memset(inst, 0, sizeof(ullong)*MAX_CPUS_SUPPORTED);

  verbose(2, "CPU power model dummy ...............");


	if ((lsig == NULL) || (data == NULL) || (sig == NULL) || (nmgr == NULL)){ 
		debug("Warning, cpupower model cannot be applied");
		return EAR_SUCCESS;
	}

	
	/* compute_total_node_fff compute the metrics only for the calling job. It is needed to iterate over the jobs to have per node */
	
	for (uint j = 0; j < MAX_CPUS_SUPPORTED; j ++){
		/* Power condition is to do only in case the signature is ready */
		if (nmgr[j].creation_time && (nmgr[j].libsh != NULL) && (nmgr[j].shsig != NULL) && (nmgr[j].libsh->node_signature.DC_power)){  
			used_cpus += nmgr[j].libsh->num_cpus;
			for (uint lp = 0; lp < nmgr[j].libsh->num_processes; lp++){
				if (nmgr[j].shsig[lp].sig.DC_power > 0){
				  job_in_process[node_process] = j;
				  L3[node_process]  = nmgr[j].shsig[lp].sig.L3_misses;
				  node_L3 += L3[node_process];
				}
				node_process++;
			}

		}
	}
	#if EARL_LIGHT
	debug("There are %d jobs in node ", node_process);
	#endif

  if (used_cpus == 0) return EAR_SUCCESS;


	uint lp = 0;
	int cj = 0;
	my_job_GBS = 0;
  my_job_TPI = 0;
	my_job_DRAM_power = 0;
	for (uint j = 0; j < node_process; j ++){
    /* GBs is per-node, we can use our own GBs */
    if (node_L3){
      float mem_ratio = (float)L3[j]/(float)node_L3;
      gbs[j] = mem_ratio * lsig->GBS;
      tpi[j] = (double)(mem_ratio * (double)data->cas_counters) / (double)inst[j];
      dram_power[j] = mem_ratio * lsig->DRAM_power;
    }else{
      /* If there is no cache misses, we distribute the GBs proportionally to the number of cpus, naive approach */
      cj = job_in_process[j];
      if (nmgr[cj].libsh->num_processes == 0){
        gbs[j] = 0;
        tpi[j] = 0;
        dram_power[j] = 0;
      }else{

        float mem_ratio = ((float)nmgr[cj].libsh->num_cpus/(float)used_cpus)/(float)nmgr[cj].libsh->num_processes;
        gbs[j] = lsig->GBS * mem_ratio;
        tpi[j] = lsig->TPI * mem_ratio;
        if (lsig->GBS) dram_power[j] = (gbs[j]/lsig->GBS) * lsig->DRAM_power;
        else           dram_power[j] = lsig->DRAM_power/(float)nmgr[cj].libsh->num_processes;
      }

    }
    if ((L3[j] || gbs[j])  && (job_in_process[j] == node_mgr_index)){

				my_job_GBS        += gbs[j];
        my_job_TPI        += tpi[j];
				my_job_DRAM_power += dram_power[j];
				sig[lp].sig.GBS = gbs[j];	
				sig[lp].sig.TPI = tpi[j];	
				sig[lp].sig.DRAM_power = dram_power[j];	
				debug("Process %d/%d GBS %.2lf TPI %.2lfL3 %llu", job_in_process[j], lp, sig[lp].sig.GBS, sig[lp].sig.TPI, L3[j]);
				lp++;
    }
  }
	data->job_signature.GBS = my_job_GBS;
	data->job_signature.DRAM_power = my_job_DRAM_power;
  data->job_signature.TPI = my_job_TPI;


	debug("My job[%d] GB/s is %.2lf, TPI %.2lf DRAM %lf", node_mgr_index, data->job_signature.GBS, data->job_signature.TPI, data->job_signature.DRAM_power);


	/* Power : Power is estimated based on CPU activity and Memory activity. Less CPI means more CPU activity and more L3 means more Mem actitivy */

	double total_power_estimated = 0, CPU_power, GPU_power = 0;
  CPU_power = lsig->DC_power;
  #if USE_GPUS
  for (uint gid = 0; gid < lsig->gpu_sig.num_gpus; gid++){
    GPU_power +=  lsig->gpu_sig.gpu_data[gid].GPU_power;
  }
  #endif
	#if EARL_LIGHT	
	if (CPU_power == 0) CPU_power = lsig->PCK_power + lsig->DRAM_power + GPU_power;
	#endif
  	CPU_power -= GPU_power;

	debug("Baseline CPU power %lf", CPU_power);
	double GPU_process_power = 0;

  if (data->num_processes == 0) return EAR_SUCCESS;

	if (node_process == 0){
		for (uint lp = 0; lp < data->num_processes; lp ++){
			sig[lp].sig.DC_power	= (CPU_power + GPU_power) / data->num_processes;
			sig[lp].sig.PCK_power   = lsig->PCK_power / data->num_processes;
			debug("process[%u] DC power %f PCK power %f", lp, sig[lp].sig.DC_power, sig[lp].sig.PCK_power);
		}
		data->job_signature.DC_power  = CPU_power + GPU_power;
		data->job_signature.PCK_power = CPU_power;
		return EAR_SUCCESS;
	}

  if (cpu_count == 0) return EAR_SUCCESS;

	/* Per job in node loop */
	lp = 0;
	total_power_estimated = 0;
	double P = (double)CPU_power / (double)cpu_count;
	for (uint j = 0; j < MAX_CPUS_SUPPORTED; j ++){
		/* If job is active we compute its power estimated */
		if (nmgr[j].creation_time && (nmgr[j].libsh != NULL) && (nmgr[j].shsig != NULL)){
			for (uint proc = 0;proc < nmgr[j].libsh->num_processes; proc++){
				power_estimations[lp] = P * nmgr[j].shsig[proc].num_cpus;
				debug("Power estimated for job %u/%u --> %.2lf/with %u cpus = %.3lf (gbs %.3lf)",j,proc, P,nmgr[j].shsig[proc].num_cpus, power_estimations[lp],gbs[lp]);
				total_power_estimated += power_estimations[lp];	
				lp++;
			}
		}
	}


	job_power_ratio = 0.0;
	lp = 0;
	for (uint proc =0; proc < node_process; proc++){
    GPU_process_power = 0;
	  if ((job_in_process[proc] == node_mgr_index) && (power_estimations[proc] > 0)){ 
		  job_power_ratio += power_estimations[proc]/total_power_estimated;
		  process_power_ratio[proc] = power_estimations[proc]/total_power_estimated;
		  debug("ID[%u] Ratio %f power estimation %f total %f process ratio %f", proc, job_power_ratio, power_estimations[proc], total_power_estimated, process_power_ratio[proc]);
			if (GPU_power) GPU_process_power = GPU_power / data->num_processes;
			sig[lp].sig.DC_power = (CPU_power * process_power_ratio[proc]) + GPU_process_power; // what about GPU power in processes	
			sig[lp].sig.PCK_power = process_power_ratio[proc] * lsig->PCK_power;
			debug("Power process %d/%d is %lf. PCK power %lf", job_in_process[proc], lp, sig[lp].sig.DC_power, sig[lp].sig.PCK_power);
			lp++;
		}
	}
	data->job_signature.DC_power = (CPU_power	* job_power_ratio) + GPU_power;
	data->job_signature.PCK_power = job_power_ratio * lsig->PCK_power;

	debug("My job[%d] power is %lf (node (CPU %lf + GPU %lf) x ratio %lf) PCK %lf", node_mgr_index, data->job_signature.DC_power, CPU_power, GPU_power, job_power_ratio, data->job_signature.PCK_power);

	return EAR_SUCCESS;
}



