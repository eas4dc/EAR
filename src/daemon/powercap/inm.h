/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef INM_COMMANDS_H
#define INM_COMMANDS_H
#include <common/states.h>
#include <common/system/monitor.h>

state_t inm_disable_powercap_policy(uint pid);
state_t inm_disable_powercap_policies();
state_t inm_enable_powercap_policies();
state_t inm_set_powercap_value(uint pid, uint domain, uint limit);
state_t inm_get_powercap_value(uint pid, uint *powercap);
uint inm_is_powercap_policy_enabled(uint pid);
void inm_print_powercap_value(int fd);
void inm_powercap_to_str(char *b);

state_t inm_enable();
state_t inm_disable();

void inm_set_status(uint status);
uint inm_get_powercap_stragetgy();
void inm_set_pc_mode(uint mode);

#endif
