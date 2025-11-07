/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARD_COMMON_H
#define _EARD_COMMON_H

#include <common/types/configuration/cluster_conf.h>
#include <common/types/configuration/node_conf.h>

void build_energy_plugin_path(char *dst_buff, size_t buff_size, cluster_conf_t *cluster_conf,
                              my_node_conf_t *node_conf);

#endif // _EARD_COMMON_H
