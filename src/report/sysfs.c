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

typedef unsigned long long	ull;
typedef unsigned long		ulong;

static uint must_report = 0;
char nodename[128];
int island;
char path[1024];
int dir;
int chwd;
char name[128];
double freq_total=0;
double avg_freq_total=0;
double temp_total=0;
double avg_temp_total=0;
int period=1;
ulong DC_energy_total=0;
ulong DRAM_energy_total=0;
ulong PCK_energy_total=0;
ulong GPU_energy_total=0;
FILE *file;
char *env1;
char *env2;


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

int openFile(const char *filename, const char *format, ull value) {
	FILE *file;
	
	file = fopen(filename,"w");
	fprintf(file, format, value);
	fclose(file);
	
	return 0;
}
	
state_t report_init(report_id_t *id,cluster_conf_t *cconf){
	
	debug("report_init: Start");

    if (id->master_rank >= 0) must_report = 1;
    if (!must_report) return EAR_SUCCESS;

    gethostname(nodename, sizeof(nodename));
    strtok(nodename, ".");
	debug("Reporting in node %s", nodename);
	
	island = get_node_island(cconf, nodename);

    debug("report_init: End");
	return EAR_SUCCESS;
}

state_t report_periodic_metrics(report_id_t *id,periodic_metric_t *metric, uint count){
	
	debug("report_periodic_metrics: Start");

    if (!must_report) return EAR_SUCCESS;
    if ((metric == NULL) || (count == 0)) return EAR_SUCCESS;
	
	char *env1 = getenv("METRICS_ROOT_DIR");
	if (env1 != NULL) debug("Using telemetry folder %s", env1);

	char *env2 = getenv("CLUSTER_NAME");
	if (env2 != NULL)  debug("Using cluster name %s", env2);
	
	for (int i=0;i<count;i++){
		
		sprintf(path, "%s/%s/%d/%s/%s/", env1, env2, island, nodename, "avg" );
        
		dir = makeDir(path, 0777);
        chwd = chdir(path);
		
		freq_total = freq_total + metric->avg_f;
		avg_freq_total = freq_total / period;
		//openFile("avg_cpu_freq_KHz", "%.2f\n", avg_freq_total);
		file = fopen("avg_cpu_freq_KHz","w");
		fprintf(file,"%.2f\n", avg_freq_total);
		fclose(file);

		temp_total = temp_total + metric->temp;
		avg_temp_total = temp_total / period;
		//openFile("avg_temp_celsius", "%lu\n", avg_temp_total);
		file = fopen("avg_temp_celsius","w");
		fprintf(file,"%.2f\n", avg_temp_total);
		fclose(file);
		
		period=period+1;
		
		DC_energy_total = DC_energy_total + metric->DC_energy;
		openFile("avg_dc_energy_Joules", "%lu\n", DC_energy_total);
		
		DRAM_energy_total = DRAM_energy_total + metric->DRAM_energy;
		openFile("avg_dram_energy_Joules", "%lu\n", DRAM_energy_total);
		
		PCK_energy_total = PCK_energy_total + metric->PCK_energy;
		openFile("avg_pck_energy_Joules", "%lu\n", PCK_energy_total);

		#if USE_GPUS
		GPU_energy_total = GPU_energy_total + metric->GPU_energy;
		openFile("avg_gpu_energy_Joules", "%lu\n", GPU_energy_total);
		#endif

		openFile("start_time_timestamp", "%lld\n", metric->start_time);
		openFile("end_time_timestamp", "%lld\n", metric->end_time);
		
		
        sprintf(path, "%s/%s/%d/%s/%s/", env1, env2, island, nodename, "current" );

        dir = makeDir(path, 0777);
        chwd = chdir(path);
		
		openFile("temp_celsius", "%lu\n", metric->temp);
		openFile("dc_energy_Joules", "%lu\n", metric->DC_energy);
		openFile("dram_energy_Joules", "%lu\n", metric->DRAM_energy);
		openFile("pck_energy_Joules", "%lu\n", metric->PCK_energy);
		#if USE_GPUS
		openFile("gpu_energy_Joules", "%lu\n", metric->GPU_energy);
		#endif
		openFile("start_time_timestamp", "%lld\n", metric->start_time);
		openFile("end_time_timestamp", "%lld\n", metric->end_time);
		
	}
	
	debug("report_periodic_metrics: End");
	return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id,loop_t *loops, uint count){
	
	debug("report_loops: Start");

    if (!must_report) return EAR_SUCCESS;
    if ((loops == NULL) || (count == 0)) return EAR_SUCCESS;

	env1=getenv("METRICS_ROOT_DIR");
	env2=getenv("CLUSTER_NAME");

	for (int i=0;i<count;i++){
		
		sprintf(path, "%s/%s/%d/%s/%ld/%ld/%s/%s/", env1, env2, island, "jobs", loops->jid, loops->step_id, nodename, "current" );

        dir = makeDir(path, 0777);
        chwd = chdir(path);
		
		openFile("total_iterations", "%lu\n", loops->total_iterations);
		openFile("dc_power_watt", "%lu\n", loops->signature.DC_power);
		openFile("dram_power_watt", "%lu\n", loops->signature.DRAM_power);
		openFile("pck_power_watt", "%lu\n", loops->signature.PCK_power);
		//openFile("edp", "%lu\n", loops->signature.EDP);
		file = fopen("edp","w");
		fprintf(file,"%.3f\n", loops->signature.EDP);
		fclose(file);
		
		//openFile("mem_gbs", "%lu\n", loops->signature.GBS);
		file = fopen("mem_gbs","w");
		fprintf(file,"%f\n", loops->signature.GBS);
		fclose(file);
		
		//openFile("io_mbs", "%lu\n", loops->signature.IO_MBS);
		file = fopen("io_mbs","w");
		fprintf(file,"%f\n", loops->signature.IO_MBS);
		fclose(file);
		
		//openFile("tpi", "%lu\n", loops->signature.TPI);
		file = fopen("tpi","w");
		fprintf(file,"%f\n", loops->signature.TPI);
		fclose(file);
		
		//openFile("cpi", "%lu\n", loops->signature.CPI);
		file = fopen("cpi","w");
		fprintf(file,"%f\n", loops->signature.CPI);
		fclose(file);
		
		//openFile("gflops", "%lu\n", loops->signature.Gflops);
		file = fopen("gflops","w");
		fprintf(file,"%f\n", loops->signature.Gflops);
		fclose(file);
		
		openFile("l1_misses", "%llu\n", loops->signature.L1_misses);
		openFile("l2_misses", "%llu\n", loops->signature.L2_misses);
		openFile("l3_misses", "%llu\n", loops->signature.L3_misses);
		//openFile("instructions", "%llu\n", loops->signature.instructions);
		file = fopen("instructions","w");
		fprintf(file,"%llu\n", loops->signature.instructions);
		fclose(file);
		
		//openFile("cycles", "%llu\n", loops->signature.cycles);
		file = fopen("cycles","w");
		fprintf(file,"%llu\n", loops->signature.cycles);
		fclose(file);
		
		openFile("avg_cpu_freq_KHz", "%lu\n", loops->signature.avg_f);
		openFile("avg_imc_freq_KHz", "%lu\n", loops->signature.avg_imc_f);
		openFile("def_cpu_freq_KHz", "%lu\n", loops->signature.def_f);
		//openFile("perc_mpi_percentage", "%lu\n", loops->signature.perc_MPI);
		file = fopen("perc_mpi_percentage","w");
		fprintf(file,"%f\n", loops->signature.perc_MPI);
		fclose(file);
		
		//openFile("time_timestamp", "%lld\n", loops->signature.time);
		file = fopen("time_timestamp","w");
		fprintf(file,"%f\n", loops->signature.time);
		fclose(file);
		
		for(int j=0;j<8;j++){
			sprintf(name, "%s_%d", "flops", j);
			//openFile(name, "%llu\n", loops->signature.FLOPS[j]);
			file = fopen(name,"w");
			fprintf(file,"%llu\n", loops->signature.FLOPS[j]);
			fclose(file);
		}
		
		#if USE_GPUS
		openFile("gpus_number", "%ld\n", loops->signature.gpu_sig.num_gpus);

        for(int j = 1; j <= loops->signature.gpu_sig.num_gpus; j++){
			
			sprintf(path, "%s/%s/%d/%s/%ld/%ld/%s/%s-%d", env1, env2, island, "jobs", loops->jid, loops->step_id, nodename, "current/GPU", j);

            dir = makeDir(path, 0777);
            chwd = chdir(path);
			
			openFile("gpu_power_watt", "%lu\n", loops->signature.gpu_sig.gpu_data[j].GPU_power);
			openFile("gpu_freq_KHz", "%lu\n", loops->signature.gpu_sig.gpu_data[j].GPU_freq);
			openFile("gpu_mem_freq_KHz", "%lu\n", loops->signature.gpu_sig.gpu_data[j].GPU_mem_freq);
			openFile("gpu_util_percentage", "%lu\n", loops->signature.gpu_sig.gpu_data[j].GPU_util);
			openFile("gpu_mem_util_percentage", "%lu\n", loops->signature.gpu_sig.gpu_data[j].GPU_mem_util);
			
		}
		#endif
		
    }
	
	debug("report_loops: End");
	return EAR_SUCCESS;
}

state_t report_applications(report_id_t *id,application_t *apps, uint count){
	
	debug("report_applications: Start");

    if (!must_report) return EAR_SUCCESS;
    if ((apps == NULL) || (count == 0)) return EAR_SUCCESS;
	
	env1=getenv("METRICS_ROOT_DIR");
	env2=getenv("CLUSTER_NAME");

    for (int i=0;i<count;i++){
		
		sprintf(path, "%s/%s/%d/%s/%ld/%ld/%s/%s/", env1, env2, island, "jobs", apps->job.id, apps->job.step_id, nodename, "avg" );
		
		dir = makeDir(path, 0777);
        chwd = chdir(path);
		
		//openFile("app_job_username", "%s\n", apps->job.user_id);
		//openFile("app_job_group", "%s\n", apps->job.group_id);
		//openFile("app_job_name", "%s\n", apps->job.app_id);
		openFile("app_job_start_time_timestamp", "%lld\n", apps->job.start_time);
		openFile("app_job_end_time_timestamp", "%lld\n", apps->job.end_time);
		openFile("app_job_start_earl_timestamp", "%lld\n", apps->job.start_mpi_time);
		openFile("app_job_end_earl_timestamp", "%lld\n", apps->job.end_mpi_time);
		//openFile("app_job_policy", "%s\n", apps->job.policy);
		openFile("app_job_def_cpu_freq_KHz", "%lu\n", apps->job.def_f);
		openFile("app_dc_power_watt", "%lu\n", apps->power_sig.DC_power);
		openFile("app_dram_power_watt", "%lu\n", apps->power_sig.DRAM_power);
		openFile("app_pck_power_watt", "%lu\n", apps->power_sig.PCK_power);
		//openFile("app_edp", "%lu\n", apps->power_sig.EDP);
		file = fopen("app_edp","w");
		fprintf(file,"%.3f\n", apps->power_sig.EDP);
		fclose(file);
		
		openFile("app_max_dc_power_watt", "%lu\n", apps->power_sig.max_DC_power);
		openFile("app_min_dc_power_watt", "%lu\n", apps->power_sig.min_DC_power);
		openFile("app_avg_cpu_freq_KHz", "%lu\n", apps->power_sig.avg_f);
		openFile("app_def_cpu_freq_KHz", "%lu\n", apps->power_sig.def_f);
		//openFile("app_time_timestamp", "%lu\n", apps->power_sig.time);
		file = fopen("app_time_timestamp","w");
		fprintf(file,"%f\n", apps->power_sig.time);
		fclose(file);
		
		//openFile("app_sig_dc_power_watt", "%lu\n", apps->signature.DC_power);
		file = fopen("app_sig_dc_power_watt","w");
		fprintf(file,"%f\n", apps->signature.DC_power);
		fclose(file);
		
		//openFile("app_sig_dram_power_watt", "%lu\n", apps->signature.DRAM_power);
		file = fopen("app_sig_dram_power_watt","w");
		fprintf(file,"%f\n", apps->signature.DRAM_power);
		fclose(file);
		
		//openFile("app_sig_pck_power_watt", "%lu\n", apps->signature.PCK_power);
		file = fopen("app_sig_pck_power_watt","w");
		fprintf(file,"%f\n", apps->signature.PCK_power);
		fclose(file);
		
		//openFile("app_sig_edp", "%lu\n", apps->signature.EDP);
		file = fopen("app_sig_edp","w");
		fprintf(file,"%f\n", apps->signature.EDP);
		fclose(file);
		
		//openFile("app_sig_mem_gbs", "%lu\n", apps->signature.GBS);
		file = fopen("app_sig_mem_gbs","w");
		fprintf(file,"%f\n", apps->signature.GBS);
		fclose(file);
		
		//openFile("app_sig_io_mbs", "%lu\n", apps->signature.IO_MBS);
		file = fopen("app_sig_io_mbs","w");
		fprintf(file,"%f\n", apps->signature.IO_MBS);
		fclose(file);
		
		//openFile("app_sig_tpi", "%lu\n", apps->signature.TPI);
		file = fopen("app_sig_tpi","w");
		fprintf(file,"%f\n", apps->signature.TPI);
		fclose(file);
		
		//openFile("app_sig_cpi", "%lu\n", apps->signature.CPI);
		file = fopen("app_sig_cpi","w");
		fprintf(file,"%f\n", apps->signature.CPI);
		fclose(file);
		
		//openFile("app_sig_gflops", "%lu\n", apps->signature.Gflops);
		file = fopen("app_sig_gflops","w");
		fprintf(file,"%f\n", apps->signature.Gflops);
		fclose(file);
		
		openFile("app_sig_l1_misses", "%llu\n", apps->signature.L1_misses);
		openFile("app_sig_l2_misses", "%llu\n", apps->signature.L2_misses);
		openFile("app_sig_l3_misses", "%llu\n", apps->signature.L3_misses);
		//openFile("app_sig_instructions", "%llu\n", apps->signature.instructions);
		file = fopen("app_sig_instructions","w");
		fprintf(file,"%llu\n", apps->signature.instructions);
		fclose(file);
		
		//openFile("app_sig_cycles", "%llu\n", apps->signature.cycles);
		file = fopen("app_sig_cycles","w");
		fprintf(file,"%llu\n", apps->signature.cycles);
		fclose(file);
		
		openFile("app_sig_avg_cpu_freq_KHz", "%lu\n", apps->signature.avg_f);
		openFile("app_sig_avg_imc_freq_KHz", "%lu\n", apps->signature.avg_imc_f);
		openFile("app_sig_def_cpu_freq_KHz", "%lu\n", apps->signature.def_f);
		//openFile("app_sig_perc_mpi_percentage", "%lu\n", apps->signature.perc_MPI);
		file = fopen("app_sig_perc_mpi_percentage","w");
		fprintf(file,"%f\n", apps->signature.perc_MPI);
		fclose(file);
		
		//openFile("app_sig_time_timestamp", "%lu\n", apps->signature.time);
		file = fopen("app_sig_time_timestamp","w");
		fprintf(file,"%f\n", apps->signature.time);
		fclose(file);
		
		for(int j=0;j<8;j++){
			sprintf(name, "%s_%d", "app_sig_flops", j);
			//openFile(name, "%lu\n", apps->signature.FLOPS[j]);
			file = fopen(name,"w");
			fprintf(file,"%llu\n", apps->signature.FLOPS[j]);
			fclose(file);
		}
		
		#if USE_GPUS
		openFile("app_sig_gpus_number", "%d\n", apps->signature.gpu_sig.num_gpus);
		
        for(int j = 1; j <= apps->signature.gpu_sig.num_gpus; j++){
			
			sprintf(path, "%s/%s/%d/%s/%ld/%ld/%s/%s-%d", env1, env2, island, "jobs", apps->job.id, apps->job.step_id, nodename, "avg/GPU", j);

            dir = makeDir(path, 0777);
            chwd = chdir(path);
			
			openFile("app_sig_gpu_power_watt", "%lf\n", apps->signature.gpu_sig.gpu_data[j].GPU_power);
			openFile("app_sig_gpu_freq_KHz", "%lu\n", apps->signature.gpu_sig.gpu_data[j].GPU_freq);
			openFile("app_sig_gpu_mem_freq_KHz", "%lu\n", apps->signature.gpu_sig.gpu_data[j].GPU_mem_freq);
			openFile("app_sig_gpu_util_percentage", "%lu\n", apps->signature.gpu_sig.gpu_data[j].GPU_util);
			openFile("app_sig_gpu_mem_util_percentage", "%lu\n", apps->signature.gpu_sig.gpu_data[j].GPU_mem_util);
			
		}
		#endif
		
	}
	
	debug("report_applications: End");
	return EAR_SUCCESS;
}
