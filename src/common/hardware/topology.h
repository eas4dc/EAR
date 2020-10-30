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

#ifndef COMMON_HARDWARE_TOPOLOGY_H_
#define COMMON_HARDWARE_TOPOLOGY_H_

#include <common/types/generic.h>
// List1: https://en.wikichip.org/wiki/intel/cpuid

#define MODEL_UNIDENTIFIED			-1
#define MODEL_SANDY_BRIDGE_X			45  //X means Server
#define MODEL_IVY_BRIDGE_X			62
#define MODEL_HASWELL_X				63
#define MODEL_BROADWELL_X			79
#define MODEL_SKYLAKE_X				85
#define MODEL_CASCADE_LAKE_X			85
#define MODEL_COOPER_LAKE_X			85
#define MODEL_HEWITT_LAKE_X			86  //Broadwell X arch based
#define MODEL_XEON_D_X				86  //Broadwell X arch based
#define MODEL_KNIGHTS_LANDING_MIC		87  //MIC means Many Integrated Cores
#define MODEL_KNIGHTS_MILL_MIC			133

#define VENDOR_INTEL				0
#define VENDOR_AMD				1

#include <common/states.h>

struct tp_select_s
{
	int core;
	int l3;
	int socket;
}
	TPSelect __attribute__((weak)) =
{
	.core = 1,
	.l3 = 2,
	.socket = 3,
};

struct tp_group_s
{
	int value;
	int merge;
}
	TPGroup __attribute__((weak)) =
{
	.value = 1,
	.merge = 2,
};

typedef struct core_s
{
	int id;
	int is_thread;
	int l3_id;
	int sibling_id;
	int socket_id;
	ulong freq_base;
} core_t;

typedef struct topology_s
{
	core_t *cpus;		// Take a look to core_t structure.
	int cpu_count;		// Total CPUs including threads.
	int core_count;		// Total cores (not counting threads).
	int socket_count;	// 
	int threads_per_core; 	// Number or threads per core (not the whole system).
	int smt_enabled; 	// Multithreading enabled = 1, disabled = 0.
	int l3_count;		// Chunks of L3 in the system.
	int vendor;		// Take a look to top defines.
	int family;		// Take a look to top defines.
	int model;		// Take a look to top defines.
} topology_t;

state_t topology_select(topology_t *t, topology_t *s, int component, int group, int val);

state_t topology_init(topology_t *topo);

state_t topology_close(topology_t *topo);

#endif
