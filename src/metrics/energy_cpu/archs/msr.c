/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <metrics/common/pci.h>
#include <common/output/debug.h>
#include <metrics/energy_cpu/archs/msr.h>

// Intel Haswell
static off_t  hwell_pu_addr[1]      = { 0x606 }; //MSR_RAPL_POWER_UNIT
static off_t  hwell_es_pack_addr[1] = { 0x611 }; //pkg energy
static off_t  hwell_es_dram_addr[1] = { 0x619 }; //dram energy
static off_t  hwell_es_core_addr[1] = { 0x000 };
static uint   hwell_pu_count        = 1;
static uint   hwell_es_pack_count   = 1;
static uint   hwell_es_dram_count   = 1;
static uint   hwell_es_core_count   = 0;
// AMD Zen
static off_t  zen_pu_addr[1]      = { 0xC0010299 }; //AMD_MSR_PWR_UNIT
static off_t  zen_es_pack_addr[1] = { 0xC001029B }; //pck_energy
static off_t  zen_es_dram_addr[1] = { 0x00000000 };
static off_t  zen_es_core_addr[1] = { 0xC001029A }; //core energy
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
static uint init = 0;


state_t rapl_msr_load(topology_t *tp_in)
{
	state_t s;

	debug("tp_in initialized %d", tp_in->initialized);
	// Testing
	if (state_fail(s = msr_test(tp_in, MSR_RD))) {
		debug("msr_test fails");
		return s;
	}
	//if (tp_in->vendor == VENDOR_INTEL && tp_in->model == MODEL_HASWELL_X) {
	if (tp_in->vendor == VENDOR_INTEL) {
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
		debug("incompatible architecture");
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	//
	if (state_fail(s = topology_select(tp_in, &tp, TPSelect.socket, TPGroup.merge, 0))) {
		debug("topology select fails");
		return s;
	}
	//
	es_pack_units = 1.0;
	es_core_units = 1.0;
	es_dram_units = 1.0;

	return EAR_SUCCESS;
}

state_t rapl_msr_get_granularity(ctx_t *c, uint *granularity)
{
	*granularity = GRANULARITY_SOCKET;
	return EAR_SUCCESS;
}

state_t rapl_msr_init(ctx_t *c)
{
	ullong result;
	int i, s;

	if (init) {
		return EAR_SUCCESS;
	}
	for (i = 0; i < tp.cpu_count; ++i) {
		s = msr_open(tp.cpus[i].id, MSR_RD);
		if (s) debug("error opening cpu %d", i);
	}
	if ((s = msr_read(tp.cpus[0].id, &result, sizeof(result), *pu_addr))) {
		return s;
	}
	if (tp.model >= MODEL_SAPPHIRE_RAPIDS){
		es_pack_units = pow(0.5, (double) ((result >> 8) & 0x1f)) * 1e9;
		es_core_units = es_pack_units;
		es_dram_units = es_pack_units;
		debug("eneregy units pck %lf", es_core_units);
		debug("eneregy units dram %lf", es_dram_units);
	}else{
		// It's pending to double check in Intel doc if same algorithn can be applied in all Intel archs
		// Take a look to Intel's RAPL test
		es_pack_units = pow(0.5, (double) ((result >> 8) & 0x1f)) * 1e9;
		debug("es_pack_units %lf", es_pack_units);
		es_core_units = es_pack_units;
		es_dram_units = pow(0.5, (double) 16) * 1e9;
		debug("eneregy units pck %lf", es_core_units);
		debug("eneregy units dram %lf", es_dram_units);
	}

	init = 1;
	return EAR_SUCCESS;
}

state_t rapl_msr_dispose(ctx_t *c)
{
	int i;
	if (!init) {
		return EAR_SUCCESS;
	}
	for (i = 0; i < tp.cpu_count; ++i) {
		msr_close(tp.cpus[i].id);
	}
	init = 0;
	return EAR_SUCCESS;
}

state_t rapl_msr_count_devices(ctx_t *c, uint *devs_count_in)
{
	*devs_count_in = tp.cpu_count;
	return EAR_SUCCESS;
}

/** \todo msr_read function return values are not handled.  */
state_t rapl_msr_read(ctx_t *c, ullong *values)
{
	int i, t;
	debug("entering cpu_read");
	for (i = 0; i < tp.cpu_count; i++) {
		debug("reading cpu %d", i);
		// PACK
		for (t = 0; t < es_pack_count; t++) { //this for loop is a glorified if statement, inherited from previous code
			msr_read(tp.cpus[i].id, &values[tp.cpu_count+i], sizeof(ullong), es_pack_addr[t]);
			values[tp.cpu_count + i] &= 0xffffffff;
            debug("read value %llu from PKG", values[tp.cpu_count + i]);
			values[tp.cpu_count + i] *= es_pack_units ; //transform to proper units
            debug("read value %llu from PKG (units %lf)", values[tp.cpu_count + i], es_pack_units);
		}
		// DRAM
		for (t = 0; t < es_dram_count; t++) {
			msr_read(tp.cpus[i].id, &values[i], sizeof(ullong), es_dram_addr[t]);
			values[i] &= 0xffffffff;
			values[i] *= es_dram_units; 
			debug("read value %llu from DRAM", values[i]);
		}
#if 0
		// CORE
		for (t = 0; t < es_core_count; ++t) {
			s = msr_read(tp.cpus[i].id, &values[i*NUM_PACKS+offset], sizeof(ullong), es_core_addr[t]);
			values[i*NUM_PACKS+offset] &= 0xffffffff;
			values[i*NUM_PACKS+offset] *= es_core_units;
			offset++;
		}
#endif
	}
	return EAR_SUCCESS;
}
