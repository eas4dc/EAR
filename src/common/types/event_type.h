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

#ifndef _EVENT_H
#define _EVENT_H

#include <time.h>
#include <common/config.h>
#include <common/types/generic.h>
#include <common/types/job.h>


typedef struct ear_event {
	job_id jid;
    job_id step_id;
	char node_id[GENERIC_NAME];
	uint event;                 /*!< The event type. */
	long freq;                  /*!< The event value. TODO: The attribute name must be changed in future versions. */
	time_t timestamp;
} ear_event_t;


/* Should we move to a phases.h? */
#define APP_COMP_BOUND   1
#define APP_MPI_BOUND    2
#define APP_IO_BOUND     3
#define APP_BUSY_WAITING 4
#define APP_CPU_GPU      5
#define APP_COMP_CPU     6
#define APP_COMP_MEM     7
#define APP_COMP_MIX     8


/** \name Event types
 * This group of macros define the different EAR Events types. */
/**@{*/
/** EARL events. */
#define ENERGY_POLICY_NEW_FREQ  0
#define GLOBAL_ENERGY_POLICY    1
#define ENERGY_POLICY_FAILS     2
#define DYNAIS_OFF              3
#define EARL_STATE              4
#define EARL_PHASE              5
#define EARL_POLICY_PHASE       6
#define EARL_MPI_LOAD_BALANCE   7
#define EARL_OPT_ACCURACY       8
#define ENERGY_SAVING           9
#define POWER_SAVING            10
#define PERF_PENALTY            11
#define ENERGY_SAVING_AVG       12
#define POWER_SAVING_AVG        13
#define PERF_PENALTY_AVG        14

#define PHASE_SUMMARY_BASE      20
#define PERC_MPI                30
#define MPI_CALLS               31
#define MPI_SYNC_CALLS          32
#define MPI_BLOCK_CALLS         33
#define TIME_SYNC_CALLS         34
#define TIME_BLOCK_CALLS        35
#define TIME_MAX_BLOCK          36

#define JOB_POWER               40
#define NODE_POWER              41


/** EARD Init events. */
#define PM_CREATION_ERROR       100
#define APP_API_CREATION_ERROR  101
#define DYN_CREATION_ERROR      102
#define UNCORE_INIT_ERROR       103
#define RAPL_INIT_ERROR         104
#define ENERGY_INIT_ERROR       105
#define CONNECTOR_INIT_ERROR    106
#define RCONNECTOR_INIT_ERROR   107


/** EARD runtime events. */
#define DC_POWER_ERROR 300
#define TEMP_ERROR	   301
#define FREQ_ERROR	   302
#define RAPL_ERROR	   303
#define GBS_ERROR	   304
#define CPI_ERROR	   305


/** EARD powercap events. */
#define POWERCAP_VALUE 500
#define RESET_POWERCAP 501
#define INC_POWERCAP   502
#define RED_POWERCAP   503
#define SET_POWERCAP   504
#define SET_ASK_DEF    505
#define RELEASE_POWER  506


// TODO: These values already exist. Take a look if you want to use them again.
#define EARD_RT_ERRORS  6
#define FIRST_RT_ERROR  300


/* EARGM events. */
#define CLUSTER_POWER   600
#define NODE_POWERCAP   601
#define POWER_UNLIMITED 602
/**@}*/


/** \name Optimisation accuracy status.
 * This group of macros define the different states in which the 
 * optimisation policies can be. */
/**@{*/
#define OPT_NOT_READY  0
#define OPT_OK         1
#define OPT_NOT_OK     2
#define OPT_TRY_AGAIN  3
/**@}*/


/** \name Events types names.
 * This group of macros define the display text when printing the event type. */
/**@{*/
#define ENERGY_POLICY_NEW_FREQ_TXT "new_frequency"
#define GLOBAL_ENERGY_POLICY_TXT   "cluster_policy"
#define ENERGY_POLICY_FAILS_TXT    "policy_error"
#define DYNAIS_OFF_TXT             "dynais_off"
#define EARL_STATE_TXT             "earl_state"
#define EARL_PHASE_TXT 			   "earl_phase"
#define EARL_POLICY_PHASE_TXT 	   "policy_phase"
#define EARL_MPI_LOAD_BALANCE_TXT  "mpi_load_balance"
#define EARL_OPT_ACCURACY_TXT      "optim_accuracy"
#define ENERGY_SAVING_TXT          "energy_saving"
#define POWER_SAVING_TXT           "power_saving"
#define PERF_PENALTY_TXT           "performance_penalty"

#define OPT_NOT_READY_TXT         "optim_not_ready"
#define OPT_OK_TXT                "optim_ok"
#define OPT_NOT_OK_TXT            "optim_not_ok"
#define OPT_TRY_AGAIN_TXT         "optim_try_again"
/**@}*/


/** Writes the displaying name of the type of the event pointed by \p ev to \p str,
 * which must point to an array of at most \p max bytes. If the event type is not recognized, it writes
 * "unknown(event_type)", where event_type is the type of the event passed as argument. */
void event_type_to_str(ear_event_t *ev, char *str, size_t max);


/** \todo */
void event_value_to_str(ear_event_t *ev, char *str, size_t max);
#endif
