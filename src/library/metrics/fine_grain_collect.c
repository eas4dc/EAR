/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <library/metrics/fine_grain_collect.h>

#include <common/types.h>
#include <common/states.h>
#include <common/system/monitor.h>

#include <library/api/mpi_support.h>
#include <library/loader/module_mpi.h>
#include <library/common/utils.h>
#include <library/common/verbose_lib.h>
#include <library/metrics/metrics.h>
#include <library/policies/policy.h>
#include <library/policies/common/mpi_stats_support.h>
#include <library/tracer/tracer.h>


#if MPI_OPTIMIZED
uint ear_mpi_opt = 0;
uint ear_mpi_opt_num_nodes = 1;

mpi_opt_policy_t mpi_opt_symbols;
suscription_t *earl_mpi_opt;

/* 0 = init, 1 = end, 2= diff */
static mpi_information_t my_mpi_stats[3];
static mpi_calls_types_t my_mpi_types[3];

extern ulong local_max_sync_block;


static state_t earl_periodic_actions_mpi_opt_init(void *no_arg);


static state_t earl_periodic_actions_mpi_opt(void *no_arg);
#endif


state_t fine_grain_metrics_init()
{
#if MPI_OPTIMIZED
  ear_mpi_opt_num_nodes = utils_get_sched_num_nodes();

	if (module_mpi_is_enabled() && ear_mpi_opt)
	{
		earl_mpi_opt = suscription();
		earl_mpi_opt->call_init = earl_periodic_actions_mpi_opt_init;
		earl_mpi_opt->call_main = earl_periodic_actions_mpi_opt;
		earl_mpi_opt->time_relax = 200;
		earl_mpi_opt->time_burst = 200;

		if (state_fail(monitor_register(earl_mpi_opt)))
		{
			verbose_master(2, "Monitor for MPI opt register fails! %s", state_msg);
		}

		monitor_relax(earl_mpi_opt);
		verbose_master(2, "Monitor for MPI opt created");
	}
#endif
	return EAR_SUCCESS;
}


#if MPI_OPTIMIZED
static state_t earl_periodic_actions_mpi_opt_init(void *no_arg)
{
	if (module_mpi_is_enabled() && ear_mpi_opt)
	{
		memset(my_mpi_stats, 0, sizeof(mpi_information_t)*3);
		memset(my_mpi_types, 0, sizeof(mpi_calls_types_t)*3);
		metrics_start_cpupower();
	}
	return EAR_SUCCESS;
}
#endif


#if MPI_OPTIMIZED
static state_t earl_periodic_actions_mpi_opt(void *no_arg)
{
	if (module_mpi_is_enabled() && (ear_mpi_opt || traces_are_on()))
	{
		/* Read */
		mpi_call_get_stats(&my_mpi_stats[1], NULL, -1);
		mpi_call_get_types(&my_mpi_types[1], NULL, -1);
		/* Diff */
		mpi_call_diff(     &my_mpi_stats[2], &my_mpi_stats[1], &my_mpi_stats[0]);

		my_mpi_stats[2].perc_mpi = (double) my_mpi_stats[2].mpi_time * 100.0 / (double) my_mpi_stats[2].exec_time;

		mpi_call_types_diff(&my_mpi_types[2], &my_mpi_types[1], &my_mpi_types[0]);

		/* Copy */
		memcpy(&my_mpi_stats[0], &my_mpi_stats[1], sizeof(mpi_information_t));
		memcpy(&my_mpi_types[0], &my_mpi_types[1], sizeof(mpi_calls_types_t));

		metrics_read_cpupower();

		uint new_pstate;
		if (mpi_opt_symbols.periodic != NULL)  mpi_opt_symbols.periodic(&new_pstate);

		/* We change the CPU freq after processing, is that a problem, warning !*/
		if (new_pstate) policy_restore_to_mpi_pstate(new_pstate);

		if (traces_are_on())
		{
			traces_generic_event(ear_my_rank, my_node_id, PERC_MPI,				  (ullong) my_mpi_stats[2].perc_mpi * 100);
			traces_generic_event(ear_my_rank, my_node_id, MPI_CALLS,			  (ullong) my_mpi_types[2].mpi_call_cnt);
			traces_generic_event(ear_my_rank, my_node_id, MPI_SYNC_CALLS,   (ullong) my_mpi_types[2].mpi_sync_call_cnt);
			traces_generic_event(ear_my_rank, my_node_id, MPI_BLOCK_CALLS,  (ullong) my_mpi_types[2].mpi_block_call_cnt);
			traces_generic_event(ear_my_rank, my_node_id, TIME_SYNC_CALLS,  (ullong) my_mpi_types[2].mpi_sync_call_time);
			traces_generic_event(ear_my_rank, my_node_id, TIME_BLOCK_CALLS, (ullong) my_mpi_types[2].mpi_block_call_time);
			traces_generic_event(ear_my_rank, my_node_id, TIME_MAX_BLOCK,		(ullong) local_max_sync_block);
			local_max_sync_block = 0;
		}
	}
	return EAR_SUCCESS;
}
#endif // MPI_OPTIMIZED
