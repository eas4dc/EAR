/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <common/output/debug.h>
#include <data_center_monitor/plugins/conf.h>

static conf_t     conf;

declr_up_get_tag()
{
    *tag = "conf";
    *tags_deps = NULL;
}

declr_up_action_init(_conf)
{
    static char buffer[4096];
    *data_alloc = &conf;

    topology_init(&conf.tp); // It doesn't fail
    // Getting hostname
    if (gethostname(conf.hostname_full, sizeof(conf.hostname_full)) < 0) {
        sprintf(buffer, "Error getting node name (%s)", strerror(errno));
        return buffer;
    }
    strcpy(conf.hostname, conf.hostname_full);
    strtok(conf.hostname, "."); // Alias
    // Getting cluster_conf
    if (state_fail(get_ear_conf_path(buffer))) {
        return "Configuration path not found";
    }
    debug("Reading ear.conf in '%s' in node '%s'", buffer, conf.hostname);
    if (state_fail(read_cluster_conf(buffer, &conf.cluster))) {
        return "Error while reading ear.conf file";
    }
    if ((conf.node = get_my_node_conf(&conf.cluster, conf.hostname)) == NULL) {
        if ((conf.node = get_my_node_conf(&conf.cluster, conf.hostname_full)) == NULL) {
            return "Node not found in ear.conf";
        }
    }
    strcpy(conf.cluster.install.obj_ener,conf.node->energy_plugin);
    conf.cluster_loaded = 1;
    debug("database: %s", conf.cluster.database.database);
    return "Configuration plugin loaded correctly";
}
