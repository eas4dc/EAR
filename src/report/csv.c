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

#include <stdio.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/output/verbose.h>

static char *csv_log_file_env;
static char csv_loop_log_file[1024];
static char csv_log_file[1024];

state_t report_init()
{
	debug("eard report_init");
	char nodename[128];
	gethostname(nodename, sizeof(nodename));
	strtok(nodename, ".");

	csv_log_file_env = getenv(VAR_OPT_USDB);
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

state_t report_applications(application_t *apps, uint count)
{
	int i;
	debug("csv report_applications");
	if ((apps == NULL) || (count == 0)) return EAR_SUCCESS;
	for (i=0;i<count;i++){
		append_application_text_file(csv_log_file,&apps[i],1);
	}
	return EAR_SUCCESS;
}

state_t report_loops(loop_t *loops, uint count)
{
	int i;
	debug("csv report_loops");
	if ((loops == NULL) || (count == 0)) return EAR_SUCCESS;
	for (i=0;i<count;i++){
		append_loop_text_file_no_job(csv_loop_log_file,&loops[i]);
	}
	return EAR_SUCCESS;
}
