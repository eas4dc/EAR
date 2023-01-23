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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef _EARD_CHECKPOINT_H_
#define _EARD_CHECKPOINT_H_

#include <common/types/configuration/cluster_conf.h>
#include <daemon/power_monitor.h>

typedef struct eard_dyn_conf{
	cluster_conf_t *cconf;
	my_node_conf_t *nconf;
	powermon_app_t *pm_app;
}eard_dyn_conf_t;

void save_eard_conf(eard_dyn_conf_t *eard_dyn_conf);
void restore_eard_conf(eard_dyn_conf_t *eard_dyn_conf);

#endif
