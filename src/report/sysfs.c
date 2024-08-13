/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

//#define SHOW_DEBUGS 1
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>
#include <sched.h>
#include <common/config.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/output/verbose.h>
#include <report/report.h>

#define PATH_MAX_STRING_SIZE 256

typedef long double		Lf;
typedef long		ld;

static uint must_report;
static char nodename[128]="node";
static int island=0;
static char path[1024];
static int dir;
static int chwd;
static char name[128];
static double freq_total=0;
static double avg_freq_total=0;
static double temp_total=0;
static double avg_temp_total=0;
static int period=1;
static ulong DC_energy_total=0;
static ulong DRAM_energy_total=0;
static ulong PCK_energy_total=0;
static ulong GPU_energy_total=0;
static FILE *file;
static char *env1;
static char *env2;


int makeDir(const char *dir, const mode_t mode) {
	char tmp[PATH_MAX_STRING_SIZE];
	char *p = NULL;
	struct stat sb;
	size_t len;

	/* copy path */
	len = strnlen (dir, PATH_MAX_STRING_SIZE);
	if (len == 0 || len == PATH_MAX_STRING_SIZE) {
		return -1;
	}
	memcpy (tmp, dir, len);
	tmp[len] = '\0';

	/* remove trailing slash */
	if(tmp[len - 1] == '/') {
		tmp[len - 1] = '\0';
	}

	/* check if path exists and is a directory */
	if (stat (tmp, &sb) == 0) {
		if (S_ISDIR (sb.st_mode)) {
			return 0;
		}
	}

	/* recursive mkdir */
	for(p = tmp + 1; *p; p++) {
		if(*p == '/') {
			*p = 0;
			/* test path */
			if (stat(tmp, &sb) != 0) {
				/* path does not exist - create directory */
				if (mkdir(tmp, mode) < 0) {
					return -1;
				}
			} else if (!S_ISDIR(sb.st_mode)) {
				/* not a directory */
				return -1;
			}
			*p = '/';
		}
	}
	/* test path */
	if (stat(tmp, &sb) != 0) {
		/* path does not exist - create directory */
		if (mkdir(tmp, mode) < 0) {
			return -1;
		}
	} else if (!S_ISDIR(sb.st_mode)) {
		/* not a directory */
		return -1;
	}
	return 0;
}

int openFileS(const char *filename, const char *format, char *value) {

	file = fopen(filename,"w");
	fprintf(file, format, value);
	fclose(file);

	return 0;
}

int openFileF(const char *filename, const char *format, float value) {

	file = fopen(filename,"w");
	fprintf(file, format, value);
	fclose(file);

	return 0;
}

int openFileD(const char *filename, const char *format, double value) {

	file = fopen(filename,"w");
	fprintf(file, format, value);
	fclose(file);

	return 0;
}

int openFileI(const char *filename, const char *format, ld value) {

	file = fopen(filename,"w");
	fprintf(file, format, value);
	fclose(file);

	return 0;
}

int openFileU(const char *filename, const char *format, ull value) {

	file = fopen(filename,"w");
	fprintf(file, format, value);
	fclose(file);

	return 0;
}

state_t report_init(report_id_t *id, cluster_conf_t *cconf){

	debug("report_init: Start");

	if (id->master_rank >= 0) must_report = 1;
	if (!must_report) return EAR_SUCCESS;

	gethostname(nodename, sizeof(nodename));
	strtok(nodename, ".");

	island = get_node_island(cconf, nodename);

	env1=getenv("METRICS_ROOT_DIR");
	if (env1 == NULL) {
		env1="/home/ear-sysfs-report";
	}

	env2=getenv("CLUSTER_NAME");
	if (env2 == NULL) {
		env2="cluster";
	}

	debug("report_init: End");
	return EAR_SUCCESS;
}

state_t report_periodic_metrics(report_id_t *id, periodic_metric_t *metric_list, uint count){

	debug("report_periodic_metrics: Start");
	periodic_metric_t *metric;

	if (!must_report) return EAR_SUCCESS;
	if ((metric_list == NULL) || (count == 0)) return EAR_SUCCESS;


	for (int i=0;i<count;i++){

		metric = &metric_list[i];

		if (env1 && env2) {
			sprintf(path, "%s/%s/%d/%s/%s/", env1, env2, island, nodename, "avg" );
		}
		else {
			sprintf(path, "/home/ear-sysfs-report/report_periodic_metrics/%d/%s/%s/", island, nodename, "avg" );
		}


		dir = makeDir(path, 0777);
		chwd = chdir(path);

		freq_total = freq_total + metric->avg_f;
		avg_freq_total = freq_total / period;
		openFileF("avg_cpu_freq_KHz", "%f\n", avg_freq_total);

		temp_total = temp_total + metric->temp;
		avg_temp_total = temp_total / period;
		openFileF("avg_temp_celsius", "%f\n", avg_temp_total);

		period=period+1;

		DC_energy_total = DC_energy_total + metric->DC_energy;
		openFileU("avg_dc_energy_Joules", "%lu\n", DC_energy_total);

		DRAM_energy_total = DRAM_energy_total + metric->DRAM_energy;
		openFileU("avg_dram_energy_Joules", "%lu\n", DRAM_energy_total);

		PCK_energy_total = PCK_energy_total + metric->PCK_energy;
		openFileU("avg_pck_energy_Joules", "%lu\n", PCK_energy_total);

		#if USE_GPUS
		GPU_energy_total = GPU_energy_total + metric->GPU_energy;
		openFileU("avg_gpu_energy_Joules", "%lu\n", GPU_energy_total);
		#endif

		openFileD("start_time_timestamp", "%lf\n", metric->start_time);
		openFileD("end_time_timestamp", "%lf\n", metric->end_time);


		if (env1 && env2){
			sprintf(path, "%s/%s/%d/%s/%s/", env1, env2, island, nodename, "current" );
		}
		else {
			sprintf(path, "/home/ear-sysfs-report/report_periodic_metrics/%d/%s/%s/", island, nodename, "current" );
		}

		dir = makeDir(path, 0777);
		chwd = chdir(path);

		openFileU("temp_celsius", "%lu\n", metric->temp);
		openFileU("dc_energy_Joules", "%lu\n", metric->DC_energy);
		openFileU("dram_energy_Joules", "%lu\n", metric->DRAM_energy);
		openFileU("pck_energy_Joules", "%lu\n", metric->PCK_energy);
		#if USE_GPUS
		openFileU("gpu_energy_Joules", "%lu\n", metric->GPU_energy);
		#endif
		openFileD("start_time_timestamp", "%lf\n", metric->start_time);
		openFileD("end_time_timestamp", "%lf\n", metric->end_time);

	}

	debug("report_periodic_metrics: End");
	return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id, loop_t *loops_list, uint count){

	debug("report_loops: Start");
	loop_t *loops;

	if (!must_report) return EAR_SUCCESS;
	if ((loops_list == NULL) || (count == 0)) return EAR_SUCCESS;


	for (int i=0;i<count;i++){

		loops = &loops_list[i];

		if (env1 && env2){
			sprintf(path, "%s/%s/%d/%s/%ld/%ld/%s/%s/", env1, env2, island, "jobs", loops->jid, loops->step_id, nodename, "current" );
		}
		else {
			sprintf(path, "/home/ear-sysfs-report/report_loops/jobs/%ld/%ld/%s/%s/", loops->jid, loops->step_id, nodename, "current" );
		}

		dir = makeDir(path, 0777);
		chwd = chdir(path);

		openFileF("time_timestamp", "%f\n", loops->total_iterations);
		openFileD("dc_power_watt", "%lf\n", loops->signature.DC_power);
		openFileD("dram_power_watt", "%lf\n", loops->signature.DRAM_power);
		openFileD("pck_power_watt", "%lf\n", loops->signature.PCK_power);
		openFileD("edp", "%lf\n", loops->signature.EDP);
		openFileD("mem_gbs", "%lf\n", loops->signature.GBS);
		openFileD("io_mbs", "%lf\n", loops->signature.IO_MBS);
		openFileD("tpi", "%lf\n", loops->signature.TPI);
		openFileD("cpi", "%lf\n", loops->signature.CPI);
		openFileD("gflops", "%lf\n", loops->signature.Gflops);
		openFileD("iteration_time_sec", "%lf\n", loops->signature.time);
		openFileU("l1_misses", "%llu\n", loops->signature.L1_misses);
		openFileU("l2_misses", "%llu\n", loops->signature.L2_misses);
		openFileU("l3_misses", "%llu\n", loops->signature.L3_misses);
		openFileU("instructions", "%llu\n", loops->signature.instructions);
		openFileU("cycles", "%llu\n", loops->signature.cycles);
		openFileU("avg_cpu_freq_KHz", "%lu\n", loops->signature.avg_f);
		openFileU("avg_imc_freq_KHz", "%lu\n", loops->signature.avg_imc_f);
		openFileU("def_cpu_freq_KHz", "%lu\n", loops->signature.def_f);
		openFileD("perc_mpi_percentage", "%lf\n", loops->signature.perc_MPI);

		for(int j=0;j<8;j++){
			sprintf(name, "%s_%d", "flops", j);
			openFileU(name, "%llu\n", loops->signature.FLOPS[j]);
		}

		#if USE_GPUS
		openFileI("gpus_number", "%d\n", loops->signature.gpu_sig.num_gpus);

		for(int j = 1; j <= loops->signature.gpu_sig.num_gpus; j++){

			if (env1 && env2){
				sprintf(path, "%s/%s/%d/%s/%ld/%ld/%s/%s-%d/", env1, env2, island, "jobs", loops->jid, loops->step_id, nodename, "current/GPU", j);
			}
			else {
				sprintf(path, "/home/ear-sysfs-report/report_loops/jobs/%ld/%ld/%s/%s-%d/", loops->jid, loops->step_id, nodename, "current/GPU", j);
			}

			dir = makeDir(path, 0777);
			chwd = chdir(path);

			openFileD("gpu_power_watt", "%lf\n", loops->signature.gpu_sig.gpu_data[j].GPU_power);
			openFileU("gpu_freq_KHz", "%lu\n", loops->signature.gpu_sig.gpu_data[j].GPU_freq);
			openFileU("gpu_mem_freq_KHz", "%lu\n", loops->signature.gpu_sig.gpu_data[j].GPU_mem_freq);
			openFileU("gpu_util_percentage", "%lu\n", loops->signature.gpu_sig.gpu_data[j].GPU_util);
			openFileU("gpu_mem_util_percentage", "%lu\n", loops->signature.gpu_sig.gpu_data[j].GPU_mem_util);
		}
		#endif

	}

	debug("report_loops: End");
	return EAR_SUCCESS;
}

state_t report_applications(report_id_t *id, application_t *apps_list, uint count){

	debug("report_applications: Start");
	application_t *apps;

	if (!must_report) return EAR_SUCCESS;
	if ((apps_list == NULL) || (count == 0)) return EAR_SUCCESS;


	for (int i=0;i<count;i++){

		apps =&apps_list[i];
		if (env1 && env2){
			sprintf(path, "%s/%s/%d/%s/%ld/%ld/%s/%s/", env1, env2, island, "jobs", apps->job.id, apps->job.step_id, nodename, "avg" );
		}
		else {
			sprintf(path, "/home/ear-sysfs-report/report_applications/jobs/%ld/%ld/%s/%s/", apps->job.id, apps->job.step_id, nodename, "avg" );
		}

		dir = makeDir(path, 0777);
		chwd = chdir(path);

		openFileS("app_job_username", "%s\n", apps->job.user_id);
		openFileS("app_job_group", "%s\n", apps->job.group_id);
		openFileS("app_job_name", "%s\n", apps->job.app_id);
		openFileD("app_job_start_time_timestamp", "%lf\n", apps->job.start_time);
		openFileD("app_job_end_time_timestamp", "%lf\n", apps->job.end_time);
		openFileD("app_job_start_earl_timestamp", "%lf\n", apps->job.start_mpi_time);
		openFileD("app_job_end_earl_timestamp", "%lf\n", apps->job.end_mpi_time);
		openFileS("app_job_policy", "%s\n", apps->job.policy);
		openFileU("app_job_def_cpu_freq_KHz", "%lu\n", apps->job.def_f);
		openFileD("app_dc_power_watt", "%lf\n", apps->power_sig.DC_power);
		openFileD("app_dram_power_watt", "%lf\n", apps->power_sig.DRAM_power);
		openFileD("app_pck_power_watt", "%lf\n", apps->power_sig.PCK_power);
		openFileD("app_edp", "%lf\n", apps->power_sig.EDP);
		openFileD("app_max_dc_power_watt", "%lf\n", apps->power_sig.max_DC_power);
		openFileD("app_min_dc_power_watt", "%lf\n", apps->power_sig.min_DC_power);
		openFileU("app_avg_cpu_freq_KHz", "%lu\n", apps->power_sig.avg_f);
		openFileU("app_def_cpu_freq_KHz", "%lu\n", apps->power_sig.def_f);
		openFileD("app_elapsed_time_sec", "%lf\n", apps->power_sig.time);
		openFileD("app_sig_dc_power_watt", "%lf\n", apps->signature.DC_power);
		openFileD("app_sig_dram_power_watt", "%lf\n", apps->signature.DRAM_power);
		openFileD("app_sig_pck_power_watt", "%lf\n", apps->signature.PCK_power);
		openFileD("app_sig_edp", "%lf\n", apps->signature.EDP);
		openFileD("app_sig_mem_gbs", "%lf\n", apps->signature.GBS);
		openFileD("app_sig_io_mbs", "%lf\n", apps->signature.IO_MBS);
		openFileD("app_sig_tpi", "%lf\n", apps->signature.TPI);
		openFileD("app_sig_cpi", "%lf\n", apps->signature.CPI);
		openFileD("app_sig_gflops", "%lf\n", apps->signature.Gflops);
		openFileD("app_sig_elapsed_time_sec", "%lf\n", apps->signature.time);
		openFileU("app_sig_l1_misses", "%llu\n", apps->signature.L1_misses);
		openFileU("app_sig_l2_misses", "%llu\n", apps->signature.L2_misses);
		openFileU("app_sig_l3_misses", "%llu\n", apps->signature.L3_misses);
		openFileU("app_sig_instructions", "%llu\n", apps->signature.instructions);
		openFileU("app_sig_cycles", "%llu\n", apps->signature.cycles);
		openFileU("app_sig_avg_cpu_freq_KHz", "%lu\n", apps->signature.avg_f);
		openFileU("app_sig_avg_imc_freq_KHz", "%lu\n", apps->signature.avg_imc_f);
		openFileU("app_sig_def_cpu_freq_KHz", "%lu\n", apps->signature.def_f);
		openFileD("app_sig_perc_mpi_percentage", "%lf\n", apps->signature.perc_MPI);


		for(int j=0;j<8;j++){
			sprintf(name, "%s_%d", "app_sig_flops", j);
			openFileU(name, "%llu\n", apps->signature.FLOPS[j]);
		}

		#if USE_GPUS
		openFileI("app_sig_gpus_number", "%d\n", apps->signature.gpu_sig.num_gpus);

		for(int j = 1; j <= apps->signature.gpu_sig.num_gpus; j++){

			if (env1 && env2){
				sprintf(path, "%s/%s/%d/%s/%ld/%ld/%s/%s-%d", env1, env2, island, "jobs", apps->job.id, apps->job.step_id, nodename, "avg/GPU", j);
			}
			else {
				sprintf(path, "/home/ear-sysfs-report/report_applications/jobs/%ld/%ld/%s/%s-%d/", apps->job.id, apps->job.step_id, nodename, "avg/GPU", j );
			}


			dir = makeDir(path, 0777);
			chwd = chdir(path);

			openFileD("app_sig_gpu_power_watt", "%lf\n", apps->signature.gpu_sig.gpu_data[j].GPU_power);
			openFileU("app_sig_gpu_freq_KHz", "%lu\n", apps->signature.gpu_sig.gpu_data[j].GPU_freq);
			openFileU("app_sig_gpu_mem_freq_KHz", "%lu\n", apps->signature.gpu_sig.gpu_data[j].GPU_mem_freq);
			openFileU("app_sig_gpu_util_percentage", "%lu\n", apps->signature.gpu_sig.gpu_data[j].GPU_util);
			openFileU("app_sig_gpu_mem_util_percentage", "%lu\n", apps->signature.gpu_sig.gpu_data[j].GPU_mem_util);
		}
		#endif
	}

	debug("report_applications: End");
	return EAR_SUCCESS;
}

state_t report_events(report_id_t *id, ear_event_t *eves_list, uint count){

	debug("report_events: Start");
	ear_event_t *eves;

	if (!must_report) return EAR_SUCCESS;
	if ((eves_list == NULL) || (count == 0)) return EAR_SUCCESS;


	for (int i=0;i<count;i++){

		eves = &eves_list[i];

		if (env1 && env2){
			sprintf(path, "%s/%s/%d/%s/%ld/%ld/%s/%s/", env1, env2, island, "jobs", eves->jid, eves->step_id, nodename, "current" );
		}
		else {
			sprintf(path, "/home/ear-sysfs-report/report_events/jobs/%ld/%ld/%s/%s/", eves->jid, eves->step_id, nodename, "current" );
		}

		dir = makeDir(path, 0777);
		chwd = chdir(path);

		openFileU("event", "%u\n", eves->event);
		openFileI("event_value", "%ld\n", eves->value);
		openFileD("event_timestamp", "%Lf\n", eves->timestamp);

	}

	debug("report_events: End");
	return EAR_SUCCESS;
}
