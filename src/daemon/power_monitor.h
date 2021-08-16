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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

/**
*    \file power_monitoring.h
*    \brief This file offers the API for the periodic power monitoring. It is used by the power_monitoring thread created by EARD
*
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

typedef struct powermon_app{
    application_t app;
    uint job_created;
    energy_data_t energy_init;
	governor_t governor;
	ulong current_freq;
}powermon_app_t;



/** Periodically monitors the node power monitoring. 
*
*	@param frequency_monitoring sample period expressed in usecs. It is dessigned to be executed by a thread
*/
void *eard_power_monitoring(void *frequency_monitoring);

/**  It must be called when EARLib contacts with EARD 
*/

void powermon_mpi_init(ehandler_t *eh,application_t *j);

/**  It must be called when EARLib disconnects from EARD 
*/
void powermon_mpi_finalize(ehandler_t *eh);

/** It must be called at when job starts 
*/

void powermon_new_job(ehandler_t *eh,application_t *j,uint from_mpi);

/** It must be called at when job ends
*/
void powermon_end_job(ehandler_t *eh,job_id jid,job_id sid);

/** It must be called at when sbatch starts*/
void powermon_new_sbatch(application_t *j);
/** It must be called at when sbatch ends
 * */
void powermon_end_sbatch(job_id jid);

/** reports the application signature to the power_monitoring module 
*/
void powermon_mpi_signature(application_t *application);

/** if application is not mpi, automatically chages the node freq, it is called by dynamic_configuration API */
void powermon_new_max_freq(ulong maxf);

/** if application is not mpi, automatically chages the node freq, it is called by dynamic_configuration API */
void powermon_new_def_freq(uint p_id,ulong def);

/** Reduces, temporally, the current freq (default and max) based on new values. If application is not mpi, automatically chages the node freq if needed */
void powermon_red_freq(ulong max_freq,ulong def_freq);

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

/** */

void print_powermon_app(powermon_app_t *app);

powermon_app_t *get_powermon_app();

void powermon_get_status(status_t *my_status);
void powermon_get_app_status(app_status_t *my_status);

uint node_energy_lock(uint *tries);
void node_energy_unlock();

uint powermon_is_idle();
uint powermon_current_power();
uint powermon_get_powercap_def();
uint powermon_get_max_powercap_def();

void powermon_report_event(uint event_type, ulong value);

#endif
