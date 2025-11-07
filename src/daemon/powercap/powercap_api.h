/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

state_t disable();
state_t enable();
state_t set_powercap_value(uint pid, uint domain, uint limit);
state_t get_powercap_value(uint pid, uint *powercap);
uint is_powercap_policy_enabled(uint pid);
void print_powercap_value(int fd);
void powercap_to_str(char *b);
void set_status(uint status);
uint get_powercap_strategy();
void set_app_req_freq(ulong f);
