/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _CLUSTER_CONF_VERBOSE
#define _CLUSTER_CONF_VERBOSE
#include <common/types/configuration/cluster_conf_eargm.h>
#include <common/types/configuration/cluster_conf_eard.h>
#include <common/types/configuration/cluster_conf_eardbd.h>
#include <common/types/configuration/cluster_conf_earlib.h>
#include <common/types/configuration/cluster_conf_etag.h>
#include <common/types/configuration/cluster_conf_tag.h>

void print_database_conf(db_conf_t *conf);
void print_islands_conf(node_island_t *conf);
void print_tags_conf(tag_t *tag, int i);
#endif

