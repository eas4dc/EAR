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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef COMMON_HARDWARE_TOPOLOGY_H_
#define COMMON_HARDWARE_TOPOLOGY_H_

#include <common/states.h>
#include <common/types/generic.h>
#include <common/hardware/defines.h>

// Topology specifics
#define TOPO_UNDEFINED        -1
#define TOPO_CL_COUNT         (4+1) // Cache levels count, L0 to L4, but 0 doesn't exists
// Supported vendors
#define VENDOR_UNKNOWN         TOPO_UNDEFINED
#define VENDOR_INTEL            0
#define VENDOR_AMD              1
#define VENDOR_ARM              2
// INTEL models (https://en.wikichip.org/wiki/intel/cpuid and arch/x86/include/asm/intel-family.h)
#define MODEL_SANDY_BRIDGE_X    45
#define MODEL_IVY_BRIDGE_X      62
#define MODEL_HASWELL_X         63 // Also 0x3F (extended 3, model F)
#define MODEL_BROADWELL_X       79 // Also 0x4F (extended 4, model F)
#define MODEL_SKYLAKE_X         85 // Also 0x55 (extended 5, model 5)
#define MODEL_ICELAKE           106
#define MODEL_ICELAKE_X         MODEL_ICELAKE // It doesn't have the X, just Skylake and Cascadelake
#define MODEL_TIGERLAKE         140
#define MODEL_CASCADELAKE_X     85 // ¿Same as Skylake but higher stepping?
#define MODEL_SAPPHIRE_RAPIDS   143
// AMD models (https://en.wikichip.org/wiki/amd/cpuid)
#define FAMILY_BOBCAT           20
#define FAMILY_BULLDOZER        21
#define FAMILY_JAGUAR           22
#define FAMILY_ZEN              0x17 // Also 23
#define FAMILY_ZEN3             0x19 // Also 25
// ARM family (architecture)
#define MODEL_A8                0x01
#define FAMILY_A8               0x01
#define FAMILY_A9               0x02
// ARM models (implementer 8bits + 12bits)
#define MODEL_NEOVERSE_N1       0x41D0C
#define MODEL_NEOVERSE_N2       0x41D4A 
#define MODEL_NEOVERSE_V1       0x41D40
#define MODEL_FX1000            0x46001
#define DUMMY_BASE_FREQ         100000

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

typedef struct cl_s {
    int id;
} cl_t;

typedef struct cpu_s {
    int     id;
    int     apicid;
    int     is_thread;      // If is the secondary thread in a core.
    int     sibling_id;     // The thread sibling within a core.
    int     core_id;        // Basically, the lesser sibling_id
    int     socket_id;
    cl_t    l[TOPO_CL_COUNT]; // Index = level, 0 doesn't exists
} cpu_t;

typedef struct topology_s {
    cpu_t *cpus;         // Take a look to cpu_t structure.
    int   cpu_count;      // Total CPUs including threads.
    int   core_count;     // Total cores (not counting threads).
    int   socket_count;   //
    int   threads_per_core; // Number or threads per core (not the whole system).
    int   smt_enabled;    // Multithreading enabled = 1, disabled = 0.
    int   l2_count;       // Chunks of L2 in the system.
    int   l3_count;       // Chunks of L3 in the system.
    int   l4_count;       // Chunks of L4 in the system.
    int   cache_last_level; // Returns the maximum cache level
    int   cache_line_size;
    int   vendor;         // Take a look to top defines.
    int   family;         // Take a look to top defines.
    int   model;          // Take a look to top defines.
    char  brand[48];
    int   gpr_count;      // Number of general purpose registers.
    int   gpr_bits;       // General purpose registers bit width.
    int   nmi_watchdog;	  // NMI watdog enabled.
    int   apicids_found;  // List of CPUs ApicId's found.
    int   initialized;    // 1 if the topology is initialized
    ulong base_freq;      // Nominal CPU clock, in KHz
    int   boost_enabled;
    int   avx512;         // 1 if AVX-512 is enabled
    int   sve;            // 1 if ARM-SVE is enabled
    int   sve_bits;       // Counts the ARM-SVE vector bits
    int   tdp;
} topology_t;

state_t topology_select(topology_t *t, topology_t *s, int component, int group, int val);

state_t topology_copy(topology_t *dst, topology_t *src);

state_t topology_init(topology_t *topo);

state_t topology_close(topology_t *topo);

state_t topology_print(topology_t *topo, int fd);

state_t topology_tostr(topology_t *topo, char *buffer, size_t n);
// This function won't be public, use topo.base_freq value instead.
state_t topology_freq_getbase(uint cpu, ulong *freq_base);

#endif
