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
//#define SHOW_DEBUGS 1
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


void log_report_eard_powercap_event(uint usedb,uint useeardbd,job_id job,job_id sid,uint eventid,ulong value)
{
#if REPORT_POWERCAP
    ear_event_t event;

    event.timestamp=time(NULL);
    debug("RT EARD powercap event reported %u", eventid);
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
#endif
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

