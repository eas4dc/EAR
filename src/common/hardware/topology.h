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

// https://en.wikichip.org/wiki/intel/cpuid
// arch/x86/include/asm/intel-family.h
#define MODEL_UNIDENTIFIED			-1
#define MODEL_SANDY_BRIDGE_X		45
#define MODEL_IVY_BRIDGE_X			62
#define MODEL_HASWELL_X				63 // Also 0x3F (extended 3, model F)
#define MODEL_BROADWELL_X			79 // Also 0x4F (extended 4, model F)
#define MODEL_SKYLAKE_X				85 // Also 0x55 (extended 5, model 5)
#define MODEL_CASCADE_LAKE_X		85
#define MODEL_COOPER_LAKE_X			85
#define MODEL_HEWITT_LAKE_X			86
#define MODEL_XEON_D_X				86
#define MODEL_KNIGHTS_LANDING_MIC	87
#define MODEL_KNIGHTS_MILL_MIC		133
// https://en.wikichip.org/wiki/amd/cpuid
#define FAMILY_BOBCAT				20
#define FAMILY_BULLDOZER			21
#define FAMILY_JAGUAR				22
#define FAMILY_ZEN					0x17 // Also 23
#define FAMILY_ZEN3					25
// Supported vendors
#define VENDOR_INTEL				0
#define VENDOR_AMD					1

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
	int is_thread;      // If is the second thread in a core.
	int l3_id;
	int sibling_id;     // The thread sibling within a core.
	int socket_id;
} core_t;

typedef struct topology_s
{
	core_t *cpus;       // Take a look to core_t structure.
	int cpu_count;      // Total CPUs including threads.
	int core_count;     // Total cores (not counting threads).
	int socket_count;   //
	int threads_per_core; // Number or threads per core (not the whole system).
	int smt_enabled;    // Multithreading enabled = 1, disabled = 0.
	int l3_count;       // Chunks of L3 in the system.
	int cache_line_size; //
	int vendor;         // Take a look to top defines.
	int family;         // Take a look to top defines.
	int model;          // Take a look to top defines.
	int gpr_count;		// Number of general purpose registers.
	int gpr_bits;		// General purpose registers bit width.
	int nmi_watchdog;	// NMI watdog enabled.
} topology_t;

state_t topology_select(topology_t *t, topology_t *s, int component, int group, int val);

state_t topology_copy(topology_t *dst, topology_t *src);

state_t topology_init(topology_t *topo);

state_t topology_close(topology_t *topo);

state_t topology_print(topology_t *topo, int fd);

state_t topology_tostr(topology_t *topo, char *buffer, size_t n);

state_t topology_freq_getbase(uint cpu, ulong *freq_base);

#endif
