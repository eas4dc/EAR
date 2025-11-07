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

// #define SHOW_DEBUGS 1
#include "examon.h"
#include "iniparser.h"
#include "mosquitto.h"
#include <assert.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/types.h>
#include <inttypes.h>
#include <math.h>
#include <report/report.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/***********************************
 *   Publisher Specific Functions  *
 ***********************************/

int init_ear_pub(struct sys_data *sysd)
{

    memset(sysd, 0, sizeof(*sysd));

    strcpy(sysd->logfile, ""); // logfile
    sysd->topic      = NULL;   // topic;
    sysd->cmd_topic  = NULL;   // cmd_topic;
    sysd->brokerHost = NULL;   // brokerHost;
    sysd->brokerPort = 1883;   // brokerPort;
    sysd->qos        = 0;      // qos;
    sysd->dT         = 2.0;    // dT;
    strcpy(sysd->tmpstr, "");  // tmpstr

    return 0;
}

void get_timestamp(char *buf)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    sprintf(buf, "%.3f", tv.tv_sec + (tv.tv_usec / 1000000.0));
}

/****************************
 *     Report Functions     *
 ****************************/

static uint must_report;
struct sys_data sysd_;
char *conffile = "/hpc/opt/ear/etc/ear/examon.conf";
dictionary *ini;
char nodename[128];
char buffer[1024];
int mosqMajor, mosqMinor, mosqRevision;
struct mosquitto *mosq;
char data[255];
char tmp_[255];
int earid = 0;
int island;

state_t report_init(report_id_t *id, cluster_conf_t *cconf)
{

    debug("report_init: start");

    if (id->master_rank >= 0)
        must_report = 1;
    if (!must_report)
        return EAR_SUCCESS;

    gethostname(nodename, sizeof(nodename));
    strtok(nodename, ".");

    // Initiate the publisher
    init_ear_pub(&sysd_);

    // debug("report_init: EAR Publisher Initiated");

    // Parse the conf file
    ini = iniparser_load(conffile);

    sysd_.brokerHost        = iniparser_getstring(ini, "MQTT:brokerHost", NULL);
    sysd_.brokerPort        = iniparser_getint(ini, "MQTT:brokerPort", 1883);
    sysd_.topic             = iniparser_getstring(ini, "MQTT:topic", NULL);
    sysd_.cmd_topic         = iniparser_getstring(ini, "MQTT:cmd_topic", NULL);
    sysd_.qos               = iniparser_getint(ini, "MQTT:qos", 0);
    sysd_.data_topic_string = iniparser_getstring(ini, "MQTT:data_topic_string", NULL);
    sysd_.cmd_topic_string  = iniparser_getstring(ini, "MQTT:cmd_topic_string", NULL);

    island = get_node_island(cconf, nodename);

    // sprintf(buffer, "%s/%s/%d/%s/%s/%s", sysd_.topic, "cluster/mini/island", island, "node", nodename,
    // sysd_.data_topic_string); sysd_.topic = strdup(buffer);
    sprintf(buffer, "%s/%s/%d/%s/%s/%s", sysd_.topic, "cluster/mini/island", island, "node", nodename,
            sysd_.cmd_topic_string);
    sysd_.cmd_topic = strdup(buffer);

    // debug("report_init: Config File Parsed");

    // MQTT
    mosquitto_lib_version(&mosqMajor, &mosqMinor, &mosqRevision);
    mosquitto_lib_init();

    mosq = mosquitto_new(NULL, true, &sysd_);

    mosquitto_subscribe(mosq, NULL, sysd_.cmd_topic, 0);
    mosquitto_connect(mosq, sysd_.brokerHost, sysd_.brokerPort, 1000);
    mosquitto_loop_start(mosq);

    // debug("report_init: Connected to Mosquitto");

    debug("report_init: End");
    return EAR_SUCCESS;
}

state_t report_periodic_metrics(report_id_t *id, periodic_metric_t *metric_list, uint count)
{

    debug("report_periodic_metrics: Start");
    periodic_metric_t *metric;

    if (!must_report)
        return EAR_SUCCESS;
    if ((metric == NULL) || (count == 0))
        return EAR_SUCCESS;

    for (int i = 0; i < count; i++) {

        metric = &metric_list[i];

        // Publish to broker
        sysd_.job_id      = metric->job_id;
        sysd_.start_time  = metric->start_time;
        sysd_.end_time    = metric->end_time;
        sysd_.avg_f       = metric->avg_f * 1000;
        sysd_.temp        = metric->temp;
        sysd_.DC_energy   = metric->DC_energy;
        sysd_.DRAM_energy = metric->DRAM_energy;
        sysd_.PCK_energy  = metric->PCK_energy;
#if USE_GPUS
        sysd_.GPU_energy = metric->GPU_energy;
#endif
        sysd_.node_id = metric->node_id;
        // debug("report_periodic_metrics: some Metrics Collected");

        sysd_.avg_DC_power   = sysd_.DC_energy / (sysd_.end_time - sysd_.start_time);
        sysd_.avg_DRAM_power = sysd_.DRAM_energy / (sysd_.end_time - sysd_.start_time);
        sysd_.avg_PCK_power  = sysd_.PCK_energy / (sysd_.end_time - sysd_.start_time);
#if USE_GPUS
        sysd_.avg_GPU_power = sysd_.GPU_energy / (sysd_.end_time - sysd_.start_time);
#endif
        sprintf(sysd_.tmpstr, "%.3ld", (sysd_.start_time + sysd_.end_time) / 2);
        // debug("report_periodic_metrics: some Metrics Calculated");

        if (sysd_.job_id > 0) {
            sprintf(sysd_.topic, "%s/%d/%s/%s/%s/%ld/%s", "org/bsc/cluster/mini/island", island, "node", sysd_.node_id,
                    "jobid", sysd_.job_id, sysd_.data_topic_string);
        } else {
            sprintf(sysd_.topic, "%s/%d/%s/%s/%s", "org/bsc/cluster/mini/island", island, "node", sysd_.node_id,
                    sysd_.data_topic_string);
        }
        // debug("report_periodic_metrics: Topic Created");

        // debug("report_periodic_metrics: Start Publishing data");
        PUB_METRIC("AVG_FREQ_HZ", sysd_.avg_f, "%lu;%s");
        PUB_METRIC("AVG_DC_POWER_W", sysd_.avg_DC_power, "%lu;%s");
        PUB_METRIC("AVG_DRAM_POWER_W", sysd_.avg_DRAM_power, "%lu;%s");
        PUB_METRIC("AVG_PCK_POWER_W", sysd_.avg_PCK_power, "%lu;%s");
#if USE_GPUS
        PUB_METRIC("AVG_GPU_POWER_W", sysd_.avg_GPU_power, "%lu;%s");
#endif
        // debug("report_periodic_metrics: End publishing Data");
    }

    debug("report_periodic_metrics: End");
    return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id, loop_t *loops_list, uint count)
{

    debug("report_loops: Start");
    loop_t *loops;

    if (!must_report)
        return EAR_SUCCESS;
    if ((loops == NULL) || (count == 0))
        return EAR_SUCCESS;

    for (int i = 0; i < count; i++) {

        loops = &loops_list[i];

        // Publish to broker
        sysd_.jid              = loops->jid;
        sysd_.step_id          = loops->step_id;
        sysd_.total_iterations = loops->total_iterations;
        sysd_.DC_power         = loops->signature.DC_power;
        sysd_.DRAM_power       = loops->signature.DRAM_power;
        sysd_.PCK_power        = loops->signature.PCK_power;
        sysd_.EDP              = loops->signature.EDP;
        sysd_.GBS              = loops->signature.GBS;
        sysd_.IO_MBS           = loops->signature.IO_MBS;
        sysd_.TPI              = loops->signature.TPI;
        sysd_.CPI              = loops->signature.CPI;
        sysd_.Gflops           = loops->signature.Gflops;
        sysd_.time             = loops->signature.time;
        sysd_.L1_misses        = loops->signature.L1_misses;
        sysd_.L2_misses        = loops->signature.L2_misses;
        sysd_.L3_misses        = loops->signature.L3_misses;
        sysd_.instructions     = loops->signature.instructions;
        sysd_.cycles           = loops->signature.cycles;
        sysd_.avg_f_loop       = loops->signature.avg_f * 1000;
        sysd_.avg_imc_f        = loops->signature.avg_imc_f;
        sysd_.def_f            = loops->signature.def_f;
        sysd_.perc_MPI         = loops->signature.perc_MPI;
#if USE_GPUS
        sysd_.num_gpus = loops->signature.gpu_sig.num_gpus;
#endif
        // debug("report_loops: Metrics Collected");

        get_timestamp(sysd_.tmpstr);
        if (sysd_.jid > 0) {
            sprintf(sysd_.topic, "%s/%d/%s/%s/%s/%ld/%s/%ld/%s", "org/bsc/cluster/mini/island", island, "node",
                    sysd_.node_id, "jobid", sysd_.jid, "stepid", sysd_.step_id, sysd_.data_topic_string);
        } else {
            sprintf(sysd_.topic, "%s/%d/%s/%s/%s", "org/bsc/cluster/mini/island", island, "node", sysd_.node_id,
                    sysd_.data_topic_string);
        }
        // debug("report_loops: Topic Created");

        for (int j = 0; j < 8; j++) {
            sysd_.FLOPS[j] = loops->signature.FLOPS[j];
            char *name;
            sprintf(name, "%s_%d", "FLOPS", j);
            PUB_METRIC(name, sysd_.FLOPS[j], "%lu;%s");
        }

        // debug("report_loops: Start Publishing data");
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
        PUB_METRIC("L1_MISSES", sysd_.L1_misses, "%llu;%s");
        PUB_METRIC("L2_MISSES", sysd_.L2_misses, "%llu;%s");
        PUB_METRIC("L3_MISSES", sysd_.L3_misses, "%llu;%s");
        PUB_METRIC("INSTRUCTIONS", sysd_.instructions, "%llu;%s");
        PUB_METRIC("CYCLES", sysd_.cycles, "%llu;%s");
        PUB_METRIC("AVG_CPUFREQ_HZ", sysd_.avg_f_loop, "%lu;%s");
        PUB_METRIC("AVG_IMCFREQ_KHZ", sysd_.avg_imc_f, "%lu;%s");
        PUB_METRIC("DEF_FREQ_KHZ", sysd_.def_f, "%lu;%s");
        PUB_METRIC("PERC_MPI", sysd_.perc_MPI, "%lf;%s");
        // debug("report_loops: End Publishing data");

#if USE_GPUS
        if (sysd_.num_gpus > 0) {
            for (int i = 0; i < sysd_.num_gpus; i++) {
                sysd_.gpu_id = i;
                sprintf(sysd_.topic, "%s/%d/%s/%s/%s/%d/%s/%ld/%s/%ld/%s", "org/bsc/cluster/mini/island", island,
                        "node", sysd_.node_id, "gpu", sysd_.gpu_id, "jobid", sysd_.jid, "stepid", sysd_.step_id,
                        sysd_.data_topic_string);
                // debug("report_loops: Topic Created - GPU Part");

                sysd_.GPU_power    = loops->signature.gpu_sig.gpu_data[i].GPU_power;
                sysd_.GPU_freq     = loops->signature.gpu_sig.gpu_data[i].GPU_freq;
                sysd_.GPU_mem_freq = loops->signature.gpu_sig.gpu_data[i].GPU_mem_freq;
                sysd_.GPU_util     = loops->signature.gpu_sig.gpu_data[i].GPU_util;
                sysd_.GPU_mem_util = loops->signature.gpu_sig.gpu_data[i].GPU_mem_util;
                // debug("report_loops: Metrics Collected - GPU Part");

                // debug("report_loops: Start Publishing data - GPU Part");
                PUB_METRIC("GPU_POWER", sysd_.GPU_power, "%lf;%s");
                PUB_METRIC("GPU_FREQ", sysd_.GPU_freq, "%lu;%s");
                PUB_METRIC("GPU_MEM_FREQ", sysd_.GPU_mem_freq, "%lu;%s");
                PUB_METRIC("GPU_UTIL", sysd_.GPU_util, "%lu;%s");
                PUB_METRIC("GPU_MEM_UTI", sysd_.GPU_mem_util, "%lu;%s");
            }
        }
#endif
    }

    debug("report_loops: End");
    return EAR_SUCCESS;
}

state_t report_events(report_id_t *id, ear_event_t *eves_list, uint count)
{

    debug("report_events: Start");
    ear_event_t *eves;

    if (!must_report)
        return EAR_SUCCESS;
    if ((eves == NULL) || (count == 0))
        return EAR_SUCCESS;

    for (int i = 0; i < count; i++) {

        eves = &eves_list[i];

        sysd_.jid     = eves->jid;
        sysd_.step_id = eves->step_id;
        sysd_.node_id = eves->node_id;
        sysd_.event   = eves->event;
        sysd_.freq    = eves->freq;
        // sysd_.timestamp = eves->timestamp;

        if (sysd_.jid > 0) {
            sprintf(sysd_.topic, "%s/%d/%s/%s/%s/%ld/%s/%ld/%s", "org/bsc/cluster/mini/island", island, "node",
                    sysd_.node_id, "jobid", sysd_.jid, "stepid", sysd_.step_id, sysd_.data_topic_string);
        } else {
            sprintf(sysd_.topic, "%s/%d/%s/%s/%s", "org/bsc/cluster/mini/island", island, "node", sysd_.node_id,
                    sysd_.data_topic_string);
        }

        PUB_METRIC("EVENT", sysd_.event, "%lu;%s");
        PUB_METRIC("VALUE", sysd_.freq, "%lu;%s");
        // PUB_METRIC("TimeStamp", sysd_.timestamp, "%ls;%s");
    }

    debug("report_events: End Publishing data");
    return EAR_SUCCESS;
}

state_t report_dispose(report_id_t *id)
{

    debug("report_dispose: Start");
    if (mosquitto_disconnect(mosq) != MOSQ_ERR_SUCCESS) {
        exit(EXIT_FAILURE);
    }

    mosquitto_destroy(mosq);
    iniparser_freedict(ini);

    debug("report_dispose: End");
    return EAR_SUCCESS;
}
