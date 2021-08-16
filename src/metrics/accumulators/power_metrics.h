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

/**
*    \file power_metrics.h
*    \brief This file include functions to simplify the work of power monitoring It can be used by privileged applications (such as eard)
*	 \brief	or not-privileged applications such as the EARLib (or external commands)
*
*/

#ifndef ACCUMULATORS_POWER_METRICS_H
#define ACCUMULATORS_POWER_METRICS_H

#include <common/states.h>
#include <metrics/gpu/gpu.h>
#include <metrics/energy/cpu.h>
#include <metrics/energy/energy_node.h>
#include <metrics/accumulators/types.h>

typedef long long rapl_data_t;
typedef edata_t   node_data_t;

typedef struct energy_mon_data {
	time_t 		 sample_time;
	node_data_t  AC_node_energy;
	node_data_t  DC_node_energy;
	rapl_data_t  *DRAM_energy;
	rapl_data_t  *CPU_energy;
	gpu_t 		 *gpu_data;
} energy_data_t;

typedef struct power_data {
    time_t begin;
	time_t end;
    double avg_dc; // node power (AC)
    double avg_ac; // node power (DC)
    double *avg_dram;
    double *avg_cpu;
    double *avg_gpu;
} power_data_t;


/**  Starts power monitoring */
int init_power_ponitoring(ehandler_t *eh);

/** Ends power monitoring */
void end_power_monitoring(ehandler_t *eh);

/*
 *
 */

/** Energy is returned in mili Joules */
int read_enegy_data(ehandler_t *eh,energy_data_t *acc_energy);

/** Computes the power between two energy measurements */
void compute_power(energy_data_t *e_begin, energy_data_t *e_end, power_data_t *my_power);

/*
 *
 */

/** Prints the data from an energy measurement to stdout */
void print_energy_data(energy_data_t *e);

/** Prints power information to the stdout */
void print_power(power_data_t *my_power,uint showdate,int out);

/** Write (text mode) the power information in the provided file descriptor */
void report_periodic_power(int fd,power_data_t *my_power);

/*
 *
 */


/*
 * Energy data
 */

void alloc_energy_data(energy_data_t *e);

void free_energy_data(energy_data_t *e);

/** Copies the energy measurement in *src to *dest */
void copy_energy_data(energy_data_t *dest,energy_data_t *src);

void null_energy_data(energy_data_t *acc_energy);

/*
 * Power data
 */

void alloc_power_data(power_data_t *p);

void free_power_data(power_data_t *p);

void copy_power_data(power_data_t *dest, power_data_t *src);

void null_power_data(power_data_t *p);

/*
 * Accum power
 */

double accum_node_power(power_data_t *p);

double accum_dram_power(power_data_t *p);

double accum_cpu_power(power_data_t *p);

double accum_gpu_power(power_data_t *p);

#endif
