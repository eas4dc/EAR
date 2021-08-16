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
#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <library/common/verbose_lib.h>
#include <library/common/externs.h>
#include <library/dynais/dynais.h>
#include <library/api/clasify.h>
#include <library/states/states_comm.h>



state_t classify(uint iobound,uint mpibound, uint *app_state)
{
	*app_state = APP_COMP_BOUND;
	if (iobound && !mpibound){
		*app_state = APP_IO_BOUND;
	}else if (mpibound){
		*app_state = APP_MPI_BOUND;
	}
	return EAR_SUCCESS;
}

state_t is_io_bound(float rwsec,uint *iobound)
{
    *iobound = 0;
    if (rwsec > IO_TH) *iobound = 1;
    return EAR_SUCCESS;

}

state_t is_network_bound(float mpisec,uint *mpibound)
{
	*mpibound = 0;
	if (mpisec > MPI_TH){
		*mpibound = 1;
	}
	return EAR_SUCCESS;
}

state_t must_switch_to_time_guide(float mpisec, uint *ear_guided)
{
    if (mpisec < MIN_MPI_TH){
        *ear_guided = TIME_GUIDED;
    }
    return EAR_SUCCESS;
}

state_t is_cpu_busy_waiting(signature_t *sig,uint *busy)
{
	double flops, cpi;
	ullong inst, cycles;
	uint c1, c2, c3;
	*busy = USEFUL_COMP;
	//for (i=0;i<FLOPS_EVENTS;i++) flop+=sig->FLOPS[i];
	flops = compute_node_flops(lib_shared_region,sig_shared_region);
	compute_total_node_instructions(lib_shared_region, sig_shared_region, &inst);
	compute_total_node_cycles(lib_shared_region, sig_shared_region, &cycles);
	//compute_total_node_CPI(lib_shared_region,sig_shared_region,&cpi); 
	cpi = (double)cycles/(double)inst;
	c1 = (cpi      < CPI_BUSY_WAITING);
	c2 = (sig->GBS < GBS_BUSY_WAITING);
	c3 = (flops    < GFLOPS_BUSY_WAITING);
	if (c1 || c2 || c3){
		verbose_master(2,"CPI (%.3lf/%.3lf/%.3lf) GBS(%.3lf/%.3lf) FLOPS (%.3lf/%.3lf)", sig->CPI,cpi,CPI_BUSY_WAITING,sig->GBS,GBS_BUSY_WAITING,flops,GFLOPS_BUSY_WAITING);
	}
	if (c1 && c2 && c3) *busy = BUSY_WAITING;
	return EAR_SUCCESS;
}

state_t is_cpu_bound(signature_t *sig,uint *cbound)
{
    *cbound = 0;
    if ((sig->CPI < CPI_CPU_BOUND) && (sig->GBS < GBS_CPU_BOUND)) *cbound = 1;
    return EAR_SUCCESS;
}

state_t is_mem_bound(signature_t *sig,uint *mbound)
{
    *mbound = 0;
    if ((sig->CPI > CPI_MEM_BOUND) && (sig->GBS > GBS_MEM_BOUND)) *mbound = 1;
    return EAR_SUCCESS;
}

state_t  is_gpu_idle(signature_t *sig,uint *gidle)
{
#if USE_GPUS
	int total = 0;
	int i;
	if (sig->gpu_sig.num_gpus == 0){
		*gidle = GPU_IDLE;
		return EAR_SUCCESS;
	}
	for (i=0;i< sig->gpu_sig.num_gpus;i++) total += sig->gpu_sig.gpu_data[i].GPU_util;
	if (total == 0) *gidle = GPU_IDLE;
	else *gidle = GPU_COMP;
	return EAR_SUCCESS;
#else
	*gidle = GPU_IDLE;
	return EAR_SUCCESS;
#endif
}
