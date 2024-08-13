/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <stdio.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/output/verbose.h>
#include <report/report.h>


static char *csv_log_file_env;
static char csv_loop_log_file[1024];
static char csv_log_file[1024];
static uint must_report;
state_t report_init(report_id_t *id,cluster_conf_t *cconf)
{
	debug("eard report_init");
	char nodename[128];
	if (id->master_rank >= 0) must_report = 1;
	if (!must_report) return EAR_SUCCESS;

	gethostname(nodename, sizeof(nodename));
	strtok(nodename, ".");

	csv_log_file_env = ear_getenv(ENV_FLAG_PATH_USERDB);
	/* Loop filename is automatically generated */
	if (csv_log_file_env != NULL){
		xsnprintf(csv_log_file,sizeof(csv_log_file),"%s",csv_log_file_env);
	}else{
		xsnprintf(csv_log_file,sizeof(csv_log_file),"ear_app_log");
	}
	xsnprintf(csv_loop_log_file,sizeof(csv_loop_log_file),"%s.%s.loops.csv",csv_log_file,nodename);
	xstrncat(csv_log_file,".csv",sizeof(csv_log_file));
	return EAR_SUCCESS;
}

state_t report_applications(report_id_t *id,application_t *apps, uint count)
{
	int i;
	if (!must_report) return EAR_SUCCESS;
	debug("csv report_applications");
	if ((apps == NULL) || (count == 0)) return EAR_SUCCESS;
	for (i=0;i<count;i++){
		append_application_text_file(csv_log_file,&apps[i],1, 1, 0);
	}
	return EAR_SUCCESS;
}

state_t report_misc(report_id_t *id, uint type, const char *data, uint count)
{
  if (type == WF_APPLICATION){
          report_applications(id, (application_t *)data, count);
  }
  return EAR_SUCCESS;

}


state_t report_loops(report_id_t *id,loop_t *loops, uint count)
{
	int i;
	if (!must_report) return EAR_SUCCESS;
	debug("csv report_loops");
	if ((loops == NULL) || (count == 0)) return EAR_SUCCESS;
	for (i=0;i<count;i++){
		append_loop_text_file_no_job(csv_loop_log_file,&loops[i], 1, 0, ' ');
	}
	return EAR_SUCCESS;
}
