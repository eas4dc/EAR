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

/**
 * \file power_monitoring.h
 * \brief This file offers the API for the periodic power monitoring.
 * It is used by the power_monitoring thread created by EARD.
 */

#ifndef _POWER_MONITORING_H_
#define _POWER_MONITORING_H_

#include <linux/version.h>
#include <common/types/application.h>
#include <common/messaging/msg_conf.h>
#include <metrics/energy/energy_node.h>
#include <metrics/accumulators/power_metrics.h>
#include <management/cpufreq/frequency.h>
#include <daemon/node_metrics.h>
#include <daemon/shared_configuration.h>

#define MAX_NESTED_LEVELS 16384

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
} powermon_app_t;

typedef struct job_context {
    job_id id;
    uint   num_ctx;
    uint   num_cpus;
} job_context_t;

/** Periodically monitors the node power monitoring.
 * @param noinfo Argument required for the function to be executed by a thread. Not used. */
void *eard_power_monitoring(void *noinfo);


/** It must be called when EARLib contacts with EARD. */
void powermon_mpi_init(ehandler_t *eh, application_t *j);


/** It must be called when EARLib disconnects from EARD. */
void powermon_mpi_finalize(ehandler_t *eh, ulong jid, ulong sid);


/** \brief Called when a new job starts.
 * This function is called by the dynamic_configuration thread when a new_job command arrives.
 * It also is called by EARD when application passes through MPI_Init.
 * \param pmapp The power monitor application handler that will be filled with the information regarding to
 * the context it will belong to. */
void powermon_new_job(powermon_app_t *pmapp, ehandler_t *eh, application_t *j, uint from_mpi, uint is_job);


/** It must be called at when job ends. */
void powermon_end_job(ehandler_t *eh, job_id jid, job_id sid, uint is_job);


/** Called by dynamic_configuration thread when a new task is created. */
void powermon_new_task(new_task_req_t *newtask);


/** Reports the application signature to the power_monitoring module. */
void powermon_mpi_signature(application_t *application);


/** New loop signature for jid-sid. */
void powermon_loop_signature(job_id jid,job_id sid,loop_t *loops);

/** Creates the idle context. */
state_t powermon_create_idle_context();


/** After an ear.conf reload, updates configurations. */
void powermon_new_configuration();


/** if application is not mpi, automatically chages the node freq, it is called by dynamic_configuration API. */
void powermon_new_max_freq(ulong maxf);


/** if application is not mpi, automatically chages the node freq, it is called by dynamic_configuration API. */
void powermon_new_def_freq(uint p_id,ulong def);


/** Sets temporally the default and max frequency to the same value. When application is not mpi, automatically chages the node freq if needed */
void powermon_set_freq(ulong freq);


/** Restores values from ear.conf (not reloads the file). When application is not mpi, automatically chages the node freq if needed */
void powermon_restore_conf();

/** Sets temporally the policy th for min_time */
void powermon_set_th(uint p_id,double th);


/** Increases temporally the policy th for min_time */
void powermon_inc_th(uint p_id,double th);


/** Resets the current appl data */
void reset_current_app();


/** Copy src into dest */
void copy_powermon_app(powermon_app_t *dest,powermon_app_t *src);


/** \todo Add a description. */
void print_powermon_app(powermon_app_t *app);


/** \todo Add a description. */
powermon_app_t *get_powermon_app();


/** \todo Add a description. */
void powermon_get_status(status_t *my_status);


/** \todo Add a description. */
void powermon_get_power(uint64_t *power);


/** \todo Add a description. */
int powermon_get_num_applications(int only_master);


/** \todo Add a description. */
void powermon_get_app_status(app_status_t *my_status, int num_apps, int only_master);


/** \todo Add a description. */
void powermon_new_configuration();


/** \todo Add a description. */
int mark_contexts_to_finish_by_jobid(job_id id, job_id step_id);


/** \todo Add a description. */
int mark_contexts_to_finish_by_pid();

/** \todo Add a description. */
void finish_pending_contexts(ehandler_t *eh);


/** \todo Add a description. */
uint node_energy_lock(uint *tries);


/** \todo Add a description. */
void node_energy_unlock();


/** Returns whether only the Idle context exists,
 * or if all currently created contexts are marked as finished. */
uint powermon_is_idle();


/** \todo Add a description. */
uint powermon_current_power();


/** \todo Add a description. */
uint powermon_get_powercap_def();


/** \todo Add a description. */
uint powermon_get_max_powercap_def();


/** \todo Add a description. */
void powermon_report_event(uint event_type, llong value);


/** \todo Add a description. */
uint is_job_in_node(job_id id, job_context_t **jc);
#endif
