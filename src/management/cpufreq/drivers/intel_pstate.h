/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_DRIVERS_INTEL_PSTATE_H
#define MANAGEMENT_DRIVERS_INTEL_PSTATE_H

#include <management/cpufreq/cpufreq.h>
#include <management/cpufreq/governor.h>

void mgt_intel_pstate_load(topology_t *tp_in, mgt_ps_driver_ops_t *ops);

state_t mgt_intel_pstate_init();

state_t mgt_intel_pstate_dispose();

// Resets to initial configuration (keeper maintained)
state_t mgt_intel_pstate_reset();

void mgt_intel_pstate_get_freq_details(freq_details_t *details);

state_t mgt_intel_pstate_get_available_list(const ullong **freq_list, uint *freq_count);

state_t mgt_intel_pstate_get_current_list(const ullong **freq_list);

state_t mgt_intel_pstate_get_boost(uint *boost_enabled);

state_t mgt_intel_pstate_set_current_list(uint *freqs_index);

state_t mgt_intel_pstate_set_current(uint freq_index, int cpu);

state_t mgt_intel_pstate_governor_get(uint *governor);

state_t mgt_intel_pstate_governor_get_list(uint *governors);

state_t mgt_intel_pstate_governor_set(uint governor);

state_t mgt_intel_pstate_governor_set_mask(uint governor, cpu_set_t mask);

state_t mgt_intel_pstate_governor_set_list(uint *governors);

// Helper functions that can be shared
int mgt_intel_pstate_governor_is_available(uint governor);

int mgt_intel_pstate_read_cpuinfo(int max, ullong *khz);

int mgt_intel_pstate_read_scaling(int max, ullong *khz);

#endif //MANAGEMENT_DRIVERS_INTEL_PSTATE_H