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

//#define SHOW_DEBUGS 1
#define INFO_METRICS 2
#define IO_VERB 3

#define SET_DYN_UNC_BY_DEFAULT 1
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <library/common/verbose_lib.h>
#include <library/common/library_shared_data.h>
#include <common/types/signature.h>
#include <common/math_operations.h>
#include <common/hardware/hardware_info.h>
#include <library/common/externs.h>
#include <library/api/clasify.h>
#include <library/metrics/metrics.h>
#include <library/metrics/signature_ext.h>
#include <library/policies/common/mpi_stats_support.h> /* Should we move this file out out policies? */
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/cpi/cpi.h>
#include <metrics/flops/flops.h>
#include <metrics/energy/energy_node_lib.h>
#include <metrics/frequency/cpu.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/io/io.h>
#include <metrics/cache/cache.h>
#include <management/imcfreq/imcfreq.h>
#include <daemon/local_api/eard_api.h>
#include <common/system/time.h>
#if USE_GPUS
#include <library/metrics/gpu.h>
char gpu_str[256];
uint ear_num_gpus_in_node=0;
#endif
extern int dispose;
//#define TEST_MB 0


// Hardware
static double hw_cache_line_size;

// Options
static int flops_supported;
static int bandwith_elements;
static int flops_elements;
static int rapl_elements;

static ulong node_energy_datasize;
static uint node_energy_units;

// Registers
#define LOO 0 // loop
#define APP 1 // application
#define ACUM 2
#define LOO_NODE 2 // We will share the same pos than for accumulated values 
#define APP_NODE 3

#define SIG_BEGIN 	0
#define SIG_END		1

/* These vectors have position APP for application granularity and LOO for loop granularity */
static int num_packs=0;
static long long *metrics_flops[2]; // (vec)
static int *metrics_flops_weights; // (vec)
static ull *metrics_bandwith[3]; // ops (vec)
static ull *metrics_bandwith_init[2]; // ops (vec)
static ull *metrics_bandwith_end[2]; // ops (vec)
static ull *diff_uncore_value;
static ull *metrics_rapl[2]; // nJ (vec)
static ull *aux_rapl; // nJ (vec)
static ull *last_rapl; // nJ (vec)
static long long aux_time;
static edata_t aux_energy;
static edata_t metrics_ipmi[2]; // mJ
static ulong   acum_ipmi[2];
static edata_t aux_energy_stop;
static ulong metrics_avg_frequency[2]; // MHz
static long long metrics_instructions[2];
static long long metrics_cycles[2];
static long long metrics_usecs[2]; // uS
/* In the case of IO we will collect per process, per loop and per node */
static io_data_t metrics_io_init[4],metrics_io_end[4],metrics_io_diff[4];
static ctx_t ctx_io;
/* MPI statistics */
mpi_information_t *metrics_mpi_calls[2],*metrics_last_mpi_calls[2];
mpi_calls_types_t *metrics_mpi_calls_types[2],*metrics_last_mpi_calls_types[2];

#if USE_GPUS
/* These vectors have position APP for application granularity and LOO for loop granularity */
static gpu_t *gpu_metrics_init[2],*gpu_metrics_end[2],*gpu_metrics_diff[2];
static ctx_t gpu_lib_ctx;
uint gpu_initialized = 0;
static uint gpu_loop_stopped = 0;
static uint earl_gpu_model = MODEL_UNDEFINED;
#endif
/* These vectors have position APP for application granularity and LOO for loop granularity */
static long long metrics_l1[2];
//static long long metrics_l2[2];
static long long metrics_l3[2];
static state_t s;
static ulong cpufreq_data_avg[2];
static cpufreq_t cpufreq_data_init[2],cpufreq_data_end[2];
static uint mycpufreq_data_size,mycpufreq_data_count;

/* New MEM FREQ mgt API */
uint imc_mgt_compatible = 1;
uint imc_api;
pstate_t *imc_pstates;
uint imc_num_pstates;
uint imc_devices;
ulong def_imc_max_khz,def_imc_min_khz;
uint *imc_max_pstate,*imc_min_pstate;
ulong imc_max_khz,imc_min_khz;
uint imc_curr_max_pstate,imc_curr_min_pstate;
/**/
/* These vectors have position APP for application granularity and LOO for loop granularity */
static imcfreq_t *imc_init[2],*imc_end[2];
static ulong *imc_freq_init[2],*imc_freq_end[2];
static ulong imc_avg[2];

static ctx_t imc_ctx;
static int NI=0;

static llong last_elap_sec = 0;
static llong app_start;

static void metrics_accum_flops(llong *loop_flops);

void set_null_dc_energy(edata_t e)
{
				memset((void *)e,0,node_energy_datasize);
}
void set_null_rapl(ull *erapl)
{
				memset((void*)erapl,0,rapl_elements*sizeof(ull));
}
void set_null_uncores(ull *band)
{
				memset(band,0,bandwith_elements*sizeof(ull));
}
long long metrics_time()
{
				return (llong) timestamp_getconvert(TIME_USECS);
}

static void metrics_global_start()
{
				int ret;
				aux_time = metrics_time();
				app_start = aux_time;

				if (masters_info.my_master_rank>=0)
				{
								/* Avg CPU freq */
								if (xtate_fail(s, eards_cpufreq_read(&cpufreq_data_init[APP],mycpufreq_data_size))){
												error("CPUFreq data read in global_start");
								}
								/* Reads IMC data */
								if (imc_mgt_compatible){
												if (xtate_fail(s, imcfreq_read(&imc_ctx, imc_init[APP]))){
																error("IMCFreq data read in global_start");
												}
								}
								/* Energy */
								ret = eards_node_dc_energy(aux_energy,node_energy_datasize);
								if ((ret == EAR_ERROR) || energy_lib_data_is_null(aux_energy)){
												error("MR[%d] Error reading Node energy at application start",masters_info.my_master_rank);
								}
								/* RAPL energy metrics */
								eards_read_rapl(aux_rapl);
								/* Uncore memory counters to compute Mem bancwith */
								eards_start_uncore();
								eards_read_uncore(metrics_bandwith_init[APP]);

#if USE_GPUS
								/* GPu metrics */
								if (gpu_initialized){
												gpu_lib_data_null(gpu_metrics_init[APP]);
												if (gpu_lib_read(&gpu_lib_ctx,gpu_metrics_init[APP]) != EAR_SUCCESS){
																debug("Error in gpu_read in application start");
												}
												debug("gpu_lib_read in global start");
												gpu_lib_data_copy(gpu_metrics_end[LOO],gpu_metrics_init[APP]);
								}	

#endif
								/* Only the master computes per node metrics */
								metrics_get_total_io(&metrics_io_init[APP_NODE]);
				}else{
								set_null_dc_energy(aux_energy);
								set_null_rapl(aux_rapl);
								set_null_uncores(metrics_bandwith_init[APP]);
				}
				copy_uncores(metrics_bandwith_end[LOO],metrics_bandwith_init[APP],bandwith_elements);
				//eards_start_uncore();

				/* Per process IO data */
				if (io_read(&ctx_io,&metrics_io_init[APP]) != EAR_SUCCESS){
								verbose_master(2,"Warning: IO data not available");
				}



}

static void metrics_global_stop()
{
				char io_info[256];
				char file_name[256];
				char *verb_path=getenv(SCHED_EARL_VERBOSE_PATH);
				long long aux_flops;
				int i;
				//
				imc_avg[APP] = 0;
				metrics_avg_frequency[APP]=0;
				/* Onle the master will collect these metrics */
				if (masters_info.my_master_rank>=0){ 
								/* MPI statistics */
								if (is_mpi_enabled()){
												read_diff_node_mpi_data(lib_shared_region, sig_shared_region, metrics_mpi_calls[APP], metrics_last_mpi_calls[APP]);
												read_diff_node_mpi_type_info(lib_shared_region, sig_shared_region, metrics_mpi_calls_types[APP], metrics_last_mpi_calls_types[APP]);
								}


								/* Avg CPU frequency */
								if (xtate_fail(s, eards_cpufreq_read(&cpufreq_data_end[APP],mycpufreq_data_size))){
												error("CPUFreq data read in global_stop");
								}
								if (xtate_fail(s,cpufreq_data_diff(&cpufreq_data_end[APP],&cpufreq_data_init[APP],NULL,&cpufreq_data_avg[APP]))){
												error("CPUFreq data diff");
								}else{
												debug("CPUFreq average for the application is %.2fGHz",(float)cpufreq_data_avg[APP]/1000000.0);
								}
								metrics_avg_frequency[APP] = cpufreq_data_avg[APP];
								/* AVG IMC frequency */
								if (imc_mgt_compatible){

												if (xtate_fail(s, imcfreq_read(&imc_ctx,imc_end[APP]))){
																error("IMCFreq data read in global_start");
												}
												if (xtate_fail(s, imcfreq_data_diff(imc_end[APP],imc_init[APP],imc_freq_end[APP],&imc_avg[APP]))){
																error("IMC data diff fails");
												}	
												verbose_master(2,"AVG IMC frequency %.2f",(float)imc_avg[APP]/1000000.0);
												for (i=0; i< imc_devices;i++) imc_min_pstate[i] = 0;
												for (i=0; i< imc_devices;i++) imc_max_pstate[i]= imc_num_pstates-1;
												if (state_fail(s = mgt_imcfreq_set_current_ranged_list(NULL,imc_min_pstate,imc_max_pstate))){
																error("Setting the IMC freq in global stop");
												}
												verbose_master(2,"IMC freq restored to %.2f-%.2f",(float)imc_pstates[imc_min_pstate[0]].khz/1000000.0,(float)imc_pstates[imc_max_pstate[0]].khz/1000000.0); 
								}

				}

				/* Cache misses */
				get_cache_metrics(&metrics_l1[APP], &metrics_l3[APP]);
				/* CPI */
				get_basic_metrics(&metrics_cycles[APP], &metrics_instructions[APP]);
				/* FLOPS */
				stop_flops_metrics(&aux_flops, metrics_flops[LOO]);
				metrics_accum_flops(metrics_flops[LOO]);	
				//get_total_fops(metrics_flops[APP]);
				/* Only the master will collect these metrics */
				if (masters_info.my_master_rank>=0){
								/* Uncore counters to compute memory bandwith */
								eards_read_uncore(metrics_bandwith_end[APP]);
#if USE_GPUS
								/* GPU */
								if (gpu_initialized){
												if (gpu_lib_read(&gpu_lib_ctx,gpu_metrics_end[APP])!= EAR_SUCCESS){
																debug("gpu_lib_read error");
												}
												debug("gpu_read in global_stop");
												gpu_lib_data_diff(gpu_metrics_end[APP], gpu_metrics_init[APP], gpu_metrics_diff[APP]);
												gpu_lib_data_tostr(gpu_metrics_diff[APP],gpu_str,sizeof(gpu_str));
												debug("gpu_data in global_stop %s",gpu_str);
								}
#endif
								/* IO node data */
								metrics_get_total_io(&metrics_io_end[APP_NODE]);
								io_diff(&metrics_io_diff[APP_NODE],&metrics_io_init[APP_NODE],&metrics_io_end[APP_NODE]);
				}else{
								set_null_uncores(metrics_bandwith_end[APP]);
				}
				//eards_start_uncore();

				/* Per process IO */
				if (io_read(&ctx_io,&metrics_io_end[APP]) != EAR_SUCCESS){
								verbose_master(2,"Warning: IO data not available");
				}
				io_diff(&metrics_io_diff[APP],&metrics_io_init[APP],&metrics_io_end[APP]);
				if (verb_level >= IO_VERB){
								int fd;
								char *io_buffer;
								io_tostr(&metrics_io_diff[APP],io_info,sizeof(io_info));
								if (verb_path != NULL){ 
												sprintf(file_name, "%s/earl_io_info.%d.%d", verb_path,my_step_id, my_job_id);
												fd = open(file_name,O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP);
								}else fd = 2;
								io_buffer = malloc(strlen(io_info) + 200);
								sprintf(io_buffer,"[%d] APP IO DATA: %s\n",ear_my_rank,io_info);
								write(fd,io_buffer,strlen(io_buffer));
								if (verb_path != NULL) close(fd);
				}


				/* Uncore Memory counters */
				diff_uncores(metrics_bandwith[APP],metrics_bandwith_end[APP],metrics_bandwith_init[APP],bandwith_elements);

}



static void metrics_partial_start()
{
				int i;
				/* This must be replaced by a copy of energy */
				/* Energy node */
				memcpy(metrics_ipmi[LOO],aux_energy,node_energy_datasize);
				/* Time */
				metrics_usecs[LOO]=aux_time;
				last_elap_sec = 0;
				/* Per process IO data */
				if (io_read(&ctx_io,&metrics_io_init[LOO]) != EAR_SUCCESS){
								verbose(0,"Warning:IO data not available");
				}
				/* These data is measured only by the master */	
				if (masters_info.my_master_rank>=0){ 
								/* Avg CPU freq */
								if (xtate_fail(s, eards_cpufreq_read(&cpufreq_data_init[LOO],mycpufreq_data_size))){
												error("CPUFreq data read in partial start");
								}
								/* Avg IMC freq */
								if (imc_mgt_compatible){
												/* Reads the IMC */
												if (xtate_fail(s, imcfreq_read(&imc_ctx,imc_init[LOO]))){
																error("IMCFreq data read in global_start");
												}
								}

								/* Per NODE IO data */
								metrics_get_total_io(&metrics_io_init[LOO_NODE]);

#if USE_GPUS
								/* GPUS */
								if (gpu_initialized){
												if (gpu_loop_stopped) {
																gpu_lib_data_copy(gpu_metrics_init[LOO],gpu_metrics_end[LOO]);
												} else{
																gpu_lib_data_copy(gpu_metrics_init[LOO],gpu_metrics_init[APP]);
												}
												debug("gpu_copy in partial start");
								}
#endif
				}
				//There is always a partial_stop before a partial_start, we can guarantee a previous uncore_read
				/* Uncore counters to compute memory bancdwith */
				copy_uncores(metrics_bandwith_init[LOO],metrics_bandwith_end[LOO],bandwith_elements);
				/* Energy RAPL */
				for (i = 0; i < rapl_elements; i++) {
								last_rapl[i]=aux_rapl[i];
				}

				/* CPI */
				start_basic_metrics();
				/* Cache misses */
				cache_start();
				/* FLOPS */
				start_flops_metrics();


}

/****************************************************************************************************************************************************************/
/*************************** This function is executed every N seconds to check if signature can be compute *****************************************************/
/****************************************************************************************************************************************************************/


void metrics_accum_flops(llong *loop_flops)
{
				uint i;
				llong *app_flops = metrics_flops[APP];
				for (i=0; i<FLOPS_EVENTS; i++) app_flops[i] += loop_flops[i];
}

extern uint ear_iterations;
extern int ear_guided;
static int metrics_partial_stop(uint where)
{
				long long aux_flops;
				int i,ret;
				ulong c_energy;
				long long c_time;
				float c_power;
				long long aux_time_stop;
				char stop_energy_str[256],start_energy_str[256];

				/* If the signature of the master is not ready, we cannot compute our signature */
				//debug("My rank is %d",masters_info.my_master_rank);
				if ((masters_info.my_master_rank<0) && (!sig_shared_region[0].ready)){
								debug("Master signature not ready at time %lld",(metrics_time() - app_start)/1000000);
								debug("Iterations %u ear_guided %u",ear_iterations,ear_guided);
								return EAR_NOT_READY;
				}

				// Manual IPMI accumulation
				if (masters_info.my_master_rank>=0){
								/* Energy node */
								ret = eards_node_dc_energy(aux_energy_stop,node_energy_datasize);
								if (energy_lib_data_is_null(aux_energy_stop) || (ret == EAR_ERROR)){ 
												return EAR_NOT_READY;
								}
								energy_lib_data_accumulated(&c_energy,metrics_ipmi[LOO],aux_energy_stop);
								energy_lib_data_to_str(start_energy_str,metrics_ipmi[LOO]);	
								energy_lib_data_to_str(stop_energy_str,aux_energy_stop);	
								if ((where==SIG_END) && (c_energy==0) && (masters_info.my_master_rank>=0)){ 
												debug("EAR_NOT_READY because of accumulated energy %lu\n",c_energy);
												if (dispose) fprintf(stderr,"partial stop and EAR_NOT_READY\n");
												return EAR_NOT_READY;
								}
				}
				/* Time */
				aux_time_stop = metrics_time();
				/* Sometimes energy is not zero but power is not correct */
				c_time=metrics_usecs_diff(aux_time_stop, metrics_usecs[LOO]);
				/* energy is computed in node_energy_units and time in usecs */
				//debug("Energy computed %lu, time %lld",c_energy,c_time);
				c_power=(float)(c_energy*(1000000.0/(double)node_energy_units))/(float)c_time;

				if (masters_info.my_master_rank>=0){
								if (dispose && ((c_power<0) || (c_power>system_conf->max_sig_power))){
												debug("dispose and c_power %lf\n",c_power);	
												debug("power %f energy %lu time %llu\n",c_power,c_energy,c_time);
								}
				}

				/* If we are not the node master, we will continue */
				if ((where==SIG_END) && (c_power<system_conf->min_sig_power) && (masters_info.my_master_rank>=0)){ 
								debug("EAR_NOT_READY because of power %f\n",c_power);
								return EAR_NOT_READY;
				}

				/* This is new to avoid cases where uncore gets frozen */
				if (masters_info.my_master_rank>=0){
								/* Uncore counters for memory bandwith */
								eards_read_uncore(metrics_bandwith_end[LOO]);
								diff_uncores(diff_uncore_value,metrics_bandwith_end[LOO],metrics_bandwith_init[LOO],bandwith_elements);
								if ((where==SIG_END) && uncore_are_frozen(diff_uncore_value,bandwith_elements)){
												debug("Doing reset of uncore counters becuase they were frozen");
												eards_reset_uncore();
												return EAR_NOT_READY;
								}else{
												copy_uncores(metrics_bandwith[LOO],diff_uncore_value,bandwith_elements);	
								}
#if USE_GPUS
								/* GPUs */
								if (gpu_initialized){
												if (gpu_lib_read(&gpu_lib_ctx,gpu_metrics_end[LOO])!= EAR_SUCCESS){
																debug("gpu_lib_read error");
												}
												debug("gpu_read in partial stop");
												gpu_loop_stopped=1;
												gpu_lib_data_diff(gpu_metrics_end[LOO], gpu_metrics_init[LOO], gpu_metrics_diff[LOO]);
												gpu_lib_data_tostr(gpu_metrics_diff[LOO],gpu_str,sizeof(gpu_str));
												debug("gpu_diff in partial_stop: %s",gpu_str);
								}
#endif
				}
				/* End new section to check frozen uncore counters */
				energy_lib_data_copy(aux_energy,aux_energy_stop);
				aux_time=aux_time_stop;

				if (masters_info.my_master_rank>=0){
								if (c_power < system_conf->max_sig_power){
												acum_ipmi[LOO] = c_energy;
								}else{
												error("Computed power was not correct (%lf) reducing it to %lf\n",c_power,system_conf->min_sig_power);
												acum_ipmi[LOO] = system_conf->min_sig_power*c_time;
								}
								acum_ipmi[APP] += acum_ipmi[LOO];
				}
				/* Per process IO data */
				if (io_read(&ctx_io,&metrics_io_end[LOO]) != EAR_SUCCESS){
								verbose(0,"Warning:IO data not available");
				}
				io_diff(&metrics_io_diff[LOO],&metrics_io_init[LOO],&metrics_io_end[LOO]);


				//  Manual time accumulation
				metrics_usecs[LOO] = c_time;
				metrics_usecs[APP] += metrics_usecs[LOO];

				// Daemon metrics
				if (masters_info.my_master_rank>=0){ 
								/* MPI statistics */
								if (is_mpi_enabled()){
												read_diff_node_mpi_data(lib_shared_region, sig_shared_region, metrics_mpi_calls[LOO], metrics_last_mpi_calls[LOO]);
												read_diff_node_mpi_type_info(lib_shared_region, sig_shared_region, metrics_mpi_calls_types[LOO], metrics_last_mpi_calls_types[LOO]);
								}



								/* Avg CPU freq */
								if (xtate_fail(s, eards_cpufreq_read(&cpufreq_data_end[LOO],mycpufreq_data_size))){
												error("CPUFreq data read in partial stop");
								}
								if (xtate_fail(s,cpufreq_data_diff(&cpufreq_data_end[LOO],&cpufreq_data_init[LOO],cpufreq_data_per_core,&cpufreq_data_avg[LOO]))){
												error("CPUFreq data diff");
								}else{
												debug("CPUFreq average is %.2fGHz",(float)cpufreq_data_avg[LOO]/1000000.0);
								}
								for (int cpu = 0; cpu < mtopo.cpu_count;cpu++) verbosen_master(3,"AVGCPU[%d] = %.2f ",cpu,(float)cpufreq_data_per_core[cpu]/1000000.0);
								verbosen_master(3,"\n");
								metrics_avg_frequency[LOO] = cpufreq_data_avg[LOO];

								/* Avg IMC frequency */
								if (imc_mgt_compatible){
												if (xtate_fail(s, imcfreq_read(&imc_ctx,imc_end[LOO]))){
																error("IMCFreq data read in partial_stop");
												}
												if (xtate_fail(s, imcfreq_data_diff(imc_end[LOO],imc_init[LOO],imc_freq_end[LOO],&imc_avg[LOO]))){
																error("IMC data diff fails");
												}
												debug("AVG IMC frequency %.2f",(float)imc_avg[LOO]/1000000.0);
								}

								/* Per node IO data */
								metrics_get_total_io(&metrics_io_end[LOO_NODE]);
								io_diff(&metrics_io_diff[LOO_NODE],&metrics_io_init[LOO_NODE],&metrics_io_end[LOO_NODE]);



								/* This code needs to be adapted to read , compute the difference, and copy begin=end 
								 * diff_uncores(values,values_end,values_begin,num_counters);
								 * copy_uncores(values_begin,values_end,num_counters);
								 */

								/* RAPL energy */
								eards_read_rapl(aux_rapl);
								//eards_start_uncore();


								// Manual bandwith accumulation: We are also computing it at the end. Should we maintain it? For very long apps maybe this approach is better
								for (i = 0; i < bandwith_elements; i++) {
												metrics_bandwith[ACUM][i] += metrics_bandwith[LOO][i];
								}
								// We read acuumulated energy
								for (i = 0; i < rapl_elements; i++) {
												if (aux_rapl[i] < last_rapl[i])
												{
																metrics_rapl[LOO][i] = ullong_diff_overflow(last_rapl[i], aux_rapl[i]);
												}
												else {
																metrics_rapl[LOO][i]=aux_rapl[i]-last_rapl[i];		
												}
								}

								// Manual RAPL accumulation
								for (i = 0; i < rapl_elements; i++) {
												metrics_rapl[APP][i] += metrics_rapl[LOO][i];
								}
				} /* Metrics collected by node_master*/


				// Local metrics
				/* CPI */
				stop_basic_metrics(&metrics_cycles[LOO], &metrics_instructions[LOO]);
				/* FLOPS */
				stop_flops_metrics(&aux_flops, metrics_flops[LOO]);
				metrics_accum_flops(metrics_flops[LOO]);
				/* Cache misses */
				cache_stop(&metrics_l1[LOO], &metrics_l3[LOO]);

				return EAR_SUCCESS;
}

static void metrics_reset()
{
				// eards_reset_uncore();
				//eards_reset_rapl();

				reset_basic_metrics();
				reset_flops_metrics();

				cache_reset();
}

ull metrics_vec_inst(signature_t *metrics)
{
				ull VI=0;
				if (flops_supported){
								if (metrics->FLOPS[3]>0) VI = metrics->FLOPS[3] / metrics_flops_weights[3];
								if (metrics->FLOPS[7]>0) VI = VI + (metrics->FLOPS[7] / metrics_flops_weights[7]);
				}
				return VI;
}

void copy_node_data(signature_t *dest,signature_t *src)
{
				dest->DC_power=src->DC_power;
				dest->DRAM_power=src->DRAM_power;
				dest->PCK_power=src->PCK_power;
				dest->avg_f=src->avg_f;
#if USE_GPUS
				memcpy(&dest->gpu_sig,&src->gpu_sig,sizeof(gpu_signature_t));
#endif
}

/******************* This function computes the signature : per loop (global = LOO) or application ( global = APP)  **************/
/******************* The node master has computed the per-node metrics and the rest of processes uses this information ***********/
static void metrics_compute_signature_data(uint global, signature_t *metrics, uint iterations, ulong procs)
{
				double time_s, cas_counter, aux;
				sig_ext_t *sig_ext;
				int i, s;

				// If global is 1, it means that the global application metrics are required
				// instead the small time metrics for loops. 's' is just a signature index.
				s = global;

				/* Avg CPU frequency */
				metrics->avg_f = metrics_avg_frequency[s];
				/* Avg IMC frequency */
				metrics->avg_imc_f = imc_avg[s];

				/* Total time */
				time_s = (double) metrics_usecs[s] / 1000000.0;

				/* Time per iteration */
				metrics->time = time_s / (double) iterations;

				/* Cache misses */
				metrics->L1_misses = metrics_l1[s];
				metrics->L2_misses = 0;
				metrics->L3_misses = metrics_l3[s];
				if (metrics->sig_ext == NULL) metrics->sig_ext = (void *)calloc(1,sizeof(sig_ext_t));
				sig_ext = metrics->sig_ext;

				/* FLOPS */
				metrics->Gflops = 0.0;
				if (flops_supported)
				{

								for (i = 0; i < flops_elements; i++) {
												metrics->FLOPS[i] = metrics_flops[s][i] * metrics_flops_weights[i];
												metrics->Gflops += (double) metrics->FLOPS[i];
								}

								if (s == APP) verbose_master(3,"Flops %lf time %lf",metrics->Gflops,time_s);

								metrics->Gflops = metrics->Gflops / time_s; // Floating ops to FLOPS
								metrics->Gflops = metrics->Gflops / 1000000000.0; // FLOPS to GFLOPS
								//metrics->Gflops = metrics->Gflops * (double) procs; // Core GFLOPS to node GFLOPS
				}
				/* TPI and Memory bandwith */
				aux = time_s * (double) (1024 * 1024 * 1024);

				cas_counter = 0.0;
				for (i = 0; i < bandwith_elements; ++i) {
								cas_counter += (double) metrics_bandwith[s][i];
				}

				if(masters_info.my_master_rank>=0) lib_shared_region->cas_counters=cas_counter;
				else cas_counter=lib_shared_region->cas_counters;

				metrics->GBS = cas_counter * hw_cache_line_size / aux;
				metrics->TPI = cas_counter * hw_cache_line_size / (double) metrics_instructions[s];

				/* CPI */
				metrics->CPI = (double) metrics_cycles[s] / (double) metrics_instructions[s];
				metrics->cycles = metrics_cycles[s];
				metrics->instructions = metrics_instructions[s];


				io_copy(&(sig_ext->iod),&metrics_io_diff[s]);
				/* Per process IO data */
				double iogb;
				int ios;
				ullong mpit, exect;
				/* Per Node IO data */
				ios = s;
				if ((masters_info.my_master_rank>=0) && (s == APP)) 								ios = APP_NODE;
				else if ((masters_info.my_master_rank>=0)  && (s == LOO))          	ios = LOO_NODE;
				iogb = (double)metrics_io_diff[ios].rchar/(double)(1024*1024) + (double)metrics_io_diff[ios].wchar/(double)(1024*1024);
				/* Each process will compute its own IO_MBS, except the master, that computes per node IO_MBS */
				metrics->IO_MBS = iogb/time_s;

				if (masters_info.my_master_rank>=0){

								/* MPI statistics */
								if (is_mpi_enabled()){
												double pmpi = 0.0, lpmpi;
												for (int i=0;i<lib_shared_region->num_processes;i++){	
																mpit = ((mpi_information_t *)metrics_mpi_calls[s])[i].mpi_time;
																exect = ((mpi_information_t *)metrics_mpi_calls[s])[i].exec_time;
																if ((metrics_mpi_calls[s])[i].mpi_time > 0) lpmpi = (double)(mpit*100.0 / exect);
																else lpmpi = 0;
																pmpi += lpmpi;
																verbose_master(3,"PerMPI Proc %d %.2lf mpit = %llu exect = %llu",i,lpmpi,mpit,exect);
												}
												metrics->perc_MPI = pmpi/lib_shared_region->num_processes;		
								}else{
												metrics->perc_MPI = 0.0;
								}
								sig_ext->mpi_stats = metrics_mpi_calls[s];
								sig_ext->mpi_types = metrics_mpi_calls_types[s];


								/* Power: Node, DRAM, PCK */
								metrics->DC_power = (double) acum_ipmi[s] / (time_s * node_energy_units);
								metrics->PCK_power=0;
								metrics->DRAM_power=0;
								int p;
								for (p=0;p<num_packs;p++) metrics->DRAM_power=metrics->DRAM_power+(double) metrics_rapl[s][p];
								for (p=0;p<num_packs;p++) metrics->PCK_power=metrics->PCK_power+(double) metrics_rapl[s][num_packs+p];
								metrics->PCK_power   = (metrics->PCK_power / 1000000000.0) / time_s;
								metrics->DRAM_power  = (metrics->DRAM_power / 1000000000.0) / time_s;
								metrics->EDP = time_s * time_s * metrics->DC_power;

								/* GPUS */
#if USE_GPUS
								if (gpu_initialized){
												metrics->gpu_sig.num_gpus = ear_num_gpus_in_node;
												if (earl_gpu_model == MODEL_DUMMY){ 
																debug("Setting num_gpu to 0 because model is DUMMY");
																metrics->gpu_sig.num_gpus = 0;
												}
												for (p=0;p<metrics->gpu_sig.num_gpus;p++){
																metrics->gpu_sig.gpu_data[p].GPU_power    = gpu_metrics_diff[s][p].power_w;
																metrics->gpu_sig.gpu_data[p].GPU_freq     = gpu_metrics_diff[s][p].freq_gpu;
																metrics->gpu_sig.gpu_data[p].GPU_mem_freq = gpu_metrics_diff[s][p].freq_mem;
																metrics->gpu_sig.gpu_data[p].GPU_util     = gpu_metrics_diff[s][p].util_gpu;
																metrics->gpu_sig.gpu_data[p].GPU_mem_util = gpu_metrics_diff[s][p].util_mem;
												}	
								}
#endif
				}else{
								copy_node_data(metrics,&lib_shared_region->master_signature);
				}

				/* Copying my info in the shared signature */
				from_sig_to_mini(&sig_shared_region[my_node_id].sig,metrics);
				/* If I'm the master, I have to copy in the special section */
				if (masters_info.my_master_rank >= 0){
								signature_copy(&lib_shared_region->master_signature,metrics);
				}
				debug("Signature ready node %d at time %lld",my_node_id,(metrics_time() - app_start)/1000000);
				signature_ready(&sig_shared_region[my_node_id],0);
}

/**************************** Init function used in ear_init ******************/
int metrics_init(topology_t *topo)
{
				ulong flops_size;
				ulong bandwith_size;
				ulong rapl_size;
				state_t st;
				char *cimc_max_khz,*cimc_min_khz;
				char imc_buffer[1024];
				pstate_t tmp_def_imc_pstate;
				int i;

				//debug("Masters region %p size %lu",&masters_info,sizeof(masters_info));
				//debug("My master rank %d",masters_info.my_master_rank);

				hw_cache_line_size = topo->cache_line_size;
				num_packs = topo->socket_count;
				topology_copy(&mtopo,topo);

				//debug("detected cache line size: %0.2lf bytes", hw_cache_line_size);
				//debug("detected sockets: %d", num_packs);

				if (hw_cache_line_size == 0) {
								return_msg(EAR_ERROR, "error detecting the cache line size");
				}
				if (num_packs == 0) {
								return_msg(EAR_ERROR, "error detecting number of packges");
				}

				st=energy_lib_init(system_conf);
				if (st!=EAR_SUCCESS){
								return_msg(EAR_ERROR, "error loading energy plugin");
				}

				// Local metrics initialization
				if (init_basic_metrics()!=EAR_SUCCESS) return EAR_ERROR;

				if (state_ok(cache_load(topo))) {
								cache_init();
				}
				if (io_init(&ctx_io,getpid()) != EAR_SUCCESS){
								verbose_master(2,"Warning: IO data not available");
				}else{
								verbose_master(INFO_METRICS,"IO data initialized");
				}	
				flops_supported = init_flops_metrics();

				if (flops_supported==1)
				{
								// Fops size and elements
								flops_elements = get_number_fops_events();
								flops_size = sizeof(long long) * flops_elements;

								// Fops metrics allocation
								metrics_flops[APP] = (long long *) calloc(1,flops_size);
								metrics_flops[LOO] = (long long *) calloc(1,flops_size);
								metrics_flops_weights = (int *) malloc(flops_size);

								if (metrics_flops[LOO] == NULL || metrics_flops[APP] == NULL) {
												return_msg(EAR_ERROR, "error allocating memory, exiting");
								}

								get_weigth_fops_instructions(metrics_flops_weights);
								//debug( "detected %d FLOP counter", flops_elements);
				}

				// Daemon metrics allocation (TODO: standarize data size)
				if (masters_info.my_master_rank>=0){
								rapl_size = eards_get_data_size_rapl();
								bandwith_size = eards_get_data_size_uncore();
				}else{
								rapl_size=sizeof(unsigned long long);
								bandwith_size=sizeof(long long);
				}
				rapl_elements = rapl_size / sizeof(unsigned long long);
				bandwith_elements = bandwith_size / sizeof(long long);

				// Allocating data for energy node metrics
				// node_energy_datasize=eards_node_energy_data_size();
				energy_lib_datasize(&node_energy_datasize);
				energy_lib_units(&node_energy_units);
				/* We should create a data_alloc for enerrgy and a set_null */
				aux_energy=(edata_t)malloc(node_energy_datasize);
				aux_energy_stop=(edata_t)malloc(node_energy_datasize);
				metrics_ipmi[0]=(edata_t)malloc(node_energy_datasize);
				metrics_ipmi[1]=(edata_t)malloc(node_energy_datasize);
				memset(metrics_ipmi[0],0,node_energy_datasize);
				memset(metrics_ipmi[1],0,node_energy_datasize);
				acum_ipmi[0]=0;acum_ipmi[1]=0;

				metrics_bandwith[LOO] = malloc(bandwith_size);
				metrics_bandwith[APP] = malloc(bandwith_size);
				metrics_bandwith[ACUM] = malloc(bandwith_size);
				metrics_bandwith_init[LOO] = malloc(bandwith_size);
				metrics_bandwith_init[APP] = malloc(bandwith_size);
				metrics_bandwith_end[LOO] = malloc(bandwith_size);
				metrics_bandwith_end[APP] = malloc(bandwith_size);
				diff_uncore_value = malloc(bandwith_size);
				metrics_rapl[LOO] = malloc(rapl_size);
				metrics_rapl[APP] = malloc(rapl_size);
				aux_rapl = malloc(rapl_size);
				last_rapl = malloc(rapl_size);

				if (diff_uncore_value         == NULL || metrics_bandwith[LOO]      == NULL || metrics_bandwith[APP]      == NULL ||
												metrics_bandwith[ACUM]    == NULL || metrics_bandwith_init[LOO] == NULL || metrics_bandwith_init[APP] == NULL ||
												metrics_bandwith_end[LOO] == NULL || metrics_bandwith_end[APP]  == NULL || metrics_rapl[LOO]          == NULL ||
												metrics_rapl[APP]         == NULL || aux_rapl                   == NULL || last_rapl                  == NULL)
				{
								return_msg(EAR_ERROR, "error allocating memory in metrics, exiting");
				}

				memset(metrics_bandwith[LOO], 0, bandwith_size);
				memset(metrics_bandwith[APP], 0, bandwith_size);
				memset(metrics_bandwith[ACUM], 0, bandwith_size);
				memset(metrics_bandwith_init[LOO], 0, bandwith_size);
				memset(metrics_bandwith_init[APP], 0, bandwith_size);
				memset(metrics_bandwith_end[LOO], 0, bandwith_size);
				memset(metrics_bandwith_end[APP], 0, bandwith_size);
				memset(diff_uncore_value, 0, bandwith_size);
				memset(metrics_rapl[LOO], 0, rapl_size);
				memset(metrics_rapl[APP], 0, rapl_size);
				memset(aux_rapl, 0, rapl_size);
				memset(last_rapl, 0, rapl_size);
				if (masters_info.my_master_rank>=0){
								/* We will collect MPI statistics */
								metrics_mpi_calls[APP] = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
								metrics_last_mpi_calls[APP] = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
								metrics_mpi_calls[LOO] = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
								metrics_last_mpi_calls[LOO] = calloc(lib_shared_region->num_processes,sizeof(mpi_information_t));
								metrics_mpi_calls_types[APP] = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
								metrics_last_mpi_calls_types[APP] = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
								metrics_mpi_calls_types[LOO] = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));
								metrics_last_mpi_calls_types[LOO] = calloc(lib_shared_region->num_processes,sizeof(mpi_calls_types_t));

								/* CPU average frequency metrics */
								if (xtate_fail(s,cpufreq_load(topo))){
												error("Cpufrequency load failed");
								}
								if (xtate_fail(s,cpufreq_init())){
												error("Cpufrequency init failed");
								}
								if (xtate_fail(s,cpufreq_data_count(&mycpufreq_data_size,&mycpufreq_data_count))){
												error("Cpufrequency data size failed");
								}else{
												verbose_master(2,"Cpufrequency data size %u count %u", mycpufreq_data_size,mycpufreq_data_count);
								}
								if (xtate_fail(s,cpufreq_data_alloc(&cpufreq_data_init[APP],NULL)) || xtate_fail(s,cpufreq_data_alloc(&cpufreq_data_end[APP],NULL)) || xtate_fail(s,cpufreq_data_alloc(&cpufreq_data_init[LOO],NULL)) || xtate_fail(s,cpufreq_data_alloc(&cpufreq_data_end[LOO],NULL))){
												error("Cpufreq allocation failed");
								}
								verbose_master(2,"CPUFreq data allocated");
								cpufreq_data_per_core = (ulong *)calloc(topo->cpu_count,sizeof(ulong));
								/* IMC average frequency initialization */
								if (imc_mgt_compatible){
												/* To compute IMC average */
												imcfreq_load(topo, EARD);
												if (xtate_fail(s, imcfreq_init(&imc_ctx))){
																error("IMC metrics init failed");
												}
												uint imc_api;
												char imc_api_txt[32];
												imcfreq_get_api(&imc_api);
												apis_tostr(imc_api,imc_api_txt,sizeof(imc_api_txt));
												verbose_master(INFO_METRICS,"IMC metrics API %s",imc_api_txt);
												if (xtate_fail(s, imcfreq_data_alloc(&imc_init[APP], &imc_freq_init[APP]))){
																error("IMC data alloc failed");
												}
												if (xtate_fail(s, imcfreq_data_alloc(&imc_end[APP], &imc_freq_end[APP]))){
																error("IMC data alloc failed");
												}
												if (xtate_fail(s, imcfreq_data_alloc(&imc_init[LOO], &imc_freq_init[LOO]) )){
																error("IMC data alloc failed");
												}
												if (xtate_fail(s, imcfreq_data_alloc(&imc_end[LOO], &imc_freq_end[LOO]) )){
																error("IMC data alloc failed");
												}
												verbose_master(INFO_METRICS,"IMC metrics initialized");

												/* IMC management */
												if(state_fail(s = mgt_imcfreq_load(topo, EARD))) {
																error("Error during mgt_imcfreq_load: %s", state_msg);
												}
												if (state_fail(s = mgt_imcfreq_get_api(&imc_api))){
																error("Error in asking dor IMC API");
												}
												char imc_mgt_txt[32];
												apis_tostr(imc_api,imc_mgt_txt,sizeof(imc_mgt_txt));
												verbose_master(INFO_METRICS,"IMC API selected %s",imc_mgt_txt);
												if (state_fail(s = mgt_imcfreq_init(NULL))){
																error("Error in IMC init");
																imc_mgt_compatible = 0;
												}	
												if (state_fail(s = mgt_imcfreq_count_devices(NULL, &imc_devices))){
																error("Error requesting IMC devices ");
												}	
												imc_max_pstate = calloc(imc_devices,sizeof(uint));
												imc_min_pstate = calloc(imc_devices,sizeof(uint));
												if ((imc_max_pstate == NULL) || (imc_min_pstate == NULL)){
																error("Asking for memory");
																return EAR_ERROR;
												}
												verbose_master(INFO_METRICS," IMC num devices %u",imc_devices);
												if (state_fail(s = mgt_imcfreq_get_available_list(NULL,(const pstate_t **)&imc_pstates, &imc_num_pstates))){
																error("Asking for IMC frequency list");
												}
												mgt_imcfreq_data_tostr(imc_pstates,imc_num_pstates,imc_buffer,sizeof(imc_buffer));
												verbose_master(2,"IMC pstates lust:%s",imc_buffer);
												/* Default values */
												def_imc_max_khz = imc_pstates[0].khz;
												def_imc_min_khz = imc_pstates[imc_num_pstates-1].khz;
												for (i=0; i< imc_devices;i++) imc_min_pstate[i] = 0;
												for (i=0; i< imc_devices;i++) imc_max_pstate[i]= imc_num_pstates-1;
												/* This section checks for HACKS */
												cimc_max_khz = getenv(SCHED_MAX_IMC_FREQ);
												cimc_min_khz = getenv(SCHED_MIN_IMC_FREQ);
												imc_max_khz = def_imc_max_khz;
												imc_min_khz = def_imc_min_khz;
												if (cimc_max_khz != (char*)NULL){
																ulong tmp_def_imc = atoi(cimc_max_khz);
																if (state_ok(from_freq_to_pstate(imc_pstates,imc_num_pstates,tmp_def_imc,&tmp_def_imc_pstate))){ 
																				for (i=0; i< imc_devices;i++) imc_min_pstate[i] = tmp_def_imc_pstate.idx;
																				imc_max_khz = imc_pstates[imc_min_pstate[0]].khz;
																}
												}
												if (cimc_min_khz != (char*)NULL){
																ulong tmp_def_imc = atoi(cimc_min_khz);
																if (state_ok(from_freq_to_pstate(imc_pstates,imc_num_pstates,tmp_def_imc,&tmp_def_imc_pstate))){ 
																				for (i=0; i< imc_devices;i++) imc_max_pstate[i] = tmp_def_imc_pstate.idx;
																				imc_min_khz = imc_pstates[imc_max_pstate[0]].khz;
																}
												}
												/* End HACKS */
												if (state_fail(s = mgt_imcfreq_set_current_ranged_list(NULL,imc_min_pstate,imc_max_pstate))){
																error("Initialzing the IMC freq");
												}
												/* We assume all the sockets have the same configuration */
												imc_curr_max_pstate = imc_max_pstate[0];
												imc_curr_min_pstate = imc_min_pstate[0];	
												verbose_master(INFO_METRICS,"New CPUfreq and IMC initialized IMC freq set to %.2f-%.2f",(float)imc_pstates[imc_min_pstate[0]].khz/1000000.0,(float)imc_pstates[imc_max_pstate[0]].khz/1000000.0);

												/* End of IMC */	
								}
				}

#if USE_GPUS
				if (masters_info.my_master_rank>=0){
								if (gpu_lib_load(system_conf) !=EAR_SUCCESS){
												gpu_initialized=0;
												debug("Error in gpu_lib_load");
								}else gpu_initialized=1;
								if (gpu_initialized){
												debug("Initializing GPU");
												if (gpu_lib_init(&gpu_lib_ctx) != EAR_SUCCESS){
																error("Error in GPU initiaization");
																gpu_initialized=0;
												}else{
																debug("GPU initialization successfully");
																gpu_lib_model(&gpu_lib_ctx,&earl_gpu_model);
																debug("GPU model %u",earl_gpu_model);
																gpu_lib_data_alloc(&gpu_metrics_init[LOO]);gpu_lib_data_alloc(&gpu_metrics_init[APP]);
																gpu_lib_data_alloc(&gpu_metrics_end[LOO]);gpu_lib_data_alloc(&gpu_metrics_end[APP]);
																gpu_lib_data_alloc(&gpu_metrics_diff[LOO]);gpu_lib_data_alloc(&gpu_metrics_diff[APP]);
																gpu_lib_count(&ear_num_gpus_in_node);
																debug("GPU library data initialized");
												}
								}
								debug("GPU initialization successfully");
				}
				fflush(stderr);	
#endif

				//debug( "detected %d RAPL counters for %d packages: %d events por package", rapl_elements,num_packs,rapl_elements/num_packs);
				//debug( "detected %d bandwith counter", bandwith_elements);

				metrics_reset();
				metrics_global_start();
				metrics_partial_start();

				return EAR_SUCCESS;
}

void metrics_dispose(signature_t *metrics, ulong procs)
{
				sig_ext_t *se;
				signature_t nodeapp;

				debug("Metrics_dispose_init %d",my_node_id);
				metrics_partial_stop(SIG_BEGIN);
				metrics_global_stop();
        metrics_compute_signature_data(APP, metrics, 1, procs);
        /* Waiting for node signatures */
        verbose_master(2,"Waiting for node signatures");
        while ( !are_signatures_ready(lib_shared_region,sig_shared_region) );

        if (masters_info.my_master_rank >= 0){
                /* Computing flops */
                verbose_master(2,"Computing application node signature");
                se = (sig_ext_t *)metrics->sig_ext;
                metrics_node_signature(metrics, &nodeapp);
                signature_copy(metrics, &nodeapp);
                metrics->sig_ext = se;
        }
#if 0
				signature_ready(&sig_shared_region[my_node_id],0);
				/* Waiting for node signatures */
				verbose_master(2,"Waiting for node signatures");
				while ( !are_signatures_ready(lib_shared_region,sig_shared_region) );

				metrics_compute_signature_data(APP, metrics, 1, procs);
				if (masters_info.my_master_rank >= 0){
								/* Computing flops */
								verbose_master(2,"Computing application node signature");
								se = (sig_ext_t *)metrics->sig_ext;
								metrics_node_signature(metrics, &nodeapp);
								signature_copy(metrics, &nodeapp);
								metrics->sig_ext = se;
				}
#endif
				debug("Metrics_dispose_end %d",my_node_id);

}

void metrics_compute_signature_begin()
{
				//
				metrics_partial_stop(SIG_BEGIN);
				metrics_reset();

				//
				metrics_partial_start();
}

void metrics_get_total_io(io_data_t *total)
{
				int i;
				io_data_t io;
				ctx_t lio;
				if (total == NULL) return;
				memset((char *)total,0,sizeof(io_data_t));
				for (i=0; i<lib_shared_region->num_processes;i++){
								if (io_init(&lio,sig_shared_region[i].pid) != EAR_SUCCESS){
												verbose(2,"IO data for process %d-%d can not be read",i,sig_shared_region[i].pid);
								}else{
												io_read(&lio,&io);
												total->rchar += io.rchar;
												total->wchar += io.wchar;
												total->syscr += io.syscr;
												total->syscw += io.syscw;
												total->read_bytes += io.read_bytes;
												total->write_bytes += io.write_bytes;
												total->cancelled += io.cancelled;
								}
								io_dispose(&lio);
				}
}

int time_ready_signature(ulong min_time_us)
{
				long long f_time;
				f_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);
				if (f_time<min_time_us) return 0;
				else return 1;
}

/****************************************************************************************************************************************************************/
/******************* This function checks if data is ready to be shared **************************************/
/****************************************************************************************************************************************************************/


int metrics_compute_signature_finish(signature_t *metrics, uint iterations, ulong min_time_us, ulong procs)
{
				long long aux_time;

				NI=iterations;

				if (!sig_shared_region[my_node_id].ready){ 

								// Time requirements
								aux_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);

								//
								if ((aux_time<min_time_us) && !equal_with_th((double)aux_time, (double)min_time_us,0.1)) {
#if 0
												verbose(1,"EAR_NOT_READY because of time %llu\n",aux_time);
#endif
												metrics->time=0;

												return EAR_NOT_READY;
								}

								//Master: Returns not ready when power is not ready
								//Not master: when master signature not ready
								if (metrics_partial_stop(SIG_END)==EAR_NOT_READY) return EAR_NOT_READY;

								metrics_reset();

								//Marks the signature as ready
								metrics_compute_signature_data(LOO, metrics, iterations, procs);

								//
								metrics_partial_start();
				}
				if (!are_signatures_ready(lib_shared_region,sig_shared_region)){
								if ((masters_info.my_master_rank >= 0) && (sig_shared_region[0].ready)){
												lib_shared_region->master_ready = 1;
								}
								return EAR_NOT_READY;
				}else if (masters_info.my_master_rank >= 0){
								lib_shared_region->master_ready = 0;
				}
				return EAR_SUCCESS;
}

long long metrics_usecs_diff(long long end, long long init)
{
				long long to_max;

				if (end < init)
				{
								debug( "Timer overflow (end: %lld - init: %lld)\n", end, init);
								to_max = LLONG_MAX - init;
								return (to_max + end);
				}

				return (end - init);
}

void metrics_node_signature(signature_t *master,signature_t *ns)
{
				ullong inst;
				ullong cycles;
				ullong FLOPS[FLOPS_EVENTS];
				sig_ext_t *se;
				int i;

				se = (sig_ext_t *)master->sig_ext;	
				signature_copy(ns,master);
				if (se != NULL) memcpy(ns->sig_ext,se,sizeof(sig_ext_t));

				compute_total_node_instructions(lib_shared_region, sig_shared_region, &inst);
				compute_total_node_flops(lib_shared_region, sig_shared_region, FLOPS);
				compute_total_node_cycles(lib_shared_region, sig_shared_region, &cycles);	
				//compute_total_node_CPI(lib_shared_region, sig_shared_region,&CPI);

				ns->CPI = (double)cycles/(double)inst;
				// verbose_master(2,"CPI %lf CPI2 %lf",ns->CPI, CPI/(double)lib_shared_region->num_processes);
				if (ns->CPI > 2.0){
								for (i=0; i< lib_shared_region->num_processes;i ++){
												verbosen_master(2," CPI[%d]=%lf",i,sig_shared_region[i].sig.CPI);
								}	
								verbose_master(2," ");
				}
				ns->Gflops = compute_node_flops(lib_shared_region, sig_shared_region);
}

extern uint last_earl_phase_classification;
extern uint last_earl_gpu_phase_classification;

state_t metrics_new_iteration(signature_t *sig)
{
				llong sflops;
				llong f_time, elap_sec;
				llong FLOPS[FLOPS_EVENTS];
				int i;
#if USE_GPUS
				gpu_t *gpud, *gpuddiff;
				int p;
#endif
				if (masters_info.my_master_rank < 0 ) return EAR_ERROR;
				/* FLOPS are used to determine the CPU BUSY waiting */
				if ((last_earl_phase_classification == APP_IO_BOUND) || (last_earl_phase_classification == APP_BUSY_WAITING)){
								f_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);
								elap_sec = f_time / 1000000;
								debug("elap %lld last %lld: Computing flops because of application phase is IO =%d or Busy waiting=%d",elap_sec,last_elap_sec, last_earl_phase_classification == APP_IO_BOUND, last_earl_phase_classification == APP_BUSY_WAITING);

								if (elap_sec <= last_elap_sec) return EAR_ERROR;
								last_elap_sec = elap_sec;
								read_flops_metrics(&sflops, FLOPS);
								sig->Gflops = 0;
								for (i= 0; i< FLOPS_EVENTS; i++){
												sig->FLOPS[i] = (ullong)FLOPS[i] * metrics_flops_weights[i];
												sig->Gflops += (double) sig->FLOPS[i];
								}
								sig->Gflops = sig->Gflops /(elap_sec *1000000000.0);
								sig->time = elap_sec;
								debug(" Flops in metrics for validation %lf time %lld",sig->Gflops, elap_sec);
				}

#if USE_GPUS
				if ((last_earl_gpu_phase_classification == GPU_IDLE) && (ear_num_gpus_in_node > 0)){
								debug("Computing GPU util because of application gpu phase is IDLE = %d",last_earl_gpu_phase_classification == GPU_IDLE);
								/* GPUs */
								if (gpu_initialized){
												gpu_lib_data_alloc(&gpud);
												gpu_lib_data_alloc(&gpuddiff);
												if (gpu_lib_read(&gpu_lib_ctx,gpud) != EAR_SUCCESS){
																debug("gpu_lib_read error");
												}
												gpu_lib_data_diff(gpud, gpu_metrics_init[LOO], gpuddiff);
												sig->gpu_sig.num_gpus = ear_num_gpus_in_node;
												if (earl_gpu_model == MODEL_DUMMY){
																debug("Setting num_gpu to 0 because model is DUMMY");
																sig->gpu_sig.num_gpus = 0;
												}
												for (p=0; p < sig->gpu_sig.num_gpus;p++){
																sig->gpu_sig.gpu_data[p].GPU_power    = gpuddiff[p].power_w;
																sig->gpu_sig.gpu_data[p].GPU_freq     = gpuddiff[p].freq_gpu;
																sig->gpu_sig.gpu_data[p].GPU_mem_freq = gpuddiff[p].freq_mem;
																sig->gpu_sig.gpu_data[p].GPU_util     = gpuddiff[p].util_gpu;
																sig->gpu_sig.gpu_data[p].GPU_mem_util = gpuddiff[p].util_mem;
												}
												gpu_lib_data_free(&gpud);
												gpu_lib_data_free(&gpuddiff);

								}
				}
#endif	
				return EAR_SUCCESS;
}
