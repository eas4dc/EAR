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

//#define SHOW_DEBUGS 1

#include <common/hardware/topology.h>
#include <metrics/common/msr.h>
#include <metrics/frequency/cpufreq_base.h>
#include <common/hardware/cpuid.h>
#include <common/hardware/bithack.h>

#define ZEN_REG_P0     0xc0010064
#define ZEN_REG_HWCONF 0xc0010015

static ulong find_base_frequency(topology_t *tp)
{
	ulong freq_base = 0LU;

	if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) {
		cpuid_regs_t regs;
		// Getting CPUID 16H
		CPUID(regs, 0x16, 0);
		// Computing base frequency
		freq_base = (regs.eax & 0x0000FFFF) * 1000LU;
	} else if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
        ullong aux;
        ullong did;
        ullong fid;
        // Getting the P0 Core Frequency
        if (state_ok(msr_read(0, &aux, sizeof(ullong), ZEN_REG_P0))) {
            fid = getbits64(aux,  7,  0);
            did = getbits64(aux, 13,  8);
            if (did > 0) {
                aux = ((fid * 200LLU) / did) * 1000;
                freq_base = (ulong) aux;
            }
        }else{
					if (state_fail(topology_freq_getbase(0,&freq_base))) freq_base = 0LU;
				}
    }
    debug("Detected base frequency %lu KHz", freq_base);
    return freq_base;
} 

void cpufreq_get_base(topology_t *tp, ulong *freq_base)
{
  *freq_base = find_base_frequency(tp);
  debug("freq_base: %lu KHz", *freq_base);
}

static uint find_boost_enabled(topology_t *tp)
{
  uint boost_enabled = 0;

  if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) {
    cpuid_regs_t regs;
    //
    CPUID(regs, 0x06, 0);
    //
    boost_enabled = (ulong) cpuid_getbits(regs.eax, 1, 1);
  } else if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
    ullong aux;
    if (state_ok(msr_read(0, &aux, sizeof(ullong), ZEN_REG_HWCONF))) {
      boost_enabled = (uint) !getbits64(aux, 25, 25);
    }
  }

  return boost_enabled;
}



void cpufreq_get_boost_enabled(topology_t *tp, uint *boost_enabled)
{
	*boost_enabled = find_boost_enabled(tp);
	debug("boost_enabled: %u", *boost_enabled);
}
