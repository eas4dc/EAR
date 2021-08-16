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

#ifndef _POWERCAP_STATUS_H
#define _POWERCAP_STATUS_H
#include <daemon/powercap/powercap_status_conf.h>
#include <common/types/pc_app_info.h>


uint util_changed(ulong curr,ulong prev);
/** Limits the power to be requested to the hardware configuration */
uint limit_max_power(node_powercap_opt_t *pc_opt,uint extra);


/** Given a powercap settings and a given power consumption computes the power to be released */
uint compute_power_to_release(node_powercap_opt_t *pc_opt,uint current);

/** Given a powercap settings and a given power consumption estimates if the application needs more power: GREEDY state */
uint more_power(node_powercap_opt_t *pc_opt,uint current);

/** Given a powercap settings and a given power consumption estimates if the application can release power :RELEASE state */
uint free_power(node_powercap_opt_t *pc_opt,uint current);

/** Given a powercap settings and a given power consumption estimates if the application is ok with the allocated power */
uint ok_power(node_powercap_opt_t *pc_opt,uint current);

/** Given a powercap settings returns true when a powercap limit is defined */
int is_powercap_set(node_powercap_opt_t *pc_opt);

/** returns true if the powercap management is enabled and a powercap limit is set */
int is_powercap_on(node_powercap_opt_t *pc_opt);

/** Returns the powercap limit */
uint get_powercapopt_value(node_powercap_opt_t *pc_opt);
uint get_powercap_allocated(node_powercap_opt_t *pc_opt);
uint compute_extra_gpu_power(uint current,uint diff,uint target);


/** Given a current power , when running an application, returns the powercap status. It must be used only when powercap is set */
uint compute_power_status(node_powercap_opt_t *pc,uint current_power);

void powercap_status_to_str(uint s,char *b);
uint compute_next_status(node_powercap_opt_t *pc,uint current_power,ulong eff_f, ulong req_f);


/** Given a max, min, current and target frequencies it returns the stress level of the subsystem. */
uint powercap_get_stress(uint max_freq, uint min_freq, uint t_freq, uint c_freq);

#endif

