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

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <common/config.h>
#include <common/system/monitor.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/math_operations.h>
#include <common/hardware/hardware_info.h>
#include <metrics/accumulators/power_metrics.h>

uint8_t 		power_mon_connected = 0;
rapl_data_t 	*RAPL_metrics;
node_data_t		*Node_metrics;
static uint 	node_units;
static size_t 	node_size;
static uint8_t 	rootp = 0;
static uint8_t 	pm_already_connected = 0;
static uint8_t 	pm_connected_status = 0;
static char 	my_buffer[1024];
static int 		num_packs = 0;
static int 		*pm_fds_rapl;

// GPU
#if USE_GPUS
static gpu_t	*gpu_diff;
static ctx_t	 gpu_context;
static uint		 gpu_sel_model;
static uint		 gpu_num;
#endif

// Things to do
//	1: replace time calls by our common/system/time.
//  2: do not accept any context parameter (they are internal).
//	3: add the word 'node' to the node energy API.
//	4: 'ponitoring' o 'read_enegy'.
//	5: return state_t

/*
 *
 */
/** Computes the difference betwen two RAPL energy measurements */
static rapl_data_t diff_RAPL_energy(rapl_data_t end,rapl_data_t init);

static int pm_get_data_size_rapl()
{
	if (rootp) {
		// Check that 
		if (num_packs == 0) {
			num_packs = detect_packages(NULL);
			if (num_packs == 0) {
				error("Error detecting num packages");
				return EAR_ERROR;
			}
		}
		return RAPL_POWER_EVS * sizeof(rapl_data_t) * num_packs;
	} else {
		return EAR_ERROR;
	}
}

static void pm_disconnect(ehandler_t *my_eh)
{
	if (rootp)
	{
		energy_dispose(my_eh);

		#if USE_GPUS
		if (gpu_num > 0) {
			gpu_dispose(&gpu_context);
		}
		#endif
	}
}

static int pm_read_rapl(ehandler_t *my_eh, rapl_data_t *rm)
{
	if (rootp) {
		return read_rapl_msr(my_eh->fds_rapl, (unsigned long long *) rm);
	} else {
		return EAR_ERROR;
	}
}

static int pm_node_dc_energy(ehandler_t *my_eh, node_data_t *dc)
{
	if (rootp) {
		return energy_dc_read(my_eh, dc);
	} else {
		*dc = 0;
		return EAR_ERROR;
	}
}

static int pm_connect(ehandler_t *my_eh)
{
	int status_enode,i;
	
	if ((pm_already_connected) && (pm_connected_status==EAR_SUCCESS))
	{
		if (state_fail(status_enode = energy_init(NULL, my_eh))) {
			pm_connected_status=status_enode;
			return status_enode;
		}
		/* We reuse RAPL fdds */
		for (i=0;i<num_packs;i++) {
			my_eh->fds_rapl[i]=pm_fds_rapl[i];
		}
		/* Nothing for GPUS */
		return EAR_SUCCESS;
	}

	rootp = (getuid() == 0);
	if (!rootp) {
		return_msg(EAR_ERROR, Generr.no_permissions);
	}
	// Initializing node energy
	debug("Initializing energy in main power_monitor thread");
	pm_connected_status  = energy_init(NULL, my_eh);
	pm_already_connected = 1;

	if (pm_connected_status != EAR_SUCCESS) {
		error("Initializing energy node plugin");
		return pm_connected_status;
	}

	// Getting data size (this could be delegated to the node energy API)
	energy_units(my_eh, &node_units);
	energy_datasize(my_eh, &node_size);

	num_packs = detect_packages(NULL);
	if (num_packs == 0) {
		error("Packages cannot be detected");
		pm_connected_status = EAR_ERROR;
		return EAR_ERROR;
	}
	/* We will use this vector for fd reutilization */
	pm_fds_rapl=calloc(num_packs,sizeof(int));
	if (pm_fds_rapl==NULL){
		error("Allocating memory for RAPL fds in power_metrics");
		return EAR_ERROR;
	}

	// Initializing CPU/DRAM energy
	unsigned long rapl_size;

	debug("%d packages detected in power metrics ", num_packs);
	pm_connected_status = init_rapl_msr(my_eh->fds_rapl);

	if (pm_connected_status != EAR_SUCCESS) {
		error("Error initializing RAPl in pm_connect");
	}

	for (i=0;i<num_packs;i++) pm_fds_rapl[i]=my_eh->fds_rapl[i];
	// Allocating CPU/DRAM energy data
	rapl_size = pm_get_data_size_rapl();
	if (rapl_size == 0) {
		pm_disconnect(my_eh);
		return EAR_ERROR;
	}
	RAPL_metrics = (rapl_data_t *) malloc(rapl_size);
	if (RAPL_metrics == NULL) {
		pm_disconnect(my_eh);
		return EAR_ERROR;
	}
	memset((char *) RAPL_metrics, 0, rapl_size);

	#if USE_GPUS
	int gpu_error = 0;
	state_t s;

	if (xtate_fail(s, gpu_load(empty, none, &gpu_sel_model))) {
		error("gpu_load returned %d (%s)", s, state_msg);
		gpu_error = 1;
	}
	if (xtate_fail(s, gpu_init(&gpu_context))) {
		error("gpu_init returned %d (%s)", s, state_msg);
		gpu_error = 1;
	}
	if (xtate_fail(s, gpu_count(&gpu_context, &gpu_num))) {
		error("gpu_count returned %d (%s)", s, state_msg);
		gpu_error = 1;
	}
	if (xtate_fail(s, gpu_data_alloc(&gpu_diff))) {
		error("gpu_data_alloc returned %d (%s)", s, state_msg);
		gpu_error = 1;
	}
	if (gpu_error) {
		gpu_num = 0;
	}
	#endif

	return pm_connected_status;
}

/*
 *
 */

int init_power_ponitoring(ehandler_t *my_eh)
{
	state_t s;
	debug("init_power_ponitoring");
	if (state_fail(s = pm_connect(my_eh))) {
		return s;
	}
	power_mon_connected = 1;
	return EAR_SUCCESS;
}

void end_power_monitoring(ehandler_t *my_eh)
{
	if (power_mon_connected) {
		pm_disconnect(my_eh);
	}
	power_mon_connected = 0;
}

/*
 *
 */

int read_enegy_data(ehandler_t *my_eh, energy_data_t *acc_energy)
{
	debug("read_enegy_data");
	time(&acc_energy->sample_time);

	if (!power_mon_connected) {
		return EAR_ERROR;
	}
	if (acc_energy == NULL) {
		return EAR_ERROR;
	}

	// Node
	pm_node_dc_energy(my_eh, acc_energy->DC_node_energy);

	// CPU/DRAM
	pm_read_rapl(my_eh, RAPL_metrics);

	memcpy(acc_energy->DRAM_energy,  RAPL_metrics,            num_packs * sizeof(rapl_data_t));
	memcpy(acc_energy->CPU_energy , &RAPL_metrics[num_packs], num_packs * sizeof(rapl_data_t));

	// Debugging data
	#ifdef SHOW_DEBUGS
	int p;
	energy_to_str(my_eh,buffer,acc_energy->DC_node_energy);
	debug("Node %s",buffer);
	for (p = 0; p < num_packs; p++) {
		debug("DRAM pack %d = %llu", p, RAPL_metrics[p]);
	}
	for (p = 0; p < num_packs; p++) {
		debug("CPU pack %d = %llu", p, RAPL_metrics[num_packs + p]);
	}
	#endif

	#if USE_GPUS
	state_t s;

	if (xtate_fail(s, gpu_read(&gpu_context, acc_energy->gpu_data))) {
		error("gpu_read returned %d (%s)", s, state_msg);
	}
	#endif

	return EAR_SUCCESS;
}

void compute_power(energy_data_t *e_begin, energy_data_t *e_end, power_data_t *my_power)
{
	unsigned long curr_node_energy;
	rapl_data_t *dram, *pack;
	double t_diff;
	int p;

	// CPU/DRAM
	dram = (rapl_data_t *) calloc(num_packs, sizeof(rapl_data_t));
	pack = (rapl_data_t *) calloc(num_packs, sizeof(rapl_data_t));

	// Compute the difference
	for (p = 0; p < num_packs; p++) dram[p] = diff_RAPL_energy(e_end->DRAM_energy[p], e_begin->DRAM_energy[p]);
	for (p = 0; p < num_packs; p++) pack[p] = diff_RAPL_energy(e_end->CPU_energy[p] , e_begin->CPU_energy[p]);

	#ifdef SHOW_DEBUGS
	for (p = 0; p < num_packs; p++) debug("energy dram pack %d %llu", p, dram[p]);
	for (p = 0; p < num_packs; p++) debug("energy cpu pack %d %llu" , p, pack[p]);
	#endif

	// eh is not needed here
	energy_accumulated(NULL, &curr_node_energy, e_begin->DC_node_energy, e_end->DC_node_energy);

	t_diff           = difftime(e_end->sample_time, e_begin->sample_time);
	my_power->begin  = e_begin->sample_time;
	my_power->end    = e_end->sample_time;

	my_power->avg_ac = 0;
	my_power->avg_dc = (double) (curr_node_energy) / (t_diff * node_units);

	for (p = 0; p < num_packs; p++) my_power->avg_dram[p] = (double) (dram[p]) / (t_diff * 1000000000);
	for (p = 0; p < num_packs; p++) my_power->avg_cpu[p]  = (double) (pack[p]) / (t_diff * 1000000000);

	#ifdef SHOW_DEBUGS
	for (p = 0; p < num_packs; p++) debug("power dram p = %d %lf", p, my_power->avg_dram[p]);
	for (p = 0; p < num_packs; p++) debug("power pack p = %d %lf", p, my_power->avg_cpu[p]);
	#endif

	free(dram);
	free(pack);

	#if USE_GPUS
	gpu_data_diff(e_end->gpu_data, e_begin->gpu_data, gpu_diff);

	#ifdef SHOW_DEBUGS
	for (p = 0; p < gpu_num  ; p++) {
		debug("energy gpu pack %d %llu", p, gpu_diff[p].energy_j);
	}
	#endif

	for (p = 0; p < gpu_num  ; p++) {
		my_power->avg_gpu[p]  = (gpu_diff[p].power_w);
	}

	#ifdef SHOW_DEBUGS
	for (p = 0; p < gpu_num  ; p++) {
		debug("power gpu p = %d %lf", p, my_power->avg_gpu[p]);
	}
	#endif
	#endif
}

/*
 *
 */

void print_energy_data(energy_data_t *e)
{
	struct tm *current_t;
	char nodee[256];
	char s[64];
	int j;

	// We format the end time into localtime and string
	current_t = localtime(&(e->sample_time));
	strftime(s, sizeof(s), "%c", current_t);

	// Node
	energy_to_str(NULL, nodee, e->DC_node_energy);
	printf("%s: energy (J) for node (%s)", s, nodee);

	//
	printf(", DRAM (");
	for (j = 0; j < num_packs; j++) {
		if (j < (num_packs - 1)) {
			printf("%llu,", e->DRAM_energy[j]);
		} else {
			printf("%llu)", e->DRAM_energy[j]);
		}
	}

	//
	printf(", CPU (");
	for (j = 0; j < num_packs; j++) {
		if (j < (num_packs - 1)) {
			printf("%llu,", e->CPU_energy[j]);
		} else {
			printf("%llu)", e->CPU_energy[j]);
		}
	}

	printf("\n");
}

void print_power(power_data_t *my_power,uint show_date,int out)
{
	double dram_power = 0, pack_power = 0, gpu_power = 0;
	struct tm *current_t;
	char s[64];
	int fd;

	if (out>=0) fd=out;
	else fd=STDERR_FILENO;

	// CPU/DRAM
	dram_power = accum_dram_power(my_power);
	pack_power = accum_cpu_power(my_power);
	#if USE_GPUS
	gpu_power  = accum_gpu_power(my_power);
	#endif

	// We format the end time into localtime and string
	if (show_date){
		current_t = localtime(&(my_power->end));
		strftime(s, sizeof(s), "%c", current_t);

		#if USE_GPUS
		dprintf(fd,"%s: avg. power (W) for node %.2lf, for DRAM %.2lf, for CPU %.2lf, for GPU %.2lf\n",
		   s, my_power->avg_dc, dram_power, pack_power, gpu_power);
		#else
		dprintf(fd,"%s: avg. power (W) for node %.2lf, for DRAM %.2lf, for CPU %.2lf\n",
				s, my_power->avg_dc, dram_power, pack_power);
		#endif
	}else{
		#if USE_GPUS
		dprintf(fd,"avg. power (W) for node %.2lf, for DRAM %.2lf, for CPU %.2lf, for GPU %.2lf\n",
		   my_power->avg_dc, dram_power, pack_power, gpu_power);
		#else
		dprintf(fd,"avg. power (W) for node %.2lf, for DRAM %.2lf, for CPU %.2lf\n",
				my_power->avg_dc, dram_power, pack_power);
		#endif
	}
}

void report_periodic_power(int fd, power_data_t *my_power)
{
	char s[64], spdram[256], spcpu[256], s1dram[64], s1cpu[64], spgpu[256], s1gpu[64];
	double dram_power = 0, pack_power = 0, gpu_power = 0;
	struct tm *current_t;
	int p;

	dram_power = accum_dram_power(my_power);
	pack_power = accum_cpu_power(my_power);

	// We format the end time into localtime and string
	current_t = localtime(&(my_power->end));
	strftime(s, sizeof(s), "%c", current_t);

	//
	sprintf(spdram, ", for DRAM %.2lf (", dram_power);
	for (p = 0; p < num_packs; p++) {
		if (p < (num_packs - 1)) sprintf(s1dram, "%.2lf, ", my_power->avg_dram[p]);
		else sprintf(s1dram, "%.2lf)", my_power->avg_dram[p]);
		strcat(spdram, s1dram);
	}
	//
	sprintf(spcpu, ", for CPU %.2lf (", pack_power);
	for (p = 0; p < num_packs; p++) {
		if (p < (num_packs - 1)) sprintf(s1cpu, "%.2lf, ", my_power->avg_cpu[p]);
		else sprintf(s1cpu, "%.2lf)", my_power->avg_cpu[p]);
		strcat(spcpu, s1cpu);
	}

	#if USE_GPUS
	gpu_power  = accum_gpu_power(my_power);

	spgpu[0] = '\0';

	for (p = 0; p < gpu_num; p++) {
		if (p == 0) sprintf(spgpu, ", for GPU %.2lf (", gpu_power);
		if (p < (gpu_num - 1)) sprintf(s1gpu, "%.2lf, ", my_power->avg_gpu[p]);
		else sprintf(s1gpu, "%.2lf)", my_power->avg_gpu[p]);
		strcat(spgpu, s1gpu);
	}

	sprintf(my_buffer, "%s: avg. power (W) for node %.2lf%s%s%s\n",
			s, my_power->avg_dc, spdram, spcpu, spgpu);
	#else
	sprintf(my_buffer, "%s: avg. power (W) for node %.2lf%s%s\n",
			s, my_power->avg_dc, spdram, spcpu);
	#endif
	write(fd, my_buffer, strlen(my_buffer));
}

/*
 *
 */

rapl_data_t diff_RAPL_energy(rapl_data_t end, rapl_data_t init)
{
	rapl_data_t ret = 0;

	if (end >= init) {
		ret = end - init;
	} else {
		ret = ullong_diff_overflow(init, end);
		//printf("OVERFLOW DETECTED RAPL! %llu,%llu\n",init,end);
	}
	return ret;
}

/*
 * Energy data
 */

void alloc_energy_data(energy_data_t *e)
{
	// Node
	e->DC_node_energy = (edata_t *) malloc(node_size);
	// CPU/DRAM
	e->DRAM_energy = (rapl_data_t *) calloc(num_packs, sizeof(rapl_data_t));
	e->CPU_energy  = (rapl_data_t *) calloc(num_packs, sizeof(rapl_data_t));
	#if USE_GPUS
	state_t s;
	if (xtate_fail(s, gpu_data_alloc(&e->gpu_data))) {
		error("gpu_data_alloc returned %d (%s)", s, state_msg);
	}
	#endif
}

void free_energy_data(energy_data_t *e)
{
	// Node
	free(e->DC_node_energy);
	// CPU/DRAM
	free(e->DRAM_energy);
	free(e->CPU_energy);
	#if USE_GPUS
	state_t s;
	if (xtate_fail(s, gpu_data_free(&e->gpu_data))) {
		error("gpu_data_free returned %d (%s)", s, state_msg);
	}
	#endif
}

void copy_energy_data(energy_data_t *dest, energy_data_t *src)
{
	// Time
	dest->sample_time = src->sample_time;
	// Node
	memcpy(dest->DC_node_energy, src->DC_node_energy, node_size);
	// CPU/DRAM
	memcpy(dest->DRAM_energy, src->DRAM_energy, num_packs * sizeof(rapl_data_t));
	memcpy(dest->CPU_energy , src->CPU_energy , num_packs * sizeof(rapl_data_t));
	#if USE_GPUS
	state_t s;
	if (xtate_fail(s, gpu_data_copy(dest->gpu_data, src->gpu_data))) {
		error("gpu_data_copy returned %d (%s)", s, state_msg);
	}
	#endif
}

void null_energy_data(energy_data_t *acc_energy)
{
	// Time
	time(&acc_energy->sample_time);
	// Node
	memset(acc_energy->DC_node_energy, 0, node_size);
	// CPU/DRAM
	memset(acc_energy->DRAM_energy, 0, num_packs * sizeof(rapl_data_t));
	memset(acc_energy->CPU_energy , 0, num_packs * sizeof(rapl_data_t));
	#if USE_GPUS
	state_t s;
	if (xtate_fail(s, gpu_data_null(acc_energy->gpu_data))) {
		error("gpu_data_null returned %d (%s)", s, state_msg);
	}
	#endif
}

/*
 * Power data
 */

void alloc_power_data(power_data_t *p)
{
	// CPU/DRAM
	p->avg_dram = (double *) calloc(num_packs, sizeof(double));
	p->avg_cpu  = (double *) calloc(num_packs, sizeof(double));
	#if USE_GPUS
	p->avg_gpu  = (double *) calloc(gpu_num  , sizeof(double));
	#endif
}

void free_power_data(power_data_t *p)
{
	// CPU/DRAM
	free(p->avg_dram);
	free(p->avg_cpu);
	#if USE_GPUS
	free(p->avg_gpu);
	#endif
}

void copy_power_data(power_data_t *dest, power_data_t *src)
{
	// Time
	dest->begin  = src->begin;
	dest->end    = src->end;
	// Node
	dest->avg_dc = src->avg_dc;
	dest->avg_ac = src->avg_ac;
	// CPU/DRAM
	memcpy(dest->avg_dram, src->avg_dram, num_packs * sizeof(double));
	memcpy(dest->avg_cpu , src->avg_cpu , num_packs * sizeof(double));
	#if USE_GPUS
	memcpy(dest->avg_gpu , src->avg_gpu , gpu_num   * sizeof(double));
	#endif
}

void null_power_data(power_data_t *p)
{
	// Time
	p->begin  = 0;
	p->end    = 0;
	// Node
	p->avg_dc = 0;
	p->avg_ac = 0;
	// CPU/DRAM
	memset(p->avg_dram, 0, num_packs * sizeof(double));
	memset(p->avg_cpu , 0, num_packs * sizeof(double));
	#if USE_GPUS
	memset(p->avg_gpu , 0, gpu_num   * sizeof(double));
	#endif
}

/*
 * Accum power
 */

double accum_node_power(power_data_t *p)
{
	return p->avg_dc;
}

double accum_dram_power(power_data_t *p)
{
	double dram_power = 0;
	int pid;
	for (pid = 0; pid < num_packs; pid++) {
		debug("accum_dram %d %lf total %lf", pid, p->avg_dram[pid], dram_power);
		dram_power = dram_power + p->avg_dram[pid];
	}
	return dram_power;
}

double accum_cpu_power(power_data_t *p)
{
	double pack_power = 0;
	int pid;
	for (pid = 0; pid < num_packs; pid++) {
		debug("accum_cpu %d %lf total %lf", pid, p->avg_cpu[pid], pack_power);
		pack_power = pack_power + p->avg_cpu[pid];
	}
	return pack_power;
}

double accum_gpu_power(power_data_t *p)
{
	double gpu_power = 0;
	#if USE_GPUS
	int pid;
	for (pid = 0; pid < gpu_num; pid++) {
		debug("accum_gpu %d %lf total %lf", pid, p->avg_gpu[pid], gpu_power);
		gpu_power = gpu_power + p->avg_gpu[pid];
	}
	#endif
	return gpu_power;
}
