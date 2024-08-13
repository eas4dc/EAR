/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_DRIVERS_ACPI_CPUFREQ_H
#define MANAGEMENT_DRIVERS_ACPI_CPUFREQ_H

#include <management/cpufreq/cpufreq.h>
#include <management/cpufreq/governor.h>

// This API contacts with CPUFreq files to change frequency and governor. These
// files are used by the ACPI_CPUFreq and Intel_PSTATE drivers. Is adapted to
// serve EAR cpufreq API.
//
// More information about CPUFreq fies
//	https://www.kernel.org/doc/html/v4.14/admin-guide/pm/cpufreq.html

/* Drivers can load without permissions to set frequency/governor. */
void mgt_acpi_cpufreq_load(topology_t *tp, mgt_ps_driver_ops_t *ops);

state_t mgt_acpi_cpufreq_init();

state_t mgt_acpi_cpufreq_dispose();

state_t mgt_acpi_cpufreq_reset();

void mgt_acpi_cpufreq_get_freq_details(freq_details_t *details);

state_t mgt_acpi_cpufreq_get_available_list(const ullong **freq_list, uint *freq_count);

state_t mgt_acpi_cpufreq_get_current_list(const ullong **freq_list);

state_t mgt_acpi_cpufreq_get_boost(uint *boost_enabled);

state_t mgt_acpi_cpufreq_set_current_list(uint *freq_index);

/* Sets a frequency in a specified CPU. Use all_cpus to set that P_STATE in all CPUs. */
state_t mgt_acpi_cpufreq_set_current(uint freq_index, int cpu);

// Governors
state_t mgt_acpi_cpufreq_governor_get(uint *governor);

state_t mgt_acpi_cpufreq_governor_get_list(uint *governors);

/* You can recover the governor (and its frequency) by setting Governor.last and Governor.init. */
state_t mgt_acpi_cpufreq_governor_set(uint governor);

state_t mgt_acpi_cpufreq_governor_set_mask(uint governor, cpu_set_t mask);

state_t mgt_acpi_cpufreq_governor_set_list(uint *governors);

int mgt_acpi_cpufreq_governor_is_available(uint governor);

#endif //MANAGEMENT_DRIVERS_ACPI_CPUFREQ_H