/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef UP_CONFIG_H
#define UP_CONFIG_H

#include <common/system/plugin_manager.h>
#include <common/types/configuration/cluster_conf.h>
#include <metrics/metrics.h>

typedef struct conf_s {
    char hostname[256]; // Alias
    char hostname_full[256];
    topology_t tp;
    cluster_conf_t cluster; // Its a pointer to check if was loaded
    int cluster_loaded;
    my_node_conf_t *node;
} conf_t;

#endif // UP_CONFIG_H