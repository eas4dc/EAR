/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef CPU_GENERIC_POWERCAP_H
#define CPU_GENERIC_POWERCAP_H
#include <common/states.h>
#include <common/system/monitor.h>

state_t cpu_pc_disable_powercap_policy(uint pid);
state_t cpu_pc_disable_powercap_policies();
state_t cpu_pc_enable_powercap_policies();
state_t cpu_pc_set_powercap_value(uint pid, uint domain, uint limit);
state_t cpu_pc_get_powercap_value(uint pid, uint *powercap);
uint cpu_pc_is_powercap_policy_enabled(uint pid);
void cpu_pc_print_powercap_value(int fd);
void cpu_pc_powercap_to_str(char *b);

state_t cpu_pc_enable();
state_t cpu_pc_disable();

void cpu_pc_set_status(uint status);
uint cpu_pc_get_powercap_stragetgy();
void cpu_pc_set_pc_mode(uint mode);

#endif
