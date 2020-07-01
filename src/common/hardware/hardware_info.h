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

#ifndef _HW_INFO_H_
#define _HW_INFO_H_

#include <common/sizes.h>
#include <common/states.h>
#include <common/system/file.h>
#include <common/types/generic.h>
#include <common/hardware/cpuid.h>

// Linux files
#define PATH_SYS_SYSTEM		"/sys/devices/system"
#define PATH_SYS_CPU0_TOCO	"/sys/devices/system/cpu/cpu0/topology/core_siblings"
#define PATH_SYS_CPU0_TOTH	"/sys/devices/system/cpu/cpu0/topology/thread_siblings"

// Intel models, Based on arch/x86/include/asm/intel-family.h
// Tip: X means E, EP, ED and Server.
#define CPU_UNIDENTIFIED        -1
#define CPU_SANDY_BRIDGE        42
#define CPU_SANDY_BRIDGE_X      45
#define CPU_IVY_BRIDGE          58
#define CPU_IVY_BRIDGE_X        62
#define CPU_HASWELL             60
#define CPU_HASWELL_X           63
#define CPU_BROADWELL		61
#define CPU_BROADWELL_X         79
#define CPU_BROADWELL_XEON_D	86
#define CPU_SKYLAKE             94
#define CPU_SKYLAKE_X           85
#define CPU_KABYLAKE            158
#define CPU_KNIGHTS_LANDING     87
#define CPU_KNIGHTS_MILL        133

#define INTEL_VENDOR_NAME       "GenuineIntel"
#define AMD_VENDOR_NAME         "AuthenticAMD"


typedef struct topology {
	int cores;
	int threads;
	int sockets;
	int numas;
} topology_t;

/** */
state_t hardware_gettopology(topology_t *topo);

/** */
state_t hardware_topology_getsize(uint *size);

/** Returns if the cpu is examinable by this library */
int is_cpu_examinable();

/** Returns the vendor ID */
int get_vendor_id(char *vendor_id);

/** Returns the family of the processor */
int get_family();

/** Returns 1 if the CPU is APERF/MPERF compatible. */
int is_aperf_compatible();

/** Returns the size of a cache line of the higher cache level. */
int get_cache_line_size();

/** Returns an EAR architecture index (top). */
int get_model();

/** Returns if the processor is HTT capable */
int is_cpu_hyperthreading_capable();

/** Returns true if turbo is enabled */
int is_cpu_boost_enabled();

/** Returns the number of packages detected*/
int detect_packages(int **package_map);

#endif
