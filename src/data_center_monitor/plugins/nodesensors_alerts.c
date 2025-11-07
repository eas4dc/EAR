/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

// #define SHOW_DEBUGS 1

#include <string.h>

#include <common/config.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/system/plugin_manager.h>
#include <common/utils/string.h>
#include <data_center_monitor/plugins/nodesensors.h>

static cluster_sensors_t *edcmon_info;
// static ullong zero = 0LLU;

static char alert_action[128];

#define NO_ACTION 0
#define LOG       1
static uint warning_action = NO_ACTION;

// This plugin checks nodesensors values and execute actions, all is hardcoded for now

#define POWER_LIMIT       2000.0
#define TEMPERATURE_LIMIT 93.0
#define HUMIDITY_LIMIT    16.0

declr_up_get_tag()
{
    *tag = "nodesensors_alerts";
    // To be executed with the same period than nodesensors
    *tags_deps = "<!nodesensors";
}

// Is it neded ?
declr_up_action_init(_nodesensors_alerts)
{
    // In this case is the specific report plugin
    char **args = (char **) data;
    if (args[0] != NULL) {
        strcpy(alert_action, args[0]);
    } else
        strcpy(alert_action, "log");

    if (strcmp(alert_action, "log") == 0) {
        warning_action = LOG;
    } else {
        warning_action = NO_ACTION;
    }

    return rsprintf("nodesensors_alerts action init(): received tag %s action %s", tag, alert_action);
}

declr_up_action_init(_nodesensors)
{
    edcmon_info = (cluster_sensors_t *) data;

    return rsprintf("nodesensors_alert action init(_nodesensors): received tag %s", tag);
}

static uint sensor_warning(nodesensor_t *info)
{
    if (info->power > POWER_LIMIT)
        return 1;
    return 0;
}

static void execute_action(nodesensor_t *info)
{
    switch (warning_action) {
        case LOG:
            verbose(0, "WARNING: node %s power %lf ", info->nodename, info->power);
            break;
        case NO_ACTION:
            break;
    }
}

#define VERBOSE_DATA 0

declr_up_action_periodic(_nodesensors)
{
    // Can data change and then needs to be updated here? Or must be initializd init (as it is)?
    debug("nodesensors_alert action tag %s", tag);

    verbose(0, "Nodesensors_alerts: %u items received", edcmon_info->num_items);
    for (uint items = 0; items < edcmon_info->num_items; items++) {
        if (sensor_warning(&edcmon_info->edcmon_data[items])) {
            execute_action(&edcmon_info->edcmon_data[items]);
        }
#if VERBOSE_DATA
        verbose(0, "WARNING EDCMON[%u] node %s power %lf ", items, edcmon_info->edcmon_data[items].nodename,
                edcmon_info->edcmon_data[items].power);
#endif
    }
    return rsprintf("nodesensors_alerts action periodic(_nodesensors): received tag %s", tag);
}
