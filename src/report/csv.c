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
