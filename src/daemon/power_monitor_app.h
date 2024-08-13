/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/**
 * \file power_monitoring.h
 * \brief This file offers the API for the periodic power monitoring.
 * It is used by the power_monitoring thread created by EARD.
 */

#ifndef _POWER_MONITORING_APP_H_
#define _POWER_MONITORING_APP_H_

#include <linux/version.h>
#include <common/types/application.h>
#include <common/messaging/msg_conf.h>
#include <metrics/energy/energy_node.h>
#include <metrics/accumulators/power_metrics.h>
#include <management/cpufreq/frequency.h>
#include <daemon/node_metrics.h>
#include <daemon/shared_configuration.h>


#define PER_JOB_SHARED_REGIONS  5
#define SETTINGS_AREA           0
#define RESCHED_AREA            1
#define APP_MGT_AREA            2
#define PC_APP_AREA             3
#define SELF                    4

#define APP_DEFAULT  0
#define APP_FINISHED 1

typedef struct powermon_app {
    application_t     app; //
    pid_t             master_pid; // ?
    uint              job_created;
    uint              is_job; //
    char              state;
    uint              plug_num_cpus; // Number of CPUs from external (batch sched. plug-in)
    cpu_set_t         plug_mask; // Job's mask
    // cpu_set_t         policy_mask; // ?
    uint              earl_num_cpus; // ?
    uint              local_ids; // ?
    energy_data_t     energy_init;
    governor_t        governor; // Job's CPUs governor before job beginning.
    ulong             current_freq; // ?
    ulong             policy_freq; // ?
    int               sig_reported; // ?
    settings_conf_t  *settings; //
    resched_t        *resched; //
    app_mgt_t        *app_info; //
    pc_app_info_t    *pc_app_info; //
    cpufreq_t        *freq_job1; // 
    ulong            *freq_diff; //
    loop_t            last_loop; //
    uint              exclusive; //
    accum_power_sig_t accum_ps; //
    uint             *prio_idx_list; // List of CPU priorities before job beginning.
    pthread_mutex_t   powermon_app_mutex;
    void              *sh_self;
    int								fd_shared_areas[PER_JOB_SHARED_REGIONS]; 
} powermon_app_t;


#endif
