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

#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <common/config.h>
#include <common/types/log_eard.h>
#include <common/output/verbose.h>

#if USE_DB
#include <database_cache/eardbd_api.h>
#include <common/database/db_helper.h>
#endif

static char log_nodename[GENERIC_NAME];
static double min_interval=0.0;
static time_t last_rt_event[EARD_RT_ERRORS];

void init_eard_rt_log()
{
	int i;
	debug("init_eard_rt_log");
	gethostname(log_nodename, sizeof(log_nodename));
	strtok(log_nodename, ".");
	for (i=0;i<EARD_RT_ERRORS;i++){
		last_rt_event[i]=0;
	}
}
void log_report_eard_init_error(uint usedb,uint useeardbd,uint eventid,ulong value)
{
	ear_event_t event;
  /* we request the daemon to write the event in the DB */
	debug("New init EARD error %u",eventid);
  event.timestamp=time(NULL);
	event.jid=0;
	event.step_id=0;	
  strcpy(event.node_id,log_nodename);
	event.event=eventid;
	event.freq=value;
	if (usedb){
  	if (useeardbd) eardbd_send_event(&event);
  	else db_insert_ear_event(&event);
	}else{
		debug("INIT EARD error  event type %u value %lu",eventid,value);
	}

}

void log_report_eard_min_interval(uint secs)
{
	debug("EARD log RT errors interval defined %u",secs);
	min_interval=(double)secs;	
}

void log_report_eard_rt_error(uint usedb,uint useeardbd,job_id job,job_id sid,uint eventid,ulong value)
{
	ear_event_t event;
	
	event.timestamp=time(NULL);
	if (difftime(event.timestamp,last_rt_event[eventid-FIRST_RT_ERROR])>=min_interval){
		debug("RT EARD error reported %u", eventid);
		last_rt_event[eventid-FIRST_RT_ERROR]=event.timestamp;
		event.jid=job;
		event.step_id=sid;	
  	strcpy(event.node_id,log_nodename);
  	event.event=eventid;
  	event.freq=value;
		if (usedb){
			if (useeardbd) eardbd_send_event(&event);
			else db_insert_ear_event(&event);
		}else{
			debug("jid %lu sid %lu event type %u value %lu",job,sid,eventid,value);
		}
	}else{
		debug("RT EARD erro NOT reported %u because of min_interval definition", eventid);
	}
}
	
