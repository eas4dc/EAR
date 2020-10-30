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

/*
        timestamp:1:            Period id
        timestamp:2:            Period length
        timestamp:3:            Period iterations
        timestamp:4:            Period time
        timestamp:5:            Period CPI
        timestamp:6:            Period TPI
        timestamp:7:            Period GBs
        timestamp:8:            Period power
        timestamp:9:            Period time projection
        timestamp:10:           Period CPI projection
        timestamp:11:           Period power projection
        timestamp:12:           Frequency
*/

#include <common/config.h>
#include <common/types/generic.h>
#include <common/types/signature.h>

#define MIN_FREQ_FOR_SAMPLING 500000

#ifdef EAR_GUI
	/** Executed at application start */
 	void traces_init(char *app,int global_rank, int local_rank, int nodes, int mpis, int ppn);
	/** Executed at application end */
	void traces_end(int global_rank,int local_rank, unsigned long int total_ener);
	/** **/
	void traces_start();
	/** **/
	void traces_stop();

	/**@{ Executed when application signature is computed at EVALUATING_SIGNATURE and SIGANTURE_STABLE states */
	void traces_frequency(int global_rank, int local_rank, unsigned long f);
	void traces_new_signature(int global_rank, int local_rank, signature_t *sig);
	void traces_PP(int global_rank, int local_rank, double seconds, double power); /**@}*/ 

	/**@{ Executed when each time a new loop is detected, the loop ends, or a new iteration are reported */
	void traces_new_n_iter(int global_rank, int local_rank, ullong period_id, int loop_size, int iterations);
	void traces_new_period(int global_rank, int local_rank, ullong period_id);
	void traces_end_period(int global_rank, int local_rank); /**@}*/ 

	/** EARL internal state */
	void traces_policy_state(int global_rank, int local_rank, int state);
	void traces_dynais(int global_rank, int local_rank, int state);
	void traces_earl_mode_dynais(int global_rank, int local_rank);
	void traces_earl_mode_periodic(int global_rank, int local_rank);

	/** External reconfiguration is requested */
	void traces_reconfiguration(int global_rank, int local_rank);
	
	/** returns true if traces are dynamically activated , is independent on start/stop*/
	int traces_are_on();
#else
	#define traces_init(a,g,l,n,m,p)
	#define traces_new_n_iter(g,l,p,lo,i)
	#define traces_end(g,l,e)
	#define traces_new_signature(g,l,s,c,t,gb,p,vpi)
	#define traces_frequency(g,l,f)
	#define traces_PP(g,l,s,p)
	#define traces_new_period(g,l,p)
	#define traces_end_period(g,l)
	#define traces_policy_state(g,l,s);
	#define traces_dynais(g,l,s);
	#define traces_earl_mode_dynais(g,l);
	#define traces_earl_mode_periodic(g,l);
	#define traces_reconfiguration(g,l);
	#define traces_start()
	#define traces_stop()
	#define traces_are_on() 	0
#endif

#endif
