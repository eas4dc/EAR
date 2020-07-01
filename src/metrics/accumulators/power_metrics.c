/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
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
#include <common/states.h>
//#define SHOW_DEBUGS 0
#include <common/output/verbose.h>
#include <common/math_operations.h>
#include <common/hardware/hardware_info.h>
#include <metrics/energy/energy_cpu.h>
#include <metrics/energy/energy_gpu.h>
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
gpu_energy_t *gpu_data;
pcontext_t gpu_context;
uint       gpu_loop_ms;
uint       gpu_init;
uint       gpu_num;

// Things to do
//	1: replace time calls by our common/system/time.
//  2: do not accept any context parameter (they are internal).
//	3: add the word 'node' to the node energy API.
//	4: 'ponitoring' o 'read_enegy'.
//	5: return state_t

/*
 *
 */

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

		if (gpu_init == 1) {
			energy_gpu_dispose(&gpu_context);
		}
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
	if ((pm_already_connected) && (pm_connected_status==EAR_SUCCESS)){
		status_enode=energy_init(NULL, my_eh);
		if (status_enode!=EAR_SUCCESS){ 
			pm_connected_status=status_enode;
			return status_enode;
		}
		/* We reuse RAPL fdds */
		for (i=0;i<num_packs;i++) my_eh->fds_rapl[i]=pm_fds_rapl[i];
		/* Nothing for GPUS */
		return EAR_SUCCESS;
	}

	rootp = (getuid() == 0);

	if (!rootp) {
		return EAR_ERROR;
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

	// Initializing GPU energy
	state_t s = energy_gpu_init(&gpu_context, gpu_loop_ms);

	if (state_ok(s))
	{
		// Allocating GPU energy data
		s = energy_gpu_count(&gpu_context, &gpu_num);
		s = energy_gpu_data_alloc(&gpu_context, &gpu_data);
		
		gpu_init = state_ok(s) && gpu_num > 0;
	}

	if (gpu_init == 0) {
		error("GPU initialization APIs failed (%s)", intern_error_str);
	}

	return pm_connected_status;
}

/*
 *
 */

int init_power_ponitoring(ehandler_t *my_eh)
{
	int ret;
	debug("init_power_ponitoring");
	ret = pm_connect(my_eh);
	if (ret != EAR_SUCCESS) {
		error("Error in init_power_ponitoring");
		return EAR_ERROR;
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
	int p;
	char buffer[512];

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

	// GPU
	energy_gpu_read(&gpu_context, gpu_data);

	for (p = 0; p < gpu_num; ++p) {
		acc_energy->GPU_energy[p] = (ulong) gpu_data[p].energy_j;
	}

	// Debugging data
	#ifdef SHOW_DEBUGS
	energy_to_str(my_eh,buffer,acc_energy->DC_node_energy);
	debug("Node %s",buffer);
	for (p = 0; p < num_packs; p++) {
		debug("DRAM pack %d = %llu", p, RAPL_metrics[p]);
	}
	for (p = 0; p < num_packs; p++) {
		debug("CPU pack %d = %llu", p, RAPL_metrics[num_packs + p]);
	}
	for (p = 0; p < gpu_num  ; p++) {
		debug("GPU pack %d = %lu", p, (ulong) gpu_data[p].energy_j);
	}
	#endif

	return EAR_SUCCESS;
}

void compute_power(energy_data_t *e_begin, energy_data_t *e_end, power_data_t *my_power)
{
	unsigned long curr_node_energy;
	rapl_data_t *dram, *pack;
	ulong *gpus;
	double t_diff;
	int p;

	// CPU/DRAM
	dram = (rapl_data_t *) calloc(num_packs, sizeof(rapl_data_t));
	pack = (rapl_data_t *) calloc(num_packs, sizeof(rapl_data_t));
	gpus = (ulong *) calloc(gpu_num, sizeof(ulong));

	// Compute the difference
	for (p = 0; p < num_packs; p++) dram[p] = diff_RAPL_energy(e_end->DRAM_energy[p], e_begin->DRAM_energy[p]);
	for (p = 0; p < num_packs; p++) pack[p] = diff_RAPL_energy(e_end->CPU_energy[p] , e_begin->CPU_energy[p]);
	for (p = 0; p < gpu_num  ; p++) gpus[p] = e_end->GPU_energy[p] - e_begin->GPU_energy[p];

	#ifdef SHOW_DEBUGS
	for (p = 0; p < num_packs; p++) debug("energy dram pack %d %llu", p, dram[p]);
	for (p = 0; p < num_packs; p++) debug("energy cpu pack %d %llu" , p, pack[p]);
	for (p = 0; p < gpu_num  ; p++) debug("energy gpu pack %d %llu" , p, gpus[p]);
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
	for (p = 0; p < gpu_num  ; p++) my_power->avg_gpu[p]  = (double) (gpus[p]) / (t_diff);

	#ifdef SHOW_DEBUGS
	for (p = 0; p < num_packs; p++) debug("power dram p = %d %lf", p, my_power->avg_dram[p]);
	for (p = 0; p < num_packs; p++) debug("power pack p = %d %lf", p, my_power->avg_cpu[p]);
	for (p = 0; p < gpu_num  ; p++) debug("power gpu  p = %d %lf", p, my_power->avg_gpu[p]);
	#endif

	free(dram);
	free(pack);
	free(gpus);
}

/*
 *
 */

void print_energy_data(energy_data_t *e)
{
	struct tm *current_t;
	char nodee[256];
	char s[64];
	int i, j;

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
	
	//
	for (j = 0; j < gpu_num; j++) {
		if (j == 0) {
			printf(", GPU (");
		} if (j < (gpu_num - 1)) {
			printf("%llu,", e->GPU_energy[j]);
		} else {
			printf("%llu)", e->GPU_energy[j]);
		}
	}

	printf("\n");
}

void print_power(power_data_t *my_power)
{
	double dram_power = 0, pack_power = 0, gpu_power = 0;
	struct tm *current_t;
	char s[64];
	int p;

	// CPU/DRAM
	dram_power = accum_dram_power(my_power);
	pack_power = accum_cpu_power(my_power);
	gpu_power  = accum_gpu_power(my_power);

	// We format the end time into localtime and string
	current_t = localtime(&(my_power->end));
	strftime(s, sizeof(s), "%c", current_t);

	printf("%s: avg. power (W) for node %.2lf, for DRAM %.2lf, for CPU %.2lf, for GPU %.2lf\n",
		   s, my_power->avg_dc, dram_power, pack_power, gpu_power);
}

void report_periodic_power(int fd, power_data_t *my_power)
{
	char s[64], spdram[256], spcpu[256], s1dram[64], s1cpu[64], spgpu[256], s1gpu[64];
	double dram_power = 0, pack_power = 0, gpu_power = 0;
	struct tm *current_t;
	int p, pp;

	dram_power = accum_dram_power(my_power);
	pack_power = accum_cpu_power(my_power);
	gpu_power  = accum_gpu_power(my_power);

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
	//
	spgpu[0] = '\0';
	for (p = 0; p < gpu_num; p++) {
		if (p == 0) sprintf(spgpu, ", for GPU %.2lf (", gpu_power);
		if (p < (gpu_num - 1)) sprintf(s1gpu, "%.2lf, ", my_power->avg_gpu[p]);
		else sprintf(s1gpu, "%.2lf)", my_power->avg_gpu[p]);
		strcat(spgpu, s1gpu);
	}

	sprintf(my_buffer, "%s: avg. power (W) for node %.2lf%s%s%s\n",
			s, my_power->avg_dc, spdram, spcpu, spgpu);
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
	// GPU
	e->GPU_energy  = (ulong *) calloc(gpu_num, sizeof(ulong));
}

void free_energy_data(energy_data_t *e)
{
	// Node
	free(e->DC_node_energy);
	// CPU/DRAM
	free(e->DRAM_energy);
	free(e->CPU_energy);
	// GPU
	free(e->GPU_energy);
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
	// GPU
	memcpy(dest->GPU_energy, src->GPU_energy, gpu_num * sizeof(ulong));
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
	// GPU
	memset(acc_energy->GPU_energy, 0, gpu_num * sizeof(ulong));
}

/*
 * Power data
 */

void alloc_power_data(power_data_t *p)
{
	// CPU/DRAM
	p->avg_dram = (double *) calloc(num_packs, sizeof(double));
	p->avg_cpu  = (double *) calloc(num_packs, sizeof(double));
	// GPU
	p->avg_gpu  = (double *) calloc(gpu_num  , sizeof(double));
}

void free_power_data(power_data_t *p)
{
	// CPU/DRAM
	free(p->avg_dram);
	free(p->avg_cpu);
	// GPU
	free(p->avg_gpu);
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
	// GPU
	memcpy(dest->avg_gpu , src->avg_gpu , gpu_num   * sizeof(double));
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
	// GPU
	memset(p->avg_gpu , 0, gpu_num   * sizeof(double));
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
	int pid;
	for (pid = 0; pid < gpu_num; pid++) {
		debug("accum_gpu %d %lf total %lf", pid, p->avg_gpu[pid], gpu_power);
		gpu_power = gpu_power + p->avg_gpu[pid];
	}
	return gpu_power;
}
