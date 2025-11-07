/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_HARDWARE_TOPOLOGY_H_
#define COMMON_HARDWARE_TOPOLOGY_H_

#include <common/hardware/defines.h>
#include <common/states.h>
#include <common/types/generic.h>

// Topology specifics
#define TOPO_UNDEFINED -1
#define TOPO_CL_COUNT  (4 + 1) // Cache levels count, L0 to L4, but 0 doesn't exists
// Supported vendors
#define VENDOR_UNKNOWN TOPO_UNDEFINED
#define VENDOR_INTEL   0
#define VENDOR_AMD     1
#define VENDOR_ARM     2
// INTEL models (https://en.wikichip.org/wiki/intel/cpuid and arch/x86/include/asm/intel-family.h)
#define MODEL_SANDY_BRIDGE_X  45
#define MODEL_IVY_BRIDGE_X    62
#define MODEL_HASWELL_X       63 // Also 0x3F (extended 3, model F)
#define MODEL_BROADWELL_X     79 // Also 0x4F (extended 4, model F)
#define MODEL_SKYLAKE_X       85 // Also 0x55 (extended 5, model 5)
#define MODEL_ICELAKE         106
#define MODEL_ICELAKE_X       MODEL_ICELAKE // It doesn't have the X
#define MODEL_TIGERLAKE       140
#define MODEL_CASCADELAKE_X   85 // ¿Same as Skylake but higher stepping?
#define MODEL_SAPPHIRE_RAPIDS 143
// AMD models (https://en.wikichip.org/wiki/amd/cpuid)
#define FAMILY_BOBCAT        20
#define FAMILY_BULLDOZER     21
#define FAMILY_JAGUAR        22
#define FAMILY_ZEN           0x17 // Also 23, includes ZEN2 and ZEN+
#define FAMILY_ZEN3          0x19 // Also 25, includes ZEN4
#define MODEL_IS_ZEN4(model) (model >= 0x61 || (model >= 0x11 && model < 0x21))
#define FAMILY_ZEN5          0x1A // Also 26
// ARM family (architecture)
#define MODEL_A8  0x01
#define FAMILY_A8 0x01
#define FAMILY_A9 0x02
// ARM models (implementer 8bits + 12bits)
#define MODEL_NEOVERSE_N1 0x41D0C
#define MODEL_NEOVERSE_N2 0x41D4A
#define MODEL_NEOVERSE_V1 0x41D40
#define MODEL_NEOVERSE_V2 0x41D4F
#define MODEL_FX1000      0x46001

struct tp_select_s {
    int core;
    int l3;
    int socket;
} TPSelect __attribute__((weak)) = {
    .core   = 1,
    .l3     = 2,
    .socket = 3,
};

struct tp_group_s {
    int value;
    int merge;
} TPGroup __attribute__((weak)) = {
    .value = 1,
    .merge = 2,
};

typedef struct cl_s {
    int id;
} cl_t;

typedef struct cpu_s {
    int id;                //
    int apicid;            // Taken from
    int is_thread;         // If is the secondary thread in a core.
    int sibling_id;        // The thread sibling within a core.
    int core_id;           // Basically the minor sibling id
    int socket_id;         // Socket id taken from 'physical_package_id' file
    int socket_idx;        // Socket index from 0 to socket_count, best option to work with arrays
    cl_t l[TOPO_CL_COUNT]; // Index = level, 0 doesn't exists
} cpu_t;

typedef struct topology_s {
    cpu_t *cpus;          // Take a look to cpu_t structure.
    int cpu_count;        // Total CPUs including threads.
    int core_count;       // Total cores (not counting threads).
    int socket_count;     //
    int threads_per_core; // Number or threads per core (not the whole system).
    int smt_enabled;      // Multithreading enabled = 1, disabled = 0.
    int l2_count;         // Chunks of L2 in the system.
    int l3_count;         // Chunks of L3 in the system.
    int l4_count;         // Chunks of L4 in the system.
    int cache_last_level; // Returns the maximum cache level
    int cache_line_size;
    int vendor; // Take a look to top defines.
    int family; // Take a look to top defines.
    int model;  // Take a look to top defines.
    char brand[48];
    int gpr_count;     // Number of general purpose registers.
    int gpr_bits;      // General purpose registers bit width.
    int nmi_watchdog;  // NMI watdog enabled.
    int apicids_found; // List of CPUs ApicId's found.
    int initialized;   // 1 if the topology is initialized
    int avx512;        // 1 if AVX-512 is enabled
    int sve;           // 1 if ARM-SVE is enabled
    int sve_bits;      // Counts the ARM-SVE vector bits
    int tdp;
} topology_t;

state_t topology_select(topology_t *t, topology_t *s, int component, int group, int val);

state_t topology_copy(topology_t *dst, topology_t *src);

state_t topology_init(topology_t *topo);

state_t topology_close(topology_t *topo);

void topology_print(topology_t *topo, int fd);

char *topology_tostr(topology_t *topo, char *buffer, size_t n);

#endif
