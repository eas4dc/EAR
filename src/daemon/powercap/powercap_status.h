/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _POWERCAP_STATUS_H
#define _POWERCAP_STATUS_H
#include <common/types/pc_app_info.h>
#include <daemon/powercap/powercap_status_conf.h>

uint util_changed(ulong curr, ulong prev);

/** Given a powercap settings and a given power consumption computes the power to be released */
uint compute_power_to_release(node_powercap_opt_t *pc_opt, uint current);

/** Given a powercap settings and a given power consumption estimates if the application needs more power: GREEDY state
 */
uint more_power(node_powercap_opt_t *pc_opt, uint current);

/** Given a powercap settings and a given power consumption estimates if the application can release power :RELEASE
 * state */
uint free_power(node_powercap_opt_t *pc_opt, uint current);

/** Given a powercap settings and a given power consumption estimates if the application is ok with the allocated power
 */
uint ok_power(node_powercap_opt_t *pc_opt, uint current);

/** Given a powercap settings returns true when a powercap limit is defined */
int is_powercap_set(node_powercap_opt_t *pc_opt);

/** Checks the default powercap in powermon */
#define is_powercap_unlimited() (powermon_get_powercap_def() == POWER_CAP_UNLIMITED)

/** returns true if the powercap management is enabled and a powercap limit is set */
int is_powercap_on(node_powercap_opt_t *pc_opt);

/** Returns the powercap limit */
uint get_powercapopt_value(node_powercap_opt_t *pc_opt);
uint get_powercap_allocated(node_powercap_opt_t *pc_opt);
uint compute_extra_gpu_power(uint current, uint diff, uint target);

/** Given a current power , when running an application, returns the powercap status. It must be used only when powercap
 * is set */
uint compute_power_status(node_powercap_opt_t *pc, uint current_power);

void powercap_status_to_str(uint s, char *b);
uint compute_next_status(node_powercap_opt_t *pc, uint current_power, ulong eff_f, ulong req_f);

/** Given a max, min, current and target frequencies it returns the stress level of the subsystem. */
uint powercap_get_stress(uint max_freq, uint min_freq, uint t_freq, uint c_freq);

#endif
