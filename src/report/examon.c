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
#include "mosquitto.h"
#include "iniparser.h"
#include "examon.h"



/***********************************
 *   Publisher Specific Functions  *
 ***********************************/

int init_ear_pub(struct sys_data * sysd) {

    memset(sysd, 0, sizeof (*sysd));
 
    strcpy(sysd->logfile, ""); // logfile
    sysd->topic = NULL; // topic;
    sysd->cmd_topic = NULL; // cmd_topic;
    sysd->brokerHost = NULL; // brokerHost;
    sysd->brokerPort = 1883; // brokerPort;
    sysd->qos = 0; // qos;
	sysd->dT = 2.0; // dT;
	strcpy(sysd->tmpstr, ""); // tmpstr

    return 0;
}

void get_timestamp(char * buf) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    sprintf(buf, "%.3f", tv.tv_sec + (tv.tv_usec / 1000000.0));
}


/****************************
 *     Report Functions     *
 ****************************/


static uint must_report;
struct sys_data sysd_;
char* conffile = "/hpc/opt/ear/etc/ear/examon.conf";
dictionary *ini;
char nodename[128];
char buffer[1024];
int mosqMajor, mosqMinor, mosqRevision;
struct mosquitto* mosq;
char data[255];
char tmp_[255];
int earid = 0;
int island;
	
state_t report_init(report_id_t *id,cluster_conf_t *cconf){
        
		debug("report_init: start");
        
		
        if (id->master_rank >= 0) must_report = 1;
        if (!must_report) return EAR_SUCCESS;

        gethostname(nodename, sizeof(nodename));
        strtok(nodename, ".");
		
		
		// Initiate the publisher
	    init_ear_pub(&sysd_);
	        
			
	    // Parse the conf file
        ini = iniparser_load(conffile);

        sysd_.brokerHost = iniparser_getstring(ini, "MQTT:brokerHost", NULL);
        sysd_.brokerPort = iniparser_getint(ini, "MQTT:brokerPort", 1883);
        sysd_.topic = iniparser_getstring(ini, "MQTT:topic", NULL);
        sysd_.cmd_topic = iniparser_getstring(ini, "MQTT:cmd_topic", NULL);
        sysd_.qos = iniparser_getint(ini, "MQTT:qos", 0);
		sysd_.data_topic_string = iniparser_getstring(ini, "MQTT:data_topic_string", NULL);
		sysd_.cmd_topic_string = iniparser_getstring(ini, "MQTT:cmd_topic_string", NULL);
		
		
		island = get_node_island(cconf, nodename);
	    
	    
        //sprintf(buffer, "%s/%s/%d/%s/%s/%s", sysd_.topic, "cluster/mini/island", island, "node", nodename, sysd_.data_topic_string);
        //sysd_.topic = strdup(buffer);
        sprintf(buffer, "%s/%s/%d/%s/%s/%s", sysd_.topic, "cluster/mini/island", island, "node", nodename, sysd_.cmd_topic_string);
        sysd_.cmd_topic = strdup(buffer);
		

		// MQTT
        mosquitto_lib_version(&mosqMajor, &mosqMinor, &mosqRevision);
		mosquitto_lib_init();
		
		mosq = mosquitto_new(NULL, true, &sysd_);
		
		mosquitto_subscribe(mosq, NULL, sysd_.cmd_topic, 0);
		mosquitto_connect(mosq, sysd_.brokerHost, sysd_.brokerPort, 1000);
		mosquitto_loop_start(mosq);


        return EAR_SUCCESS;
}

state_t report_periodic_metrics(report_id_t *id,periodic_metric_t *metric, uint count){
        
		debug("report_periodic_metrics: Start");

        if (!must_report) return EAR_SUCCESS;
        if ((metric == NULL) || (count == 0)) return EAR_SUCCESS;
	        
			
	    // Publish to broker
		
		sysd_.job_id = metric->job_id;
		sysd_.start_time = metric->start_time;
		sysd_.end_time = metric->end_time;
		sysd_.avg_f = metric->avg_f;
		sysd_.temp = metric->temp;
		sysd_.DC_energy = metric->DC_energy;
		sysd_.DRAM_energy = metric->DRAM_energy;
		sysd_.PCK_energy = metric->PCK_energy;
#if USE_GPUS
		sysd_.GPU_energy = metric->GPU_energy;
#endif
		sysd_.node_id = metric->node_id;

	
		sysd_.avg_DC_power = sysd_.DC_energy / (sysd_.end_time - sysd_.start_time);
		sysd_.avg_DRAM_power = sysd_.DRAM_energy / (sysd_.end_time - sysd_.start_time);
		sysd_.avg_PCK_power = sysd_.PCK_energy / (sysd_.end_time - sysd_.start_time);
#if USE_GPUS
		sysd_.avg_GPU_power = sysd_.GPU_energy / (sysd_.end_time - sysd_.start_time);
#endif
		sprintf(sysd_.tmpstr, "%.3ld", (sysd_.start_time + sysd_.end_time) / 2);
		
		if (sysd_.job_id > 0){
			sprintf(sysd_.topic, "%s/%d/%s/%s/%s/%ld/%s", "org/bsc/cluster/mini/island", island, "node", sysd_.node_id, "jobid", sysd_.job_id, sysd_.data_topic_string);
		}
		else {
			sprintf(sysd_.topic, "%s/%d/%s/%s/%s", "org/bsc/cluster/mini/island", island, "node", sysd_.node_id, sysd_.data_topic_string);
		}


		PUB_METRIC("AVG_FREQ_KHZ", sysd_.avg_f, "%lu;%s");
		PUB_METRIC("AVG_DC_POWER_W", sysd_.avg_DC_power, "%lu;%s");
		PUB_METRIC("AVG_DRAM_POWER_W", sysd_.avg_DRAM_power, "%lu;%s");
		PUB_METRIC("AVG_PCK_POWER_W", sysd_.avg_PCK_power, "%lu;%s");
#if USE_GPUS
		PUB_METRIC("AVG_GPU_POWER_W", sysd_.avg_GPU_power, "%lu;%s");
#endif
		
		
		debug("publishing");
		
		debug("report_periodic_metrics: End");
		return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id,loop_t *loops, uint count){
	
	debug("report_loops: Start");
    
    if (!must_report) return EAR_SUCCESS;
    if ((loops == NULL) || (count == 0)) return EAR_SUCCESS;
        
    	
    // Publish to broker
	sysd_.jid = loops->jid;
	sysd_.step_id = loops->step_id;
	sysd_.total_iterations = loops->total_iterations;
	sysd_.DC_power = loops->signature.DC_power;
	sysd_.DRAM_power = loops->signature.DRAM_power;
	sysd_.PCK_power = loops->signature.PCK_power;
	sysd_.EDP = loops->signature.EDP;
	sysd_.GBS = loops->signature.GBS;
	sysd_.IO_MBS = loops->signature.IO_MBS;
	sysd_.TPI = loops->signature.TPI;
	sysd_.CPI = loops->signature.CPI;
	sysd_.Gflops = loops->signature.Gflops;
	sysd_.time = loops->signature.time;
	sysd_.FLOPS[FLOPS_EVENTS] = loops->signature.FLOPS[FLOPS_EVENTS];
	sysd_.L1_misses = loops->signature.L1_misses;
	sysd_.L2_misses = loops->signature.L2_misses;
	sysd_.L3_misses = loops->signature.L3_misses;
	sysd_.instructions = loops->signature.instructions;
	sysd_.cycles = loops->signature.cycles;
	sysd_.avg_f_loop = loops->signature.avg_f;
	sysd_.avg_imc_f = loops->signature.avg_imc_f;
	sysd_.def_f = loops->signature.def_f;
	sysd_.perc_MPI = loops->signature.perc_MPI;
	sysd_.num_gpus = loops->signature.gpu_sig.num_gpus;
	
	
	get_timestamp(sysd_.tmpstr);
	if (sysd_.jid > 0){
		sprintf(sysd_.topic, "%s/%d/%s/%s/%s/%ld/%s/%ld/%s", "org/bsc/cluster/mini/island", island, "node", sysd_.node_id, "jobid", sysd_.jid, "stepid", sysd_.step_id, sysd_.data_topic_string);
	}
	else {
		sprintf(sysd_.topic, "%s/%d/%s/%s/%s", "org/bsc/cluster/mini/island", island, "node", sysd_.node_id, sysd_.data_topic_string);
	}
		
	
	PUB_METRIC("ITERATIONS", sysd_.total_iterations, "%lu;%s");
	PUB_METRIC("DC_NODE_POWER_W", sysd_.DC_power, "%lf;%s");
	PUB_METRIC("DRAM_POWER_W", sysd_.DRAM_power, "%lf;%s");
	PUB_METRIC("PCK_POWER_W", sysd_.PCK_power, "%lf;%s");
	PUB_METRIC("EDP", sysd_.EDP, "%lf;%s");
	PUB_METRIC("MEM_GBS", sysd_.GBS, "%lf;%s");
	PUB_METRIC("IO_MBS", sysd_.IO_MBS, "%lf;%s");
	PUB_METRIC("TPI", sysd_.TPI, "%lf;%s");
	PUB_METRIC("CPI", sysd_.CPI, "%lf;%s");
	PUB_METRIC("GFLOPS", sysd_.Gflops, "%lf;%s");
	PUB_METRIC("time", sysd_.time, "%lf;%s");
	PUB_METRIC("FLOPS", sysd_.FLOPS[FLOPS_EVENTS], "%lu;%s");
	PUB_METRIC("L1_MISSES", sysd_.L1_misses, "%llu;%s");
	PUB_METRIC("L2_MISSES", sysd_.L2_misses, "%llu;%s");
	PUB_METRIC("L3_MISSES", sysd_.L3_misses, "%llu;%s");
	PUB_METRIC("INSTRUCTIONS", sysd_.instructions, "%llu;%s");
	PUB_METRIC("CYCLES", sysd_.cycles, "%llu;%s");
	PUB_METRIC("AVG_CPUFREQ_KHZ", sysd_.avg_f_loop, "%lu;%s");
	PUB_METRIC("AVG_IMCFREQ_KHZ", sysd_.avg_imc_f, "%lu;%s");
	PUB_METRIC("DEF_FREQ_KHZ", sysd_.def_f, "%lu;%s");
	PUB_METRIC("PERC_MPI", sysd_.perc_MPI, "%lf;%s");
	
	
	
	if (sysd_.num_gpus > 0){
		
		for(int i = 0; i < sysd_.num_gpus; i++){
			sysd_.gpu_id = i; 
			sprintf(sysd_.topic, "%s/%d/%s/%s/%s/%d/%s/%ld/%s/%ld/%s", "org/bsc/cluster/mini/island", island, "node", sysd_.node_id, "gpu", sysd_.gpu_id, "jobid", sysd_.jid, "stepid", sysd_.step_id, sysd_.data_topic_string);
			
			sysd_.GPU_power = loops->signature.gpu_sig.gpu_data[i].GPU_power;
			sysd_.GPU_freq = loops->signature.gpu_sig.gpu_data[i].GPU_freq;
			sysd_.GPU_mem_freq = loops->signature.gpu_sig.gpu_data[i].GPU_mem_freq;
			sysd_.GPU_util = loops->signature.gpu_sig.gpu_data[i].GPU_util; 
			sysd_.GPU_mem_util = loops->signature.gpu_sig.gpu_data[i].GPU_mem_util;
			
			PUB_METRIC("GPU_POWER", sysd_.GPU_power, "%lf;%s");
			PUB_METRIC("GPU_FREQ", sysd_.GPU_freq, "%lu;%s");
			PUB_METRIC("GPU_MEM_FREQ", sysd_.GPU_mem_freq, "%lu;%s");
			PUB_METRIC("GPU_UTIL", sysd_.GPU_util, "%lu;%s");
			PUB_METRIC("GPU_MEM_UTI", sysd_.GPU_mem_util, "%lu;%s");
		}
	}
	
	debug("publishing");
   
    debug("report_loops: End");
    return EAR_SUCCESS;	
}

state_t report_dispose(report_id_t *id){
	
	debug("report_dispose: Start");
	if (mosquitto_disconnect(mosq) != MOSQ_ERR_SUCCESS) {
        exit(EXIT_FAILURE);
    }
	
    mosquitto_destroy(mosq);
    iniparser_freedict(ini);
	
	
	debug("report_dispose: End");
    
	
	return EAR_SUCCESS;
}
