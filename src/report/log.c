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

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <errno.h>

extern char *program_invocation_name;
extern char *program_invocation_short_name;

#define SHOW_DEBUGS 1

#include <stdio.h>
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
static char nodename[256];

static int fd_pm;
static int fd_apps;
static int fd_loops;
static int fd_pa;
static int fd_events;

static uint must_report = 1;
static char my_time[128];

char *date_str()
{
	struct tm *current_t;
	time_t     actual = time(NULL);
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

	sprintf(log_file_name_pm,"%s/%s.%s.periodic_metrics.txt", cconf->install.dir_temp,program_invocation_short_name,nodename);
	sprintf(log_file_name_loops,"%s/%s.%s.loops.txt", cconf->install.dir_temp,program_invocation_short_name,nodename);
	sprintf(log_file_name_apps,"%s/%s.%s.apps.txt", cconf->install.dir_temp,program_invocation_short_name,nodename);
	sprintf(log_file_name_pa,"%s/%s.%s.periodic_agg_metrics.txt", cconf->install.dir_temp,program_invocation_short_name,nodename);
	sprintf(log_file_name_events,"%s/%s.%s.events.txt", cconf->install.dir_temp,program_invocation_short_name,nodename);

	debug("Using PM file %s", log_file_name_pm);
	debug("Using APP file %s", log_file_name_apps);
	debug("Using Loop file %s", log_file_name_loops);
	debug("Using PA file %s", log_file_name_pa);
	debug("Using events file %s", log_file_name_events);

	fd_pm 		= open(log_file_name_pm,O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IROTH);
	fd_apps 	= open(log_file_name_apps,O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IROTH);
	fd_loops 	= open(log_file_name_loops,O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IROTH);
	fd_pa 		= open(log_file_name_pa,O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IROTH);
	fd_events 	= open(log_file_name_events,O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IROTH);

	if ((fd_pm < 0) || (fd_apps < 0) || (fd_loops < 0) || (fd_pa < 0) || (fd_events < 0)){
		debug("Error creating files, log plugin disabled");
		must_report = 0;
	}

  chmod(log_file_name_pm, S_IRUSR|S_IWUSR|S_IROTH);
  chmod(log_file_name_apps, S_IRUSR|S_IWUSR|S_IROTH);
  chmod(log_file_name_loops, S_IRUSR|S_IWUSR|S_IROTH);
  chmod(log_file_name_pa, S_IRUSR|S_IWUSR|S_IROTH);
  chmod(log_file_name_events, S_IRUSR|S_IWUSR|S_IROTH);

	return EAR_SUCCESS;

}

static void log_powersig(int fd, power_signature_t *ps)
{
	dprintf(fd,"power (node %.1lf/cpu %.1lf/dram %.1lf) EDP %.lf time %.1lf avg. CPU_freq %lu ", 
		ps->DC_power, ps->PCK_power, ps->DRAM_power, ps->EDP, ps->time, ps->avg_f);
}

static void log_signature(int fd, signature_t * sig)
{
	char sig_str[1024];
	signature_to_str(sig, sig_str, sizeof(sig_str));
	dprintf(fd, sig_str);
}

state_t report_applications(report_id_t *id,application_t *apps, uint count)
{
	if (!must_report) return EAR_SUCCESS;

	for (uint i = 0; i< count; i++){
		dprintf(fd_apps, "app[%u]: id %lu step %lu user %s start_time %u end_time %u policy %s th %.3lf procs %lu def_cpufreq %lu earl %d node %s ", 
		i, apps[i].job.id, apps[i].job.step_id, apps[i].job.user_id, (uint)apps[i].job.start_time, (uint)apps[i].job.end_time, apps[i].job.policy, apps[i].job.th, 
		apps[i].job.procs, apps[i].job.def_f, apps[i].is_mpi, apps[i].node_id);
	       log_powersig(fd_apps, &apps[i].power_sig);
	       log_signature(fd_apps, &apps[i].signature);	
	       dprintf(fd_apps, "\n");
	}

	return EAR_SUCCESS;

}

state_t report_loops(report_id_t *id,loop_t *loops, uint count)
{
	if (!must_report) return EAR_SUCCESS;
	for (uint i = 0; i< count; i++){
		dprintf(fd_loops,"loop[%u]:" , i);
		log_signature(fd_loops, &loops[i].signature);
		dprintf(fd_loops,"\n");
	}
	return EAR_SUCCESS;
}

state_t report_events(report_id_t *id,ear_event_t *eves, uint count)
{
	if (!must_report) return EAR_SUCCESS;
	char event_desc[128];
	for (uint i = 0; i< count; i++){
		event_type_to_str(&eves[i], event_desc, sizeof(event_desc));
		dprintf(fd_events,"event[%d]: jid %lu stepid %lu node %s event %s value %ld time %u\n",
				i, eves[i].jid, eves[i].step_id, eves[i].node_id,event_desc ,
				eves[i].value, (uint)eves[i].timestamp);
	}
	return EAR_SUCCESS;
}

state_t report_periodic_metrics(report_id_t *id,periodic_metric_t *mets, uint count)
{
	printf("report_periodic_metrics\n");
	if (!must_report) return EAR_SUCCESS;
	float node, cpu, dram, gpu = 0;
	long elapsed;
	for (uint i = 0; i< count; i++){
		elapsed = mets[i].end_time - mets[i].start_time;
		node = (float) mets[i].DC_energy/elapsed;
		cpu  = (float) mets[i].PCK_energy/elapsed;
		dram = (float) mets[i].DRAM_energy/elapsed;
#if USE_GPUS
		gpu  = (float) mets[i].GPU_energy/elapsed;
#endif
		dprintf(fd_pm,"%s node_metric[%u]: node %s elapsed %ld power (node %.1f/pck %.1f/dram %.1f/gpu %.1f)\n", 
				date_str(), i, mets[i].node_id, elapsed, node, cpu, dram,gpu);
	}
	return EAR_SUCCESS;
}

state_t report_misc(report_id_t *id,uint type, const char *data, uint count)
{
	if (!must_report) return EAR_SUCCESS;
	return EAR_SUCCESS;
}

state_t report_dispose(report_id_t *id)
{
	if (!must_report) return EAR_SUCCESS;
	close(fd_pm);
	close(fd_apps);
	close(fd_loops);
	close(fd_pa);
	close(fd_events);
	return EAR_SUCCESS;
}

