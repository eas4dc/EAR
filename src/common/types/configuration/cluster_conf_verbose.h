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

#ifndef _CLUSTER_CONF_VERBOSE
#define _CLUSTER_CONF_VERBOSE
#include <common/types/configuration/cluster_conf_eargm.h>
#include <common/types/configuration/cluster_conf_eard.h>
#include <common/types/configuration/cluster_conf_eardbd.h>
#include <common/types/configuration/cluster_conf_earlib.h>
#include <common/types/configuration/cluster_conf_etag.h>

void print_database_conf(db_conf_t *conf);
void print_islands_conf(node_island_t *conf);
void print_tags_conf(tag_t *tag);
#endif

