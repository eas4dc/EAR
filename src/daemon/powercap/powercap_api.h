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

state_t disable();
state_t enable();
state_t set_powercap_value(uint pid,uint domain,uint limit);
state_t get_powercap_value(uint pid,uint *powercap);
uint is_powercap_policy_enabled(uint pid);
void print_powercap_value(int fd);
void powercap_to_str(char *b);
void set_status(uint status);
uint get_powercap_strategy();
void set_app_req_freq(ulong f);


