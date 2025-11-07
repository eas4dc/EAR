/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _SHARED_TYPES_H
#define _SHARED_TYPES_H

#include <common/config.h>
#include <common/types/coefficient.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/generic.h>

#include <common/types/pc_app_info.h>
#include <daemon/app_mgt.h>
#include <daemon/powercap/powercap.h>

typedef struct services_conf {
    eard_conf_t eard;
    eargm_conf_t eargmd;
    db_conf_t db;
    eardb_conf_t eardbd;
    char net_ext[ID_SIZE];
} services_conf_t;

typedef struct settings_conf {
    uint id;
    uint user_type;
    uint learning;
    uint lib_enabled;
    uint policy;
    ulong max_freq;
    ulong def_freq;
    uint def_p_state;
    double settings[MAX_POLICY_SETTINGS];
    char policy_name[64];
    earlib_conf_t lib_info;
    double min_sig_power;
    double max_sig_power;
    double max_power_cap;
    uint report_loops;
    conf_install_t installation;
    ulong max_avx512_freq;
    ulong max_avx2_freq;
    node_powercap_opt_t pc_opt;
    uint island;
    char tag[GENERIC_NAME];
    int cpu_max_pstate;
    int imc_max_pstate;
} settings_conf_t;

typedef struct resched {
    int force_rescheduling;
} resched_t;

#include <daemon/power_monitor_app.h>

#endif
