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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <papi.h>
// #define CACHE_METRICS 1
#include <common/config.h>
#include <common/states.h>
// #define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/types/signature.h>
#include <common/math_operations.h>
#ifdef CACHE_METRICS
#include <metrics/cache/cache.h>
#endif
#include <common/hardware/hardware_info.h>
#include <library/common/externs.h>
//#include <library/common/global_comm.h>
#include <library/metrics/metrics.h>
#include <metrics/cpi/cpi.h>
#include <metrics/flops/flops.h>
#include <metrics/energy/energy_node_lib.h>
#include <metrics/bandwidth/cpu/utils.h>
#include <daemon/eard_api.h>
#include <common/system/time.h>
extern masters_info_t masters_info;
extern int dispose;
//#define TEST_MB 0

/*
 * Low level reading
 ********************
 *            | P?   | D | Type              | Size F.| Accum/Int. | Unit | Notes
 * ---------- | ---- | - | ----------------- | ------ | ---------- | ---- | ----------------------------
 * Instructs  | PAPI | x | * long long (x2)  | No     | Yes/Yes    | num  | No read (just accum)
 * Cache      | PAPI | x | * long long (x3)  | No     | Yes/Yes    | num  | No read (just accum)
 * Stalls     | PAPI | x | * long long       | No     | Yes/Yes    | num  | No read (just accum)
 * Flops      | PAPI | x | * long long (vec) | Yes    | Yes/Yes    | ops  | No read (just accum)
 * RAPL       | PAPI | v | * ull (vec)       | Yes*   | Yes/Yes**  | nJ   | *Macro, **no daemon service
 * IPMI       | IPMI | v | * ulong           | No     | Yes/No     | mJ   | Water reads J, IBMAEM reads uJ
 * Bandwith   | Cstm | v | * ull (vec)       | Yes    | No/No      | ops  | Vector (r,w,r,w.. so on)
 * Avg. Freq. | Cstm | v | * ulong           | No     | No/Yes*    | MHz? | No start/stop architecture
 *
 * Daemon services
 ******************
 * Client                                         | Req                 | Daemon
 * ---------------------------------------------- | ------------------- | -----------
 * eards_get_data_size_rapl           | DATA_SIZE_RAPL      | eard_rapl
 * eards_start_rapl                   | START_RAPL          |
 * eards_read_rapl                    | READ_RAPL           |
 * eards_reset_rapl                   | RESET_RAPL          |
 * ---------------------------------------------- | ------------------- | ----------------
 * eards_get_data_size_uncore         | DATA_SIZE_UNCORE    | eard_uncore
 * eards_start_uncore                 | START_UNCORE        |
 * eards_read_uncore                  | READ_UNCORE         |
 * eards_reset_uncore                 | RESET_UNCORE        |
 * ---------------------------------------------- | ------------------- | ----------------
 * eards_begin_compute_turbo_freq     | START_GET_FREQ      | eard_freq
 * eards_end_compute_turbo_freq       | END_GET_FREQ        |
 * eards_begin_app_compute_turbo_freq | START_APP_COMP_FREQ |
 * eards_end_app_compute_turbo_freq   | END_APP_COMP_FREQ   |
 * ---------------------------------------------- | ------------------- | ----------------
 * eards_node_dc_energy               | READ_DC_ENERGY      | eard_node_energy
 *
 *
 * Files
 ********
 *            | File                          | Notes
 * ---------- | ----------------------------- | ----------------------------------------------------
 * Basics     | instructions.c                |
 * Cache      | cache.c                       |
 * Cache      | stalls.c                      |
 * Flops      | flops.c                       | Los weights se pueden descuadrar si falla un evento.
 * RAPL       | ear_rapl_metrics.c            | Las posiciones se pueden descuadrar si falla un evento.
 * IPMI       | ear_node_energy_metrics.c     | Tiene una función que devuelve el size, para que?
 * Bandwith   | ear_uncores.c                 | Per Integrated Memory Controller, PCI bus.
 * Avg. Freq. | ear_turbo.c (ear_frequency.c) | Per core, MSR registers. Hay que ver si acumula bien.
 *
 */

// Hardware
static double hw_cache_line_size;

// Options
static int papi_flops_supported;
static int bandwith_elements;
static int flops_elements;
static int rapl_elements;

static ulong node_energy_datasize;
static uint node_energy_units;

// Registers
#define LOO 0 // loop
#define APP 1 // application
#define ACUM 2

#define SIG_BEGIN 	0
#define SIG_END		1

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
#if CACHE_METRICS
static long long metrics_l1[2];
static long long metrics_l2[2];
static long long metrics_l3[2];
#endif
static timestamp init_mpi_time;
static int first_timestamp=1;

static int NI=0;


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
	return PAPI_get_real_usec();
}

static void metrics_global_start()
{
	//
  aux_time = metrics_time();
	if (masters_info.my_master_rank>=0){
		eards_begin_app_compute_turbo_freq();
	// New
  	eards_node_dc_energy(aux_energy,node_energy_datasize);
  	eards_read_rapl(aux_rapl);
		eards_start_uncore();
		eards_read_uncore(metrics_bandwith_init[APP]);
	}else{
		set_null_dc_energy(aux_energy);
		set_null_rapl(aux_rapl);
		set_null_uncores(metrics_bandwith_init[APP]);
	}
	copy_uncores(metrics_bandwith_end[LOO],metrics_bandwith_init[APP],bandwith_elements);
	//eards_start_uncore();
	
  /* We start computing mpi statistics at this point */
	timestamp_getfast(&init_mpi_time);
	sig_shared_region[my_node_id].mpi_info.mpi_time=0;
  sig_shared_region[my_node_id].mpi_info.total_mpi_calls=0;
  sig_shared_region[my_node_id].mpi_info.exec_time=0;

}

static void metrics_global_stop()
{
	timestamp end_mpi_time;
	//
	if (masters_info.my_master_rank>=0){ 
		metrics_avg_frequency[APP] = eards_end_app_compute_turbo_freq();
	}else{
		metrics_avg_frequency[APP]=0;
	}

	// Accum calls
	#if CACHE_METRICS
	get_cache_metrics(&metrics_l1[APP], &metrics_l2[APP], &metrics_l3[APP]);
	#endif
	get_basic_metrics(&metrics_cycles[APP], &metrics_instructions[APP]);
	get_total_fops(metrics_flops[APP]);
	if (masters_info.my_master_rank>=0){
		eards_read_uncore(metrics_bandwith_end[APP]);
	}else{
		set_null_uncores(metrics_bandwith_end[APP]);
	}
	//eards_start_uncore();
	diff_uncores(metrics_bandwith[APP],metrics_bandwith_end[APP],metrics_bandwith_init[APP],bandwith_elements);
	timestamp_getfast(&end_mpi_time);
  unsigned long long extime;
  extime=timestamp_diff(&end_mpi_time,&init_mpi_time,(unsigned long long)1);
  sig_shared_region[my_node_id].mpi_info.exec_time=extime;
	sig_shared_region[my_node_id].mpi_info.perc_mpi=(double)sig_shared_region[my_node_id].mpi_info.mpi_time/(double)sig_shared_region[my_node_id].mpi_info.exec_time;

	
}

/*           | Glob | Part || Start | Stop | Read | Get accum | Reset | Size
* ---------- | ---- | ---- || ----- | ---- | ---- | --------- | ----- | ----
* Instructs. | v    | v    || v     | v    | x    | v         | v     | x
* Cache      | v    | v    || v     | v    | x    | v         | v     | x
* Flops      | v    | v    || v     | v    | x    | v         | v     | x
* RAPL       | v    | v    || v     | v    | x    | x         | x     | v* (daemon)
* IPMI       | v    | v    || x     | x    | v    | x         | x     | v
* Bandwith   | v    | v    || v     | v    | v    | x         | v     | v
* Avg. Freq. | v    | v    || v     | v    | x    | x         | x     | x
*/


static void metrics_partial_start()
{
	int i;
	memcpy(metrics_ipmi[LOO],aux_energy,node_energy_datasize);
	metrics_usecs[LOO]=aux_time;
	
	if (masters_info.my_master_rank>=0){ 
		eards_begin_compute_turbo_freq();
	}
	//There is always a partial_stop before a partial_start, we can guarantee a previous uncore_read
	copy_uncores(metrics_bandwith_init[LOO],metrics_bandwith_end[LOO],bandwith_elements);
	for (i = 0; i < rapl_elements; i++) {
		last_rapl[i]=aux_rapl[i];
	}

	start_basic_metrics();
	#if CACHE_METRICS
	start_cache_metrics();
	#endif
	start_flops_metrics();

	
}

/****************************************************************************************************************************************************************/
/*************************** This function is executed every N seconds to check if signature can be compute *****************************************************/
/****************************************************************************************************************************************************************/

static int metrics_partial_stop(uint where)
{
	long long aux_flops;
	int i;
	ulong c_energy;
	long long c_time;
	float c_power;
	long long aux_time_stop;
	char stop_energy_str[256],start_energy_str[256];

	/* If the signature of the master is not ready, we cannot compute our signature */
  if ((masters_info.my_master_rank<0) && (!sig_shared_region[0].ready)){
			//debug("Master signature not ready at time %lld",metrics_time());
      return EAR_NOT_READY;
  }

	// Manual IPMI accumulation
	if (masters_info.my_master_rank>=0){
		eards_node_dc_energy(aux_energy_stop,node_energy_datasize);
		energy_lib_accumulated(&c_energy,metrics_ipmi[LOO],aux_energy_stop);
		energy_lib_to_str(start_energy_str,metrics_ipmi[LOO]);	
		energy_lib_to_str(stop_energy_str,aux_energy_stop);	
		if ((where==SIG_END) && (c_energy==0) && (masters_info.my_master_rank>=0)){ 
			debug("EAR_NOT_READY because of accumulated energy %lu\n",c_energy);
			if (dispose) verbose(2,"partial stop and EAR_NOT_READY\n");
			return EAR_NOT_READY;
		}
	}
	aux_time_stop = metrics_time();
	/* Sometimes energy is not zero but power is not correct */
	c_time=metrics_usecs_diff(aux_time_stop, metrics_usecs[LOO]);
	/* energy is computed in node_energy_units and time in usecs */
	//debug("Energy computed %lu, time %lld",c_energy,c_time);
	c_power=(float)(c_energy*(1000000.0/(double)node_energy_units))/(float)c_time;

	if (masters_info.my_master_rank>=0){
	if (dispose && ((c_power<0) || (c_power>system_conf->max_sig_power))){
		verbose(1,"dispose and c_power %lf max %lf\n",c_power,system_conf->max_sig_power);	
		verbose(1,"power %f energy %lu time %llu\n",c_power,c_energy,c_time);
	}
	}

	/* If we are not the node master, we will continue */
	if ((where==SIG_END) && (c_power<system_conf->min_sig_power) && (masters_info.my_master_rank>=0)){ 
		debug("EAR_NOT_READY because of power %f\n",c_power);
		return EAR_NOT_READY;
	}


	/* This is new to avoid cases where uncore gets frozen */
	if (masters_info.my_master_rank>=0){
		eards_read_uncore(metrics_bandwith_end[LOO]);
		diff_uncores(diff_uncore_value,metrics_bandwith_end[LOO],metrics_bandwith_init[LOO],bandwith_elements);
		if ((where==SIG_END) && uncore_are_frozen(diff_uncore_value,bandwith_elements)){
			verbose(1,"Doing reset of uncore counters becuase they were frozen");
			eards_reset_uncore();
			return EAR_NOT_READY;
		}else{
			copy_uncores(metrics_bandwith[LOO],diff_uncore_value,bandwith_elements);	
		}
	}
	/* End new section to check frozen uncore counters */
	memcpy(aux_energy,aux_energy_stop,node_energy_datasize);
	aux_time=aux_time_stop;

	if (masters_info.my_master_rank>=0){
		if (c_power<(system_conf->max_sig_power*1.5)){
			acum_ipmi[LOO] = c_energy;
		}else{
			verbose(1,"Computed power was not correct (%lf) reducing it to %lf\n",c_power,system_conf->min_sig_power);
			acum_ipmi[LOO] = system_conf->min_sig_power*c_time;
		}
		acum_ipmi[APP] += acum_ipmi[LOO];
	}
	ulong *ei,*ee;
	ei=(ulong *)metrics_ipmi[LOO];
	ee=(ulong *)aux_energy_stop;
	//debug("loop energy %lu app acum energy %lu (init=%lu - end=%lu)",acum_ipmi[LOO],acum_ipmi[APP],*ei,*ee);
	// Manual time accumulation
	metrics_usecs[LOO] = c_time;
	metrics_usecs[APP] += metrics_usecs[LOO];
	
	// Daemon metrics
	if (masters_info.my_master_rank>=0){ 
		metrics_avg_frequency[LOO] = eards_end_compute_turbo_freq();
	/* This code needs to be adapted to read , compute the difference, and copy begin=end 
 	* diff_uncores(values,values_end,values_begin,num_counters);
 	* copy_uncores(values_begin,values_end,num_counters);
 	*/
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
	#if CACHE_METRICS
	stop_cache_metrics(&metrics_l1[LOO], &metrics_l2[LOO], &metrics_l3[LOO]);
	#endif
	stop_basic_metrics(&metrics_cycles[LOO], &metrics_instructions[LOO]);
	stop_flops_metrics(&aux_flops, metrics_flops[LOO]);


	return EAR_SUCCESS;
}

static void metrics_reset()
{
	// eards_reset_uncore();
	//eards_reset_rapl();

	reset_basic_metrics();
	reset_flops_metrics();
	#if CACHE_METRICS
	reset_cache_metrics();
	#endif
}

ull metrics_vec_inst(signature_t *metrics)
{
	ull VI=0;
	if (papi_flops_supported){
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
}

/******************* This function computes the signature : per loop (global = LOO) or application ( global = APP)  **************/
/******************* The node master has computed the per-node metrics and the rest of processes uses this information ***********/
static void metrics_compute_signature_data(uint global, signature_t *metrics, uint iterations, ulong procs)
{
	double time_s, cas_counter, aux;
	timestamp end_mpi_time;
	int i, s;

	// If global is 1, it means that the global application metrics are required
	// instead the small time metrics for loops. 's' is just a signature index.
	s = global;

	/* PER NODE METRICS */
	metrics->avg_f = metrics_avg_frequency[s];

	// Time
	time_s = (double) metrics_usecs[s] / 1000000.0;

	// Basics
	metrics->time = time_s / (double) iterations;

	#if CACHE_METRICS
	metrics->L1_misses = metrics_l1[s];
	metrics->L2_misses = metrics_l2[s];
	metrics->L3_misses = metrics_l3[s];
	#endif

	// FLOPS
	metrics->Gflops = 0.0;
	if (papi_flops_supported)
	{

		for (i = 0; i < flops_elements; i++) {
			metrics->FLOPS[i] = metrics_flops[s][i] * metrics_flops_weights[i];
			metrics->Gflops += (double) metrics->FLOPS[i];
		}


		metrics->Gflops = metrics->Gflops / time_s; // Floating ops to FLOPS
		metrics->Gflops = metrics->Gflops / 1000000000.0; // FLOPS to GFLOPS
		debug("GFlops %.4lf procs %lu",metrics->Gflops,procs);
		metrics->Gflops = metrics->Gflops * (double) procs; // Core GFLOPS to node GFLOPS
	}
	// Transactions and cycles
	aux = time_s * (double) (1024 * 1024 * 1024);
  cas_counter = 0.0;
  for (i = 0; i < bandwith_elements; ++i) {
    cas_counter += (double) metrics_bandwith[s][i];
  }
	if(masters_info.my_master_rank>=0) lib_shared_region->cas_counters=cas_counter;
	else cas_counter=lib_shared_region->cas_counters;


	// Cycles, instructions and transactions
	metrics->cycles = metrics_cycles[s];
	metrics->instructions = metrics_instructions[s];

	metrics->GBS = cas_counter * hw_cache_line_size / aux;
	metrics->CPI = (double) metrics_cycles[s] / (double) metrics_instructions[s];
	metrics->TPI = cas_counter * hw_cache_line_size / (double) metrics_instructions[s];

	if (masters_info.my_master_rank>=0){
	// Energy node
		metrics->DC_power = (double) acum_ipmi[s] / (time_s * node_energy_units);
		if ((metrics->DC_power > MAX_SIG_POWER) || (metrics->DC_power < MIN_SIG_POWER)){
			debug("Context %d:Warning: Invalid power %.2lf Watts computed in signature : Energy %lu mJ Time %lf msec.\n",s,metrics->DC_power,acum_ipmi[s],time_s* 1000.0);
		}

		int p;
		metrics->PCK_power=0;
		metrics->DRAM_power=0;
		for (p=0;p<num_packs;p++) metrics->DRAM_power=metrics->DRAM_power+(double) metrics_rapl[s][p];
		for (p=0;p<num_packs;p++) metrics->PCK_power=metrics->PCK_power+(double) metrics_rapl[s][num_packs+p];
		metrics->PCK_power   = (metrics->PCK_power / 1000000000.0) / time_s;
		metrics->DRAM_power  = (metrics->DRAM_power / 1000000000.0) / time_s;
	}else{
		copy_node_data(metrics,&sig_shared_region[0].sig);
	}
	metrics->EDP = time_s * time_s * metrics->DC_power;
	/* This part is new to share with other processes */
	timestamp_getfast(&end_mpi_time);
	unsigned long long extime;
	extime=timestamp_diff(&end_mpi_time,&init_mpi_time,(unsigned long long)1);	
	sig_shared_region[my_node_id].mpi_info.exec_time=extime;
  sig_shared_region[my_node_id].mpi_info.perc_mpi=(double)sig_shared_region[my_node_id].mpi_info.mpi_time/(double)sig_shared_region[my_node_id].mpi_info.exec_time;
	signature_copy(&sig_shared_region[my_node_id].sig,metrics);
}

/**************************** Init function used in ear_init ******************/
int metrics_init()
{
	ulong flops_size;
	ulong bandwith_size;
	ulong rapl_size;
	state_t st;

	// Cache line (using custom hardware scanning)
	hw_cache_line_size = (double) get_cache_line_size();
	//debug("detected cache line has a size %0.2lf bytes", hw_cache_line_size);
	num_packs=detect_packages(NULL);
	if (num_packs==0){
		verbose(0,"Error detecting number of packges");
		return EAR_ERROR;
	}

	st=energy_lib_init(system_conf);
	if (st!=EAR_SUCCESS){
		verbose(1,"Error loading energy plugin");
		return EAR_ERROR;
	}


	// Local metrics initialization
	if (init_basic_metrics()!=EAR_SUCCESS) return EAR_ERROR;
	#if CACHE_METRICS
	init_cache_metrics();
	#endif
	papi_flops_supported = init_flops_metrics();

	if (papi_flops_supported==1)
	{
		// Fops size and elements
		flops_elements = get_number_fops_events();
		flops_size = sizeof(long long) * flops_elements;

		// Fops metrics allocation
		metrics_flops[APP] = (long long *) malloc(flops_size);
		metrics_flops[LOO] = (long long *) malloc(flops_size);
		metrics_flops_weights = (int *) malloc(flops_size);

		if (metrics_flops[LOO] == NULL || metrics_flops[APP] == NULL)
		{
			error("error allocating memory, exiting");
			return EAR_ERROR;
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


	if (diff_uncore_value == NULL || metrics_bandwith[LOO] == NULL || metrics_bandwith[APP] == NULL || metrics_bandwith[ACUM] == NULL || metrics_bandwith_init[LOO] == NULL || metrics_bandwith_init[APP] == NULL ||
		metrics_bandwith_end[LOO] == NULL || metrics_bandwith_end[APP] == NULL  ||
			metrics_rapl[LOO] == NULL || metrics_rapl[APP] == NULL || aux_rapl == NULL || last_rapl == NULL)
	{
			verbose(0, "error allocating memory in metrics, exiting");
			return EAR_ERROR;
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


	//debug( "detected %d RAPL counters for %d packages: %d events por package", rapl_elements,num_packs,rapl_elements/num_packs);
	//debug( "detected %d bandwith counter", bandwith_elements);

	metrics_reset();
	metrics_global_start();
	metrics_partial_start();

	return EAR_SUCCESS;
}

void metrics_dispose(signature_t *metrics, ulong procs)
{
	metrics_partial_stop(SIG_BEGIN);
	metrics_global_stop();

	metrics_compute_signature_data(APP, metrics, 1, procs);
}

void metrics_compute_signature_begin()
{
	//
	metrics_partial_stop(SIG_BEGIN);
	metrics_reset();

	//
	metrics_partial_start();
}

int time_ready_signature(ulong min_time_us)
{
	long long aux_time;
	aux_time = metrics_usecs_diff(metrics_time(), metrics_usecs[LOO]);
	if (aux_time<min_time_us) return 0;
	else return 1;
}

/****************************************************************************************************************************************************************/
/******************* This function checks if data is ready to be shared **************************************/
/****************************************************************************************************************************************************************/


int metrics_compute_signature_finish(signature_t *metrics, uint iterations, ulong min_time_us, ulong procs)
{
    long long aux_time;

	NI=iterations;


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

	//
	if (metrics_partial_stop(SIG_END)==EAR_NOT_READY) return EAR_NOT_READY;
	metrics_reset();

	//
	metrics_compute_signature_data(LOO, metrics, iterations, procs);

	//
	metrics_partial_start();

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