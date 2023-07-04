/*
 * File:   examon.h
 */

#ifndef EAR_PUB_H

#define EAR_PUB_H
#define PUB_METRIC(name, function, format) \
    sprintf(tmp_, "%s/%s", sysd_.topic, name); \
    sprintf(data, format, function, sysd_.tmpstr); \
    if(mosquitto_publish(mosq, NULL, tmp_, strlen(data), data, sysd_.qos, false) != MOSQ_ERR_SUCCESS) { \
	    FILE* fp = stderr;\
        fprintf(fp, "[MQTT]: Warning: cannot send message.\n");  \
    } \

#endif


/* data structures */
struct sys_data {
    char logfile[256];
    char* topic;
    char* cmd_topic;
    char* brokerHost;
    int brokerPort;
    int qos;
	char* data_topic_string;
    char* cmd_topic_string;
	float dT;
    char tmpstr[80];
    int **fdd;
    ulong DC_energy;
    ulong job_id;
    time_t start_time;
    time_t end_time;
	char* node_id;
    ulong avg_f;
	ulong avg_DC_power;
    ulong temp;
    ulong DRAM_energy;
	ulong avg_DRAM_power;
    ulong PCK_energy;
	ulong avg_PCK_power;
    ulong GPU_energy;
	ulong avg_GPU_power;
	loop_id_t id;
	ulong jid;
	ulong step_id;
    //char node_id[GENERIC_NAME];
    ulong total_iterations;
    signature_t signature;
	double DC_power;
    double DRAM_power;
    double PCK_power;
    double EDP;
    double GBS;
    double IO_MBS;
    double TPI;
    double CPI;
    double Gflops;
    double time;
    ulong FLOPS[FLOPS_EVENTS];
    ull L1_misses;
    ull L2_misses;
    ull L3_misses;
    ull instructions;
    ull cycles;
    ulong avg_f_loop;
    ulong avg_imc_f;
    ulong def_f;
    double perc_MPI;
    gpu_signature_t gpu_sig;
	int num_gpus;
	int gpu_id;
    gpu_app_t gpu_data[MAX_GPUS_SUPPORTED];
	double GPU_power;
    ulong  GPU_freq;
    ulong  GPU_mem_freq;
    ulong  GPU_util;
    ulong  GPU_mem_util;
    //job_id jid;
    //job_id step_id;
    //char node_id[GENERIC_NAME];
    ulong event;
    ulong freq;
    time_t timestamp;

};
