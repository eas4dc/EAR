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

//#define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <common/hardware/cpuid.h>
#include <common/hardware/defines.h>
#include <common/hardware/topology_asm.h>

__attribute__((unused)) static void topology_getid_x86(topology_t *topo)
{
    cpuid_regs_t r;
    int buffer[4];

    /* Vendor */
    CPUID(r,0,0);
    buffer[0] = r.ebx;
    buffer[1] = r.edx;
    buffer[2] = r.ecx;
    topo->vendor = !(buffer[0] == GENUINE_INTEL); // Intel

    /* Family */
    CPUID(r,1,0);
    buffer[0] = cpuid_getbits(r.eax, 11,  8);
    buffer[1] = cpuid_getbits(r.eax, 27, 20); // extended
    buffer[2] = buffer[0]; // auxiliar

    if (buffer[0] == 0x0F) {
        topo->family = buffer[1] + buffer[0];
    } else {
        topo->family = buffer[0];
    }

    /* Model */
    CPUID(r,1,0);
    buffer[0] = cpuid_getbits(r.eax,  7,  4);
    buffer[1] = cpuid_getbits(r.eax, 19, 16); // extended

    if (buffer[2] == 0x0F || (buffer[2] == 0x06 && topo->vendor == VENDOR_INTEL)) {
        topo->model = (buffer[1] << 4) | buffer[0];
    } else {
        topo->model = buffer[0];
    }

    /* Cache line size */
    uint max_level = 0;
    uint cur_level = 0;
    int index      = 0;

    if (topo->vendor == VENDOR_INTEL)
    {
        while (1)
        {
            CPUID(r,4,index);

            if (!(r.eax & 0x0F)) break;
            cur_level = cpuid_getbits(r.eax, 7, 5);

            if (cur_level >= max_level) {
                topo->cache_line_size = cpuid_getbits(r.ebx, 11, 0) + 1;
                max_level = cur_level;
            }

            index = index + 1;
        }
    } else {
        CPUID(r,0x80000005,0);
        topo->cache_line_size = r.edx & 0xFF;
    }

    /* General-purpose/fixed registers */
    CPUID(r, 0x0a, 0);

    if (topo->vendor == VENDOR_INTEL) {
        //  Intel Vol. 2A
        // 	bits   7,0: version of architectural performance monitoring
        // 	bits  15,8: number of GPR counters per logical processor
        //	bits 23,16: bit width of GPR counters
        topo->gpr_count = cpuid_getbits(r.eax, 15,  8);
        topo->gpr_bits  = cpuid_getbits(r.eax, 23, 16);
    } else {
        if (topo->family >= FAMILY_ZEN) {
            topo->gpr_count = 6;
            topo->gpr_bits  = 48;
        }
    }
}

__attribute__((unused)) static void topology_getid_arm64(topology_t *topo)
{
    topo->vendor          = VENDOR_ARM;
    topo->model           = MODEL_A8;
    topo->cache_line_size = 64; // 64 bytes
}

void topology_asm_getid(topology_t *topo)
{
    #if __ARCH_ARM
    return topology_getid_arm64(topo);
    #else
    return topology_getid_x86(topo);
    #endif
}

static void topology_getbrand_x86(topology_t *topo)
{
    cpuid_regs_t r2, r3, r4;
    uint brand[12];
        
    strcpy(topo->brand, "UNKNOWN");
    if (!cpuid_isleaf(0x80000004)) {
        return;
    }
    CPUID(r2,0x80000002,0);
    CPUID(r3,0x80000003,0);
    CPUID(r4,0x80000004,0);
    brand[ 0] = r2.eax;
    brand[ 1] = r2.ebx;
    brand[ 2] = r2.ecx;
    brand[ 3] = r2.edx;
    brand[ 4] = r3.eax;
    brand[ 5] = r3.ebx;
    brand[ 6] = r3.ecx;
    brand[ 7] = r3.edx;
    brand[ 8] = r4.eax;
    brand[ 9] = r4.ebx;
    brand[10] = r4.ecx;
    brand[11] = r4.edx;

    strcpy(topo->brand, (char *) brand);	
    debug("Brand: %s", topo->brand);
}

void topology_asm_getbrand(topology_t *topo)
{
    #if __ARCH_X86
    topology_getbrand_x86(topo);
    #endif
}
