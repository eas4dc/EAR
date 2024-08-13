/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


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
#include <common/types/log.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <daemon/local_api/eard_api.h>



#define LOG_FILE 1

#if DB_FILES
static int fd_log=-1;
static char my_log_buffer[1024];
static char log_name[128];
#endif
#if USE_DB
static char log_nodename[GENERIC_NAME];
#endif

void init_log()
{
#if LOG_FILE
#if DB_FILES
	mode_t my_mask;
	time_t curr_time;
    struct tm *current_t;
	char nodename[GENERIC_NAME];
    char s[64];
	if (fd_log>=0) return;
	time(&curr_time);
	my_mask=umask(0);	
	gethostname(nodename, sizeof(nodename));
	strtok(nodename, ".");
	sprintf(log_name,"EAR.%s.log",nodename);
	verbose(2, "creating %s log file", log_name);
	fd_log=open(log_name,O_WRONLY|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (fd_log<0){
		verbose(0, "ERROR while creating EAR log file %s (%s)", log_name, strerror(errno));
	}
	umask(my_mask);
    current_t=localtime(&curr_time);
    strftime(s, sizeof(s), "%c", current_t);
	sprintf(my_log_buffer,"----------------------	EAR log created %s ------------------\n",s);
	write(fd_log,my_log_buffer,strlen(my_log_buffer));
#endif
#if USE_DB
	gethostname(log_nodename, sizeof(log_nodename));
	strtok(log_nodename, ".");
#endif
#endif
}
void end_log()
{
#if LOG_FILE
#if DB_FILES
	time_t curr_time;
	struct tm *current_t;
	char s[64];
	if (fd_log<0) return;
	time(&curr_time);
	current_t=localtime(&curr_time);
	strftime(s, sizeof(s), "%c", current_t);
	sprintf(my_log_buffer,"----------------------	EAR log closed %s ------------------\n",s);
	write(fd_log,my_log_buffer,strlen(my_log_buffer));
	close(fd_log);
#endif
#endif
}
void report_new_event(ear_event_t *event)
{
#if LOG_FILE

#if DB_FILES
	time_t curr_time;
	struct tm *current_t;
	char s[64];
    
	if (fd_log<0) return;
	time(&curr_time);
	current_t=localtime(&curr_time);
	strftime(s, sizeof(s), "%c", current_t);

	switch(event->event){
		case ENERGY_POLICY_NEW_FREQ:
    		sprintf(my_log_buffer,"%s : job_id %u --> Frequency changed to %lu because of energy policy\n",s,event->jid,event->value);
			break;
		case GLOBAL_ENERGY_POLICY:
    		sprintf(my_log_buffer,"%s : job_id %u --> Frequency changed to %lu because of global Energy Budget\n",s,event->jid,event->value);
			break;
		case ENERGY_POLICY_FAILS:
    		sprintf(my_log_buffer,"%s : job_id %u --> Frequency changed to %lu because of policy projections failure\n",s,event->jid,event->value);
			break;
		case DYNAIS_OFF:
    		sprintf(my_log_buffer,"%s : job_id %u --> DynAIS off because of overhead\n",s,event->jid);
			break;
	}

    write(fd_log,my_log_buffer,strlen(my_log_buffer));
#endif

#if USE_DB
	/* we request the daemon to write the event in the DB */
	event->timestamp=time(NULL);
	strcpy(event->node_id,log_nodename);
	//eards_write_event(event);
	//db_insert_ear_event(event);
#endif

#endif
}

void log_report_new_freq(job_id job,job_id step_id,ulong newf)
{
#if LOG_FILE
    ear_event_t new_event;
    new_event.event=ENERGY_POLICY_NEW_FREQ;
    new_event.jid=job;
    new_event.step_id=step_id;
    new_event.value=newf ;
    report_new_event(&new_event);
#endif
}


void log_report_dynais_off(job_id job,job_id sid)
{
#if LOG_FILE
    ear_event_t new_event;
    new_event.event=DYNAIS_OFF;
    new_event.jid=job;
    new_event.step_id=sid;
    new_event.value=0; //0 as it is not relevant and better than an uninitialised value
    report_new_event(&new_event);
#endif
}



void log_report_max_tries(job_id job,job_id sid,ulong newf)
{
#if LOG_FILE
    ear_event_t new_event;
    new_event.event=ENERGY_POLICY_FAILS;
    new_event.jid=job;
    new_event.step_id=sid;
    new_event.value=newf;
    report_new_event(&new_event);
#endif
}

void log_report_global_policy_freq(job_id job,job_id sid,ulong newf)
{
#if LOG_FILE
    ear_event_t new_event;
    new_event.event=GLOBAL_ENERGY_POLICY;
    new_event.value=newf;
    new_event.jid=job;
	new_event.step_id=sid;
    report_new_event(&new_event);
#endif
}



	
