/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/

#ifndef _EARD_LOG_H
#define _EARD_LOG_H

#include <time.h>
#include <common/config.h>
#include <common/types/generic.h>
#include <common/types/job.h>
#include <common/types/event_type.h>




/* EARD Init events */

#define PM_CREATION_ERROR			100
#define APP_API_CREATION_ERROR	101
#define DYN_CREATION_ERROR			102
#define UNCORE_INIT_ERROR		103
#define RAPL_INIT_ERROR 104
#define ENERGY_INIT_ERROR 105
#define CONNECTOR_INIT_ERROR 106
#define RCONNECTOR_INIT_ERROR 107



/* EARD runtime events */
#define DC_POWER_ERROR		300
#define TEMP_ERROR				301
#define FREQ_ERROR				302
#define RAPL_ERROR				303
#define GBS_ERROR					304
#define CPI_ERROR					305

#define EARD_RT_ERRORS  6
#define FIRST_RT_ERROR  300






/** Creates the log file and starts it with the current time. If it can't
*   create the file it reports it to stderr */
void init_eard_rt_log();



/** Sets the minimum interval between two runtime errors to avoid saturating the DB */
void log_report_eard_min_interval(uint secs);
/** Reports a RT error from EARD */
void log_report_eard_rt_error(uint usedb,uint useeardbd,job_id job,job_id sid,uint eventid,ulong value);
/* Reports and error when initializing EARD */
void log_report_eard_init_error(uint usedb,uint useeardbd,uint eventid,ulong value);

#endif
