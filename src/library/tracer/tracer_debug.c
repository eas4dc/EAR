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

#ifndef _EAR_TRACES_H_
#define _EAR_TRACES_H_

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <daemon/shared_configuration.h>
#include <common/types/signature.h>

	/** Executed at application start */
void traces_init(settings_conf_t *conf,char *app,int global_rank, int local_rank, int nodes, int mpis, int ppn)
{
	debug("traces_init_debug");
}
	/** Executed at application end */
void traces_end(int global_rank,int local_rank, unsigned long int total_ener)
{
	debug("traces_end_debug");
}
	/** **/
void traces_start()
{
}
	/** **/
void traces_stop()
{
}

	/**@{ Executed when application signature is computed at EVALUATING_SIGNATURE and SIGANTURE_STABLE states */
void traces_frequency(int global_rank, int local_rank, unsigned long f)
{
}
void traces_new_signature(int global_rank, int local_rank, signature_t *sig)
{
	double seconds, cpi, tpi, gbs, power, vpi;
	seconds=sig->time;
	cpi=sig->CPI;
	tpi=sig->TPI;
	gbs=sig->GBS;
	power=sig->DC_power;
	vpi=(double)(sig->FLOPS[3]/16+sig->FLOPS[7]/8)/(double)sig->instructions;
	debug("(%d,%d): traces_new_signature seconds %lf cpi %lf tpi %lf gbs %lf power %lf vpi %lf",global_rank,local_rank,seconds,cpi,tpi,gbs,power,vpi);
}
void traces_PP(int global_rank, int local_rank, double seconds, double power)
{
}

	/**@{ Executed when each time a new loop is detected, the loop ends, or a new iteration are reported */
void traces_new_n_iter(int global_rank, int local_rank, ullong period_id, int loop_size, int iterations)
{
}
void traces_new_period(int global_rank, int local_rank, ullong period_id)
{
}
void traces_end_period(int global_rank, int local_rank)
{
}

	/** EARL internal state */
void traces_policy_state(int global_rank, int local_rank, int state)
{
}
void traces_dynais(int global_rank, int local_rank, int state)
{
}
void traces_earl_mode_dynais(int global_rank, int local_rank)
{
}
void traces_earl_mode_periodic(int global_rank, int local_rank)
{
}

	/** External reconfiguration is requested */
void traces_reconfiguration(int global_rank, int local_rank)
{
}
	
	/** returns true if traces are dynamically activated , is independent on start/stop*/
int traces_are_on()
{
}
void traces_mpi_init()
{
}
void traces_mpi_call(int global_rank, int local_rank, ulong time, ulong ev, ulong a1, ulong a2, ulong a3)
{
}
void traces_mpi_end()
{
}
#endif
