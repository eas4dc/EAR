/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
