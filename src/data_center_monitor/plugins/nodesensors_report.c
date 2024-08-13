/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

//#define SHOW_DEBUGS 1

#include <string.h>

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <common/utils/string.h>
#include <common/system/plugin_manager.h>
#include <report/report.h>
#include <data_center_monitor/plugins/conf.h>
#include <data_center_monitor/plugins/nodesensors.h>


static cluster_sensors_t *edcmon_info;
// Report
static conf_t                   *conf;
static report_id_t               rid;

//static ullong zero = 0LLU;
static char my_report_plugin[128];



declr_up_get_tag ()
{
    *tag = "nodesensors_report";
    // To be executed with the same period than nodesensors
    *tags_deps = "!conf+<!nodesensors";
}

declr_up_action_init(_nodesensors_report)
{
	// data is the list of arguments for this plugin. It is received in the format of argv (null terminated)
	// In this case is the specific report plugin
	char **args = (char **) data;
	verbose(0,"Initializing nodesensors report plugini argc received %s", args[0]);

	if (args[0] != NULL){
		strcpy(my_report_plugin, args[0]);
		strcat(my_report_plugin,".so");
	}else strcpy(my_report_plugin,"nodesensor_log.so");

	verbose(0,"Using %s report plugin", my_report_plugin);
        if (state_ok(report_load(conf->cluster.install.dir_plug,my_report_plugin ))){
                report_create_id(&rid, 0, 0, 0);
                // Initializing report manager
                if (state_fail(report_init(&rid, &conf->cluster))){
                        return rsprintf("[D] Nodesensors_report plugin cannot be loaded");
                }
        }else{
                return rsprintf("[D] Nodesensors_report plugin cannot be loaded");
        }
        verbose(0,"nodesensors_report report plugin loaded");

	return rsprintf("nodesensors_report action init(): received tag %s ", tag);
}

declr_up_action_init(_conf)
{
        conf = (conf_t *) data;

    	return rsprintf("nodesensors_report action init(_nodesensors): received tag %s", tag);
}

declr_up_action_init(_nodesensors)
{
    edcmon_info = (cluster_sensors_t *)data;

    return rsprintf("nodesensors_report action init(_nodesensors): received tag %s", tag);
}


#define VERBOSE_DATA 0
// This action will be executed after nodesensors action
declr_up_action_periodic(_nodesensors)
{
	// Can data change and then needs to be updated here? Or must be initializd init (as it is)?
    	debug("nodesensors_report action tag %s", tag);
	
	verbose(0, "Nodesensors_alters: %u items received", edcmon_info->num_items);
#if VERBOSE_DATA
	for (uint items = 0; items < edcmon_info->num_items; items++){
			verbose(0,"WARNING EDCMON[%u] node %s power %lf ", items,
			edcmon_info->edcmon_data[items].nodename,
			edcmon_info->edcmon_data[items].power);
	}
#endif
	report_misc(&rid, NODESENSORS_TYPE, (const char *)edcmon_info->edcmon_data, edcmon_info->num_items);
    	return rsprintf("nodesensors_reports action periodic(_nodesensors): received tag %s", tag);
}

