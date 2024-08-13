/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


//#define SHOW_DEBUGS 1
#include <common/states.h>
#include <common/types/types.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/output/verbose.h>
#include <report/report.h>
#include <database_cache/eardbd_api.h>

static uint must_report = 0;
static uint connected = 0;
static my_node_conf_t *my_node_conf;
static cluster_conf_t *copy_cconf;
static char nodename[MAX_PATH_SIZE];

state_t report_init(report_id_t *id,cluster_conf_t *cconf)
{
	debug("eardbd report_init");
	state_t s;
	/* This copy is only valid for static regions. Areas such as islands, ranges etc cannot be accessed */
	copy_cconf = calloc(1, sizeof(cluster_conf_t));
	memcpy(copy_cconf , cconf, sizeof(cluster_conf_t));
	if (copy_cconf->eard.use_mysql && copy_cconf->eard.use_eardbd){
		must_report = 1;
		if (gethostname(nodename, sizeof(nodename)) < 0) {
			return EAR_ERROR;
		}
		strtok(nodename, ".");
		my_node_conf = get_my_node_conf(cconf, nodename);
	
		s = eardbd_connect(cconf, my_node_conf);
		if (state_fail(s))	return s;
		connected = 1;
	}
		
	return EAR_SUCCESS;
}

state_t report_applications(report_id_t *id,application_t *apps, uint count)
{
	state_t s;
	debug("eardbd report_applications");
	if (!must_report) return EAR_SUCCESS;
	if (!connected){ 
		s = eardbd_reconnect(copy_cconf, my_node_conf);
		if (state_fail(s))  return s;
		connected = 1;
	}
	/* If the reporting fails, we will try in next report function call */
	for (uint a = 0; (a < count) && connected; a++){ 
		if (state_fail(s = eardbd_send_application(&apps[a]))) connected = 0;
	}
	return s;
}

state_t report_loops(report_id_t *id,loop_t *loops, uint count)
{
	state_t s = EAR_SUCCESS;
	debug("eardbd report_loops");
	if (!must_report) return EAR_SUCCESS;
	if (copy_cconf->database.report_loops){
		if (!connected){ 
			s = eardbd_reconnect(copy_cconf, my_node_conf);
			if (state_fail(s))  return s;
			connected = 1;
		}
		/* If the reporting fails, we will try in next report function call */
		for (uint l = 0; (l < count) && connected; l++){
			if (state_fail(s = eardbd_send_loop(&loops[l]))) connected = 0;
		}
		}
	return s;
}

state_t report_events(report_id_t *id,ear_event_t *eves, uint count)
{
	state_t s;
	debug("eardbd report_events");
	if (!must_report) return EAR_SUCCESS;
	if (!connected){ 
		s = eardbd_reconnect(copy_cconf, my_node_conf);
		if (state_fail(s))  return s;
		connected = 1;
	}
	/* If the reporting fails, we will try in next report function call */
	for (uint e = 0; (e < count) && connected; e++){
		if (state_fail(s = eardbd_send_event(&eves[e]))) connected = 0;
	}
	return s;
}

state_t report_periodic_metrics(report_id_t *id,periodic_metric_t *mets, uint count)
{
	state_t s;
	if (!must_report) return EAR_SUCCESS;
	debug("eardbd report_periodic_metrics");
	if (!connected){ 
		s = eardbd_reconnect(copy_cconf, my_node_conf);
		if (state_fail(s))  return s;
		connected = 1;
	}
	/* If the reporting fails, we will try in next report function call */
	for (uint pm = 0; (pm < count) && connected; pm ++){
		if (state_fail(s = eardbd_send_periodic_metric(&mets[pm]))){ 
			debug("eardbd report_periodic_metrics fails");
			connected = 0;
		}
	}
	return s;
}


