/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <common/config.h>
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>           /* Definition of AT_* constants */


extern char *program_invocation_name;
extern char *program_invocation_short_name;

// #define SHOW_DEBUGS 1

#include <common/config.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/output/verbose.h>
#include <report/report.h>
#include <data_center_monitor/plugins/nodesensors.h>

static char log_file_name_sensors[SZ_PATH];
static char nodename[256];

static int fd_sensors;

static uint must_report = 1;
static char my_time[128];

char *date_str(time_t the_time)
{
	struct tm *current_t;
	time_t     actual = the_time;
	current_t = localtime(&actual);
        strftime(my_time, sizeof(my_time), "%c", current_t);
	return my_time;
}
state_t report_init(report_id_t *id,cluster_conf_t *cconf)
{
	if (gethostname(nodename, sizeof(nodename)) < 0) {
		sprintf(nodename, "noname");
	}
  	strtok(nodename, ".");

	sprintf(log_file_name_sensors,"%s/%s.%s.edcmon_sensors.txt", cconf->install.dir_temp,program_invocation_short_name,nodename);

	debug("Using edcmon_sensors file %s", log_file_name_sensors);

	fd_sensors 	= open(log_file_name_sensors,O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IROTH);

	if (fd_sensors < 0){
		debug("Error creating files, log plugin disabled");
		must_report = 0;
	}

  chmod(log_file_name_sensors, S_IRUSR|S_IWUSR|S_IROTH);

	return EAR_SUCCESS;

}

/*
 * PDU types are
 *     storage     = 0,
 *     network     = 1,
 *     management  = 2,
 *     others      = 3,
*/


state_t report_misc(report_id_t *id,uint type, const char *data_in, uint count)
{
	if (!must_report) return EAR_SUCCESS;

	nodesensor_t *data = (nodesensor_t *)data_in;

	for (uint i = 0; i < count; i ++){
	switch (type){
		case NODESENSORS_TYPE:
		dprintf(fd_sensors,"time[%s] type %s node %s power %.1lf W\n",
	       date_str(data[i].timestamp), pdu_type_to_str(data[i].type),
                data[i].nodename, data[i].power);
		break;

		default:
		dprintf(fd_sensors,"time[%s] node %s data not recognized",
		date_str(data[i].timestamp), data[i].nodename);
	}
	}
	return EAR_SUCCESS;
}

state_t report_dispose(report_id_t *id)
{
	if (!must_report) return EAR_SUCCESS;
	close(fd_sensors);
	return EAR_SUCCESS;
}

