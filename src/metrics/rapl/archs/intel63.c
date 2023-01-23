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

#include <math.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/rapl/archs/intel63.h>

// Intel Haswell
static off_t  hwell_pu_addr[1]      = { 0x606 }; // MSR_RAPL_POWER_UNIT
static off_t  hwell_es_pack_addr[1] = { 0x611 }; // MSR_PKG_ENERGY_STATUS
static off_t  hwell_es_dram_addr[1] = { 0x619 }; // MSR_DRAM_ENERGY_STATUS
static off_t  hwell_es_core_addr[1] = { 0x000 };
static uint   hwell_pu_count        = 1;
static uint   hwell_es_pack_count   = 1;
static uint   hwell_es_dram_count   = 1;
static uint   hwell_es_core_count   = 0;
// AMD Zen
static off_t  zen_pu_addr[1]      = { 0xC0010299 }; //
static off_t  zen_es_pack_addr[1] = { 0xC001029B };
static off_t  zen_es_dram_addr[1] = { 0x00000000 };
static off_t  zen_es_core_addr[1] = { 0xC001029A };
static uint   zen_pu_count        = 1;
static uint   zen_es_pack_count   = 1;
static uint   zen_es_dram_count   = 0;
static uint   zen_es_core_count   = 1;
// Selected
static off_t *pu_addr;
static off_t *es_pack_addr;
static off_t *es_dram_addr;
static off_t *es_core_addr;
static uint   pu_count;
static uint   es_pack_count;
static uint   es_dram_count;
static uint   es_core_count;
static double es_pack_units;
static double es_dram_units;
static double es_core_units;
//
static topology_t tp;
static uint init;

state_t rapl_load(topology_t *tp_in)
{
	state_t s;

	// Testing
	if (state_fail(s = msr_test(tp_in, MSR_RD))) {
		return s;
	}
	if (tp_in->vendor == VENDOR_INTEL && tp_in->model >= MODEL_HASWELL_X) {
		pu_addr       = hwell_pu_addr;
		es_pack_addr  = hwell_es_pack_addr;
		es_dram_addr  = hwell_es_dram_addr;
		es_core_addr  = hwell_es_core_addr;
		pu_count      = hwell_pu_count;
		es_pack_count = hwell_es_pack_count;
		es_dram_count = hwell_es_dram_count;
		es_core_count = hwell_es_core_count;
	} else if (tp_in->vendor == VENDOR_AMD && tp_in->family >= FAMILY_ZEN) {
		pu_addr       = zen_pu_addr;
		es_pack_addr  = zen_es_pack_addr;
		es_dram_addr  = zen_es_dram_addr;
		es_core_addr  = zen_es_core_addr;
		pu_count      = zen_pu_count;
		es_pack_count = zen_es_pack_count;
		es_dram_count = zen_es_dram_count;
		es_core_count = zen_es_core_count;
	} else {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	//
	if (state_fail(s = topology_select(tp_in, &tp, TPSelect.socket, TPGroup.merge, 0))) {
		return s;
	}
	//
	es_pack_units = 1.0;
	es_core_units = 1.0;
	es_dram_units = 1.0;

	return EAR_SUCCESS;
}

void rapl_get_api(uint *api)
{
    *api = API_INTEL63;
}

void rapl_get_granularity(uint *granularity)
{
	*granularity = GRANULARITY_SOCKET;
}

state_t rapl_init(ctx_t *c)
{
	ullong result;
    state_t s;
	int i;

	if (init) {
		return EAR_SUCCESS;
	}
	for (i = 0; i < tp.cpu_count; ++i) {
		if (state_fail(s = msr_open(tp.cpus[i].id, MSR_RD))) {
            debug("msr_open: %s", state_msg);
            return s;
        }
	}
	if ((s = msr_read(tp.cpus[0].id, &result, sizeof(result), pu_addr[0]))) {
		return s;
	}
	// Take a look to Intel's RAPL test
	es_pack_units = (ullong) (pow(0.5, (double) ((result >> 8) & 0x1f)) * (double) 1e9); //
	es_core_units = es_pack_units;
	es_dram_units = (ullong) (pow(0.5, (double) 16) * (double) 1e9);
	init = 1;
	return EAR_SUCCESS;
}

state_t rapl_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t rapl_count_devices(ctx_t *c, uint *devs_count_in)
{
	*devs_count_in = tp.cpu_count;
	return EAR_SUCCESS;
}

state_t rapl_read(ctx_t *c, rapl_t *r)
{
    // We avoid -Wunused-but-set-variable
    // state_t s;

	int i, t;
	for (i = 0; i < tp.cpu_count; ++i) {

		// PACK
		for (t = 0; t < es_pack_count; ++t) {

            // We avoid -Wunused-but-set-variable
			// s = msr_read(tp.cpus[i].id, &r[i].pack, sizeof(ullong), es_pack_addr[t]);
			msr_read(tp.cpus[i].id, &r[i].pack, sizeof(ullong), es_pack_addr[t]);

			r[i].pack &= 0xffffffff;
		}

		// DRAM
		for (t = 0; t < es_dram_count; ++t) {

            // We avoid -Wunused-but-set-variable
			// s = msr_read(tp.cpus[i].id, &r[i].dram, sizeof(ullong), es_dram_addr[t]);
			msr_read(tp.cpus[i].id, &r[i].dram, sizeof(ullong), es_dram_addr[t]);

			r[i].dram &= 0xffffffff;
		}
		// CORE
		for (t = 0; t < es_core_count; ++t) {
            // We avoid -Wunused-but-set-variable
			// s = msr_read(tp.cpus[i].id, &r[i].core, sizeof(ullong), es_core_addr[t]);
			msr_read(tp.cpus[i].id, &r[i].core, sizeof(ullong), es_core_addr[t]);

			r[i].core &= 0xffffffff;
		}
	}

	return EAR_SUCCESS;
}

state_t rapl_read_diff(ctx_t *c, rapl_t *r2, rapl_t *r1, rapl_t *rD)
{
    state_t s;
    if (state_fail(s = rapl_read(c, r2))) {
        return s;
    }
    return rapl_data_diff(r2, r1, rD);
}

state_t rapl_read_copy(ctx_t *c, rapl_t *r2, rapl_t *r1, rapl_t *rD)
{
    state_t s;
    if (state_fail(s = rapl_read_diff(c, r2, r1, rD))) {
        return s;
    }
    return rapl_data_copy(r1, r2);
}

state_t rapl_data_alloc(rapl_t **r)
{
    *r = calloc(tp.cpu_count, sizeof(rapl_t));
    return EAR_SUCCESS;
}

state_t rapl_data_copy(rapl_t *rd, rapl_t *rs)
{
    memcpy(rd, rs, sizeof(rapl_t)*tp.cpu_count);    
    return EAR_SUCCESS;
}

state_t rapl_data_diff(rapl_t *r2, rapl_t *r1, rapl_t *rD)
{
	int i;
	for (i = 0; i < tp.cpu_count; ++i) {
		rD[i].pack = r2[i].pack - r1[i].pack;
		rD[i].dram = r2[i].dram - r1[i].dram;
		rD[i].core = r2[i].core - r1[i].core;
        debug("SOCK%d: %llu socket, %llu dram, %llu core (diff)",
            i, rD[i].pack, rD[i].dram, rD[i].core);
		// Missing overflow control
		rD[i].pack *= es_pack_units;
		rD[i].dram *= es_dram_units;
		rD[i].core *= es_core_units;
        debug("SOCK%d: %llu socket, %llu dram, %llu core (units)",
            i, rD[i].pack, rD[i].dram, rD[i].core);
		rD[i].pack /= 1E9;
		rD[i].dram /= 1E9;
		rD[i].core /= 1E9;
        debug("SOCK%d: %llu socket, %llu dram, %llu core (power)",
            i, rD[i].pack, rD[i].dram, rD[i].core);
	}
	return EAR_SUCCESS;
}

void rapl_data_print(rapl_t *rD, int fd)
{
	int i;
	for (i = 0; i < tp.cpu_count; ++i) {
        dprintf(fd, "%llu\t", rD[i].pack);
    }
    dprintf(fd, "(W)\n");
}
