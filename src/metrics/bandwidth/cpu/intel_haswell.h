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

#ifndef METRICS_BANDWIDTH_CPU_INTEL_HASWELL_H
#define METRICS_BANDWIDTH_CPU_INTEL_HASWELL_H

#include <metrics/bandwidth/cpu/utils.h>

// HASWELL_X & BROADWELL_X
static short HASWELL_X_IDS[]      = { 0x2FB4, 0x2FB5, 0x2FB0, 0x2FB1, 0x2FD4, 0x2FD5, 0x2FD0, 0x2FD1 };
static char HASWELL_X_STA_CTL[]   = { 0xF4 };
static int  HASWELL_X_STA_CMD[]   = { 0x30000 };
static char HASWELL_X_STO_CTL[]   = { 0xF4 };
static int  HASWELL_X_STO_CMD[]   = { 0x30100 };
static char HASWELL_X_RST_CTL[]   = { 0xF4, 0xD8, 0xDC };
static int  HASWELL_X_RST_CMD[]   = { 0x30102, 0x420304, 0x420C04 };
static char HASWELL_X_RED_CTR[]   = { 0xA0, 0xA8 };
static char HASWELL_X_DEVICES[]   = { 0x14, 0x15, 0x17, 0x18 };
static char HASWELL_X_FUNCTIONS[] = { 0x00, 0x01 };
static int  HASWELL_X_N_FUNCTIONS = 8;
// SKYLAKE_X
static short SKYLAKE_X_IDS[]      = { 0x2042, 0x2046, 0x204A };
static char SKYLAKE_X_DEVICES[]   = { 0x0A, 0x0B, 0x0C, 0x0D };
static char SKYLAKE_X_REGISTERS[] = { 0xF8, 0xF4, 0xD8, 0xA0 };
static char SKYLAKE_X_FUNCTIONS[] = { 0x02, 0x06 };
static int  SKYLAKE_X_N_FUNCTIONS = 6;

/** Scans PCI buses and allocates file descriptors memory.
*   Returns the number of uncores counters on success or EAR_ERROR. */
int pci_init_uncores(int cpu_model);

/** Get the number of performance monitor counters. 
*   pci_init_uncores() have to be called before.
*   Returns the number of PCI uncore counters. */
int pci_count_uncores();

/** Freezes and resets all performance monitor uncore counters.
*   Returns 0 on success or EAR_ERROR. */
int pci_reset_uncores();

/** Unfreezes all PCI uncore counters.
*   Returns 0 on success or EAR_ERROR. */
int pci_start_uncores();

/** Freezes all PMON uncore counters and gets it's values. The array
*   has to be greater or equal than the number of PMON uncore counters
*   returned by count_uncores() function. The returned values are the
*   read and write bandwidth values in index [i] and [i+1] respectively.
*   Returns 0 on success or EAR_ERROR. */
int pci_stop_uncores(unsigned long long *values);

/** Gets uncore counters values.
*   Returns 0. */
int pci_read_uncores(unsigned long long *values);

/** Closes file descriptors and frees memory.
*   Returns 0 on success or EAR_ERROR. */
int pci_dispose_uncores();

/** Checks if the found pci uncore functions have any abnormality.
*   Returned values are EAR_SUCCESS, EAR_WARNING or EAR_ERROR. */
int pci_check_uncores();

#endif
