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

/**
*    \file power_metrics.h
*    \brief This file include functions to simplify the work of power monitoring It can be used by privileged applications (such as eard)
*	 \brief	or not-privileged applications such as the EARLib (or external commands)
*
*/

#ifndef ACCUMULATORS_POWER_METRICS_H
#define ACCUMULATORS_POWER_METRICS_H

#include <common/states.h>
#include <metrics/accumulators/types.h>
#include <metrics/energy/energy_node.h>
#include <metrics/energy/energy_cpu.h>
#include <metrics/energy/energy_gpu.h>

typedef long long rapl_data_t;
typedef edata_t   node_data_t;

typedef struct energy_mon_data {
	time_t 		 sample_time;
	node_data_t  AC_node_energy;
	node_data_t  DC_node_energy;
	rapl_data_t  *DRAM_energy;
	rapl_data_t  *CPU_energy;
	ulong        *GPU_energy;
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
void print_power(power_data_t *my_power);

/** Write (text mode) the power information in the provided file descriptor */
void report_periodic_power(int fd,power_data_t *my_power);

/*
 *
 */

/** Computes the difference betwen two RAPL energy measurements */
static rapl_data_t diff_RAPL_energy(rapl_data_t end,rapl_data_t init);

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
