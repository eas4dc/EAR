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

#include <common/states.h>
#include <common/types/types.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/output/verbose.h>
#include <report/report.h>

static char log_file_name_apps[SZ_PATH];
static char log_file_name_loops[SZ_PATH];
static char log_file_name_pm[SZ_PATH];
static char log_file_name_pa[SZ_PATH];
static char log_file_name_events[SZ_PATH];
static chat nodename[256];
state_t report_init(report_id_t *id,cluster_conf_t *cconf)
{
	if (gethostname(nodename, sizeof(nodename)) < 0) {
		sprintf(nodename, "noname");
	}
  strtok(nodename, ".");

	sprintf(log_file_name_pm,"%s/%s.pm_periodic_data.txt", cconf->install.dir_tmp,nodename);
	sprintf(log_file_name_pm,"%s/%s.pm_periodic_data.txt", cconf->install.dir_tmp,nodename);
	sprintf(log_file_name_pm,"%s/%s.pm_periodic_data.txt", cconf->install.dir_tmp,nodename);
	sprintf(log_file_name_pm,"%s/%s.pm_periodic_data.txt", cconf->install.dir_tmp,nodename);
}

state_t report_applications(report_id_t *id,application_t *apps, uint count)
{
}

state_t report_loops(report_id_t *id,loop_t *loops, uint count)
{
}

state_t report_events(report_id_t *id,ear_event_t *eves, uint count)
{
}

state_t report_periodic_metrics(report_id_t *id,periodic_metric_t *mets, uint count)
{
}

state_t report_misc(report_id_t *id,uint type, const char *data, uint count)
{
}

state_t report_dispose(report_id_t *id)
{
}
