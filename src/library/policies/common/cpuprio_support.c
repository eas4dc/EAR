/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#define _GNU_SOURCE


#include <library/policies/common/cpuprio_support.h>

#include <sched.h>

#include <common/config.h>
#include <common/types.h>
#include <common/types/generic.h>
#include <common/utils/string.h>

#include <management/cpufreq/priority.h>

#include <library/common/verbose_lib.h>
#include <library/common/externs.h>
#include <library/metrics/metrics.h>


#define DEF_USE_CPUPRIO 0

#define is_master (masters_info.my_master_rank >= 0)


static uint  use_cpuprio = DEF_USE_CPUPRIO; // Enables the usage of proiority system


/** Verboses a list of priorities. */
static void verbose_cpuprio_list(int verb_lvl, uint *cpuprio_list, uint cpuprio_list_count);

/** Encapsulates all user-priovided PRIO affinity configuration. */
static state_t policy_setup_priority_per_cpu(uint *setup_prio_list, uint setup_prio_list_count);

/** Encapsulates all user-provided PRIO task configuration. */
static state_t policy_setup_priority_per_task(uint *setup_prio_list, uint setup_prio_list_count);

/** This function sets the priority list \p src_prio_list to \p dst_prio_list.
 * The \p mask is used to map which index of \p dst_prio_list corresponds to the i-th value
 * of \p src_prio_list, i.e., the index of the i-th cpu set in \p mask sets the index of
 * \p dst_prio_list that must be filled with the i-th value of \p src_prio_list.
 * Positions of \p dst_prio_list not corresponding to any cpu set in \p mask, are filled with
 * the PRIO_SAME value. */
static state_t set_priority_list(uint *dst_prio_list, uint dst_prio_list_count,
																 const uint *src_prio_list, uint src_prio_list_count,
																 const cpu_set_t *mask);

/** Sets \p priority on those positions of \p dst_prio_list enabled by \p mask. */
static state_t policy_set_prio_with_mask(uint *dst_prio_list, uint dst_prio_list_count,
																				 uint priority, const cpu_set_t *mask);

/** Sets \p priority on those positions of \p not enabled by \p mask. */
static state_t policy_set_prio_with_mask_inverted(uint *dst_prio_list, uint dst_prio_list_count, uint priority, const cpu_set_t *mask);

/** This function sets the priority 0 to \p prio_list to those positions indicated by \p mask. */
static state_t reset_priority_list_with_mask(uint *prio_list, uint prio_list_count, const cpu_set_t *mask);


void policy_setup_priority()
{
	uint  user_cpuprio_list_count; // Count of user-prvided priority list
	uint *user_cpuprio_list;       // User-provided list of priorities.

	if ((user_cpuprio_list = (uint *) envtoat(FLAG_PRIO_CPUS, NULL, &user_cpuprio_list_count, ID_UINT)) != NULL)
	{
		// The user provided a CPU priority list per CPU involved in the job

		use_cpuprio = 1;

		verbose_cpuprio_list(VEARL_INFO, user_cpuprio_list, user_cpuprio_list_count);

		// Set user-provided list of priorities
		if (state_fail(policy_setup_priority_per_cpu(user_cpuprio_list, user_cpuprio_list_count)))
		{
			use_cpuprio = 0;

			verbose_error("Setting up priority list: %s", state_msg);
		}
	} else if ((user_cpuprio_list = (uint *) envtoat(FLAG_PRIO_TASKS, NULL, &user_cpuprio_list_count, ID_UINT)) != NULL)
	{
		// The user provided a priority list for each task involved in the job.

		use_cpuprio = 1;

		verbose_cpuprio_list(VEARL_INFO, user_cpuprio_list, user_cpuprio_list_count);

		if (state_fail(policy_setup_priority_per_task(user_cpuprio_list,
						user_cpuprio_list_count)))
		{

			use_cpuprio = 0;

			verbose_error("Setting up priority list: %s", state_msg);
		}
	} else {
		// If the user didn't set these flags, it is not needed get cpuprio enabled.
		use_cpuprio = 0;
	}
}


void policy_reset_priority()
{
	const metrics_t *cpuprio_api = metrics_get(API_ISST);
	if (!is_master || !cpuprio_api)
	{
		return;
	}

	if (!cpuprio_api->ok || !use_cpuprio || !mgt_cpufreq_prio_is_enabled())
	{
		return;
	}

	verbose_master(VEARL_INFO, "Resetting priority list...");
	if (state_fail(reset_priority_list_with_mask((uint *) cpuprio_api->current_list,
								 cpuprio_api->devs_count, &lib_shared_region->node_mask)))
	{
		verbose_error_master("Resetting priority list: %s", state_msg);
		return;
	}

	if (VERB_ON(VEARL_INFO))
	{
		char buffer[SZ_BUFFER_EXTRA];
		mgt_cpuprio_data_tostr((cpuprio_t *) cpuprio_api->avail_list,
				(uint *) cpuprio_api->current_list, buffer, SZ_BUFFER_EXTRA);

		verbose_info_master("CPUPRIO list of priorities\n%s", buffer);
	}

	if (state_fail(mgt_cpufreq_prio_set_current_list((uint *) cpuprio_api->current_list)))
	{
		verbose_error_master("Setting CPU PRIO list.");
	}
}


static void verbose_cpuprio_list(int verb_lvl, uint *cpuprio_list, uint cpuprio_list_count)
{
	if (VERB_ON(verb_lvl))
	{
		if (!cpuprio_list)
		{
			return;
		}
		verbosen_master(0, "CPUPRIO list read:");

		int i;
		for (i = 0; i < cpuprio_list_count - 1; i++)
		{
			verbosen_master(0, " %u", cpuprio_list[i]);
		}

		verbose_master(0, " %u", cpuprio_list[i]);
	}
}


static state_t policy_setup_priority_per_cpu(uint *setup_prio_list, uint setup_prio_list_count)
{
	if (setup_prio_list && setup_prio_list_count)
	{
		const metrics_t *cpuprio_api = metrics_get(API_ISST);
		if (!cpuprio_api)
		{
			// If we don't have an API, we won't set any priority configuration.
			return_msg(EAR_ERROR, Generr.api_undefined);
		}

		state_t ret = set_priority_list((uint *) cpuprio_api->current_list,
				cpuprio_api->devs_count,
				(const uint *) setup_prio_list,
				setup_prio_list_count,
				(const cpu_set_t*) &lib_shared_region->node_mask);

		if (state_fail(ret))
		{
			return_msg(EAR_ERROR, "Setting priority list.");
		}

		if (VERB_ON(VEARL_INFO))
		{
			verbose_info_master("Setting user-provided CPU priorities...");
			mgt_cpufreq_prio_data_print((cpuprio_t *) cpuprio_api->avail_list,
					(uint *) cpuprio_api->current_list, verb_channel);
		}

		if (!mgt_cpufreq_prio_is_enabled())
		{
			// We enable priority system in the case it is not yet created.
			if (state_fail(mgt_cpufreq_prio_enable()))
			{
				return_msg(EAR_ERROR, Generr.api_uninitialized);
			}
		}

		if (state_fail(mgt_cpufreq_prio_set_current_list((uint *) cpuprio_api->current_list)))
		{
			return_msg(EAR_ERROR, "Setting CPU PRIO list.");
		}
	} else {
		return_msg(EAR_ERROR, Generr.input_null);
	}

	return EAR_SUCCESS;
}


static state_t policy_setup_priority_per_task(uint *setup_prio_list, uint setup_prio_list_count)
{
	if (setup_prio_list && setup_prio_list_count)
	{
		const metrics_t *cpuprio_api = metrics_get(API_ISST);
		if (!cpuprio_api)
		{
			// If we don't have an API, we won't set any priority configuration.
			return_msg(EAR_ERROR, Generr.api_undefined);
		}

		for (int i = 0; i < lib_shared_region->num_processes; i++)
		{
			const cpu_set_t *mask = (const cpu_set_t *) &sig_shared_region[i].cpu_mask;

			int prio_list_idx = sig_shared_region[i].mpi_info.rank;
			if (prio_list_idx < setup_prio_list_count)
			{
				uint prio = setup_prio_list[prio_list_idx];

				if (state_fail(policy_set_prio_with_mask((uint *) cpuprio_api->current_list, cpuprio_api->devs_count, prio, mask)))
				{
					char err_msg[64];
					snprintf(err_msg, sizeof(err_msg), "Setting priority list for task %d (%s)", prio_list_idx, state_msg);
					return_msg(EAR_ERROR, err_msg);
				}
			} else {
				// Set PRIO_SAME on CPUs of that rank not indexed by the priority list.
				if (state_fail(policy_set_prio_with_mask((uint *) cpuprio_api->current_list, cpuprio_api->devs_count, PRIO_SAME, mask)))
				{
					char err_msg[64];
					snprintf(err_msg, sizeof(err_msg), "Setting priority list for task %d (%s)", prio_list_idx, state_msg);
					return_msg(EAR_ERROR, err_msg);
				}
			}
		}

		// Set to PRIO_SAME all CPUs not in node mask
		policy_set_prio_with_mask_inverted((uint *) cpuprio_api->current_list, cpuprio_api->devs_count, PRIO_SAME, (const cpu_set_t *) &lib_shared_region->node_mask);

		if (VERB_ON(VEARL_INFO))
		{
			verbose_master(0, "%sPOLICY%s Setting CPU priorities...", COL_BLU, COL_CLR);
			mgt_cpufreq_prio_data_print((cpuprio_t *) cpuprio_api->avail_list,
					(uint *) cpuprio_api->current_list, verb_channel);
		}

		if (!mgt_cpufreq_prio_is_enabled())
		{
			// We enable priority system in the case it is not yet enabled.
			if (state_fail(mgt_cpufreq_prio_enable()))
			{
				return_msg(EAR_ERROR, Generr.api_uninitialized);
			}
		}

		if (state_fail(mgt_cpufreq_prio_set_current_list((uint *) cpuprio_api->current_list)))
		{
			return_msg(EAR_ERROR, "Setting CPU PRIO list.");
		}
	} else {
		return_msg(EAR_ERROR, Generr.input_null);
	}

	return EAR_SUCCESS;
}


static state_t set_priority_list(uint *dst_prio_list, uint dst_prio_list_count,
																 const uint *src_prio_list, uint src_prio_list_count,
																 const cpu_set_t *mask)
{
	if (dst_prio_list && src_prio_list && mask)
	{
		int prio_list_idx = 0; // i-th src_prio_list value
		int dev_idx = 0;       // i-th dst_prio_list value

		// We iterate at most min(src, dst)
		for (dev_idx = 0; dev_idx < dst_prio_list_count && prio_list_idx < src_prio_list_count; dev_idx++)
		{
			dst_prio_list[dev_idx] = CPU_ISSET(dev_idx, mask) ? src_prio_list[prio_list_idx++] : PRIO_SAME;
		}

		// If there are remaining indexes of dst not set, we set the idle priority
		while (dev_idx < dst_prio_list_count)
		{
			dst_prio_list[dev_idx++] = PRIO_SAME;
		}

	} else {
		return_msg(EAR_ERROR, Generr.input_null);
	}

	return EAR_SUCCESS;
}


static state_t policy_set_prio_with_mask(uint *dst_prio_list, uint dst_prio_list_count,
																				 uint priority, const cpu_set_t *mask)
{
	if (dst_prio_list && mask)
	{
		for (int i = 0; i < dst_prio_list_count; i++)
		{
			if (CPU_ISSET(i, mask))
			{
				dst_prio_list[i] = priority;
			}
		}
	} else {
		return_msg(EAR_ERROR, Generr.input_null);
	}

	return EAR_SUCCESS;
}


static state_t policy_set_prio_with_mask_inverted(uint *dst_prio_list, uint dst_prio_list_count, uint priority, const cpu_set_t *mask)
{
	if (dst_prio_list && mask)
	{
		for (int i = 0; i < dst_prio_list_count; i++)
		{
			if (!CPU_ISSET(i, mask))
			{
				dst_prio_list[i] = priority;
			}
		}
	} else {
		return_msg(EAR_ERROR, Generr.input_null);
	}

	return EAR_SUCCESS;
}


static state_t reset_priority_list_with_mask(uint *prio_list, uint prio_list_count, const cpu_set_t *mask)
{
	if (prio_list && mask)
	{
		if (!prio_list_count)
		{
			return_msg(EAR_ERROR, Generr.arg_outbounds);
		}

		for (int i = 0; i < prio_list_count; i++)
		{
			prio_list[i] = CPU_ISSET(i, mask) ? 0 : PRIO_SAME;
		}
	} else {
		return_msg(EAR_ERROR, Generr.input_null);
	}

	return EAR_SUCCESS;
}
