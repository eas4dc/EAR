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

#ifndef INM_COMMANDS_H
#define INM_COMMANDS_H
#include <common/states.h>
#include <common/system/monitor.h>


state_t inm_disable_powercap_policy(uint pid);
state_t inm_disable_powercap_policies();
state_t inm_enable_powercap_policies();
state_t inm_set_powercap_value(uint pid,uint domain,uint limit);
state_t inm_get_powercap_value(uint pid,uint *powercap);
uint inm_is_powercap_policy_enabled(uint pid);
void inm_print_powercap_value(int fd);
void inm_powercap_to_str(char *b);


state_t inm_enable();
state_t inm_disable();


void inm_set_status(uint status);
uint inm_get_powercap_stragetgy();
void inm_set_pc_mode(uint mode);



#endif
