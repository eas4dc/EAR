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
#include <daemon/log_eard.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>

#include <report/report.h>

static char   log_nodename[GENERIC_NAME];
static double min_interval;
static time_t last_rt_event[EARD_RT_ERRORS];

void init_eard_rt_log()
{
    debug("init_eard_rt_log");

    gethostname(log_nodename, sizeof(log_nodename));

    strtok(log_nodename, ".");

    for (int i = 0; i < EARD_RT_ERRORS; i++) {
        last_rt_event[i] = 0;
    }
}

void log_report_eard_init_error(report_id_t *id, uint eventid, llong value)
{
    debug("New init EARD error %u", eventid);

    ear_event_t event;

    event.jid       = 0;
    event.step_id   = 0;	
    event.timestamp = time(NULL);
    event.event     = eventid;
    event.freq      = value;

    strcpy(event.node_id, log_nodename);

    /* We request the Daemon to write the event in the DB */
	report_events(id, &event, 1);
}

void log_report_eard_min_interval(uint secs)
{
    debug("EARD log RT errors interval defined %u", secs);

    min_interval = (double) secs;	
}


void log_report_eard_powercap_event(report_id_t *id, job_id job, job_id sid, uint eventid, llong value)
{
#if REPORT_POWERCAP
    debug("RT EARD powercap event reported %u", eventid);

    ear_event_t event;

    event.timestamp = time(NULL);
    event.jid       = job;
    event.step_id   = sid;	
    event.event     = eventid;
    event.freq      = value;

    strcpy(event.node_id,log_nodename);

	report_events(id, &event, 1);
#endif
}

void log_report_eard_rt_error(report_id_t *id, job_id job, job_id sid, uint eventid, llong value)
{
    ear_event_t event;

    event.timestamp = time(NULL);

    if (difftime(event.timestamp, last_rt_event[eventid-FIRST_RT_ERROR]) >= min_interval) {
        debug("RT EARD error reported %u", eventid);

        last_rt_event[eventid-FIRST_RT_ERROR] = event.timestamp;

        event.jid                             = job;
        event.step_id                         = sid;	
        event.event                           = eventid;
        event.freq                            = value;

        strcpy(event.node_id, log_nodename);
		report_events(id, &event, 1);
    } else {
        debug("RT EARD erro NOT reported %u because of min_interval definition", eventid);
    }
}
