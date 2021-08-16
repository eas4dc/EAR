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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <common/system/time.h>
#include <common/output/debug.h>
#include <metrics/common/msr.h>
#include <metrics/bandwidth/cpu/amd49.h>


// References:
// PPR for AMD Family 17h.
// 	Topic 2.1.15.4.1 Floating Point (FP) Events.
//	Topic 2.1.14.3 MSRs - MSRC001_0xxx. Look for DF_ word.
//
// Up to 8 CCDs. But these data counters
// belongs to a Data Fabric, so are shared
// by all cores. So just accessing the
// core 0 will be enough. Remember the 49h
// (7742) model have just 4 DF counters.
const ulong	df_cmd0 = 0x0000000000403807; //CH0
const ulong df_cmd1 = 0x0000000000403847; //CH1
const ulong df_cmd2 = 0x0000000000403887; //CH2
const ulong df_cmd3 = 0x00000000004038C7; //CH3
const ulong df_cmd4 = 0x0000000100403807; //CH4
const ulong df_cmd5 = 0x0000000100403847; //CH5
const ulong df_cmd6 = 0x0000000100403887; //CH6
const ulong df_cmd7 = 0x00000001004038C7; //CH7
const ulong df_ctl0 = 0x00000000c0010240;
const ulong df_ctl1 = 0x00000000c0010242;
const ulong df_ctl2 = 0x00000000c0010244;
const ulong df_ctl3 = 0x00000000c0010246;
const ulong df_ctr0 = 0x00000000c0010241;
const ulong df_ctr1 = 0x00000000c0010243;
const ulong df_ctr2 = 0x00000000c0010245;
const ulong df_ctr3 = 0x00000000c0010247;
// These L3 counters are shared by all CCX,
// and each CCX have 4 threads and 8 cores.
// It is required to get one core per CCX. 
const ulong cmd_l3  = 0xff0f000000400104;
const uint  ctl_l3  = 0x00000000c0010230;
const uint  ctr_l3  = 0x00000000c0010231;
// To stop all the counters. 
const ulong cmd_off  = 0x0000000000000000;

typedef struct bwidth_amd49_s
{
	topology_t tp;
	timestamp_t time;
	ulong *cas_curr;
	ulong *cas_prev;
	ulong *l3_prev;
	ulong *l3_curr;
	double coeff_count;
	double coeff;
	uint fd_count;
} bwidth_amd49_t;

static int initialized;

state_t bwidth_amd49_status(topology_t *tp)
{
	if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN){
		return EAR_SUCCESS;
	}
	return EAR_ERROR;
}

state_t bwidth_amd49_init(ctx_t *c, topology_t *tp)
{
	bwidth_amd49_t *bw;
	state_t s;
	int i;

	if (c == NULL) {
		debug("%s", Generr.context_null);
		return_msg(EAR_ERROR, Generr.context_null);
	}

	// Initializing data
	c->context = calloc(1, sizeof(bwidth_amd49_t));

	if (c->context == NULL) {
		debug("%s", Generr.alloc_error);
		return_msg(EAR_ERROR, Generr.alloc_error);
	}

	bw = (bwidth_amd49_t *) c->context;
	uint ccx_count = tp->l3_count;
	#if 0
	uint ccd_count = ccx_count / 2;
	#endif

	bw->fd_count = ccx_count;
	bw->cas_prev = calloc(2, sizeof(ulong));
	bw->cas_curr = calloc(2, sizeof(ulong));
	bw->l3_curr  = calloc(ccx_count, sizeof(ulong));
	bw->l3_prev  = calloc(ccx_count, sizeof(ulong));

	// Getting the L3 groups
	topology_select(tp, &bw->tp, TPSelect.l3, TPGroup.merge, 0);

	for (i = 0; i < bw->fd_count; ++i)
	{
		debug("opening CPU %d (socket %d, L3 %d)",
			bw->tp.cpus[i].id, bw->tp.cpus[i].socket_id, bw->tp.cpus[i].l3_id);
		s = msr_open(bw->tp.cpus[i].id);

		if (state_fail(s)) {
			return EAR_ERROR;
		}
	}

	//
	debug("initialized");
	initialized = 1;

	return EAR_SUCCESS;
}

state_t bwidth_amd49_dispose(ctx_t *c)
{
	bwidth_amd49_t *bw = (bwidth_amd49_t *) c->context;
	state_t s;
	int i;
	
	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}

	for (i = 0; i < bw->fd_count; ++i) {
		s = msr_close(bw->tp.cpus[i].id);
	}

	topology_close(&bw->tp);
	free(bw->cas_curr);

	// Remove warning
	(void) (s);

	return EAR_SUCCESS;
}

state_t bwidth_amd49_count(ctx_t *c, uint *count)
{
	bwidth_amd49_t *bw = (bwidth_amd49_t *) c->context;
	
	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	
	*count = bw->fd_count;
	
	return EAR_SUCCESS;
}

state_t bwidth_amd49_start(ctx_t *c)
{
	bwidth_amd49_t *bw = (bwidth_amd49_t *) c->context;
	state_t s;
	int i;
	
	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	// One chunk of L3 per CCX	
	for (i = 0; i < bw->fd_count; ++i) {
		s = msr_write(bw->tp.cpus[i].id, &cmd_l3 , sizeof(ulong), ctl_l3);
	}
	// Remove warning
	(void) (s);

	return EAR_SUCCESS;
}

state_t bwidth_amd49_stop(ctx_t *c, ullong *cas)
{
	bwidth_amd49_t *bw = (bwidth_amd49_t *) c->context;
	state_t s;
	int i;
	
	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	#if 0
	// Two channels per CCD
	s = msr_write(bw->tp.cpus[0].id, &cmd_off, sizeof(ulong), df_ctl0);
	s = msr_write(bw->tp.cpus[0].id, &cmd_off, sizeof(ulong), df_ctl1);
	#endif
	// One chunk of L3 per CCX	
	for (i = 0; i < bw->fd_count; ++i) {
		s = msr_write(bw->tp.cpus[i].id, &cmd_off, sizeof(ulong), ctl_l3);
	}
	// Remove warning
	(void) (s);

	return bwidth_amd49_read(c, cas);
}

state_t bwidth_amd49_reset(ctx_t *c)
{
	bwidth_amd49_t *bw = (bwidth_amd49_t *) c->context;
	state_t s;
	int i;
	
	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	#if 0
	// Two channels per CCD (more or less)
	s = msr_write(bw->tp.cpus[0].id, &df_cmd0, sizeof(ulong), df_ctl0);
	s = msr_write(bw->tp.cpus[0].id, &df_cmd1, sizeof(ulong), df_ctl1);
	
	s = msr_write(bw->tp.cpus[0].id, &cmd_off, sizeof(ulong), df_ctr0);
	s = msr_write(bw->tp.cpus[0].id, &cmd_off, sizeof(ulong), df_ctr1);
	#endif
	// One chunk of L3 per CCX	
	for (i = 0; i < bw->fd_count; ++i) {
		s = msr_write(bw->tp.cpus[i].id, &cmd_off, sizeof(ulong), ctr_l3);
	}
	// Remove warning
	(void) (s);

	return EAR_SUCCESS;
}

//state_t bwidth_amd49_counters_read(ctx_t *c, double *gbs)
state_t bwidth_amd49_read(ctx_t *c, ullong *cas)
{
	bwidth_amd49_t *bw = (bwidth_amd49_t *) c->context;
	state_t s;
	int i;

	// Currently we are just using the L3 miss per L3 chunk to
	// estimate the bandwidth. Before that we were using the
	// CAS counters of channel 0 and 1, corresponding to CCD0
	// and CCD0, and the L3 misses of both CCDs (CCX0, 1, 2
	// and 3). With that data, we got a coefficient to match
	// the L3 readings with the CAS values. That coefficient
	// were used with the other L3 chunks to convert to CAS
	// values.
	//
	// Problem. When is the coefficient good enough to
	// approximate the CAS values with the L3 counts? Because
	// the coefficient is variable every time is generated.
	//
	// Possible solution. Use the L3 values to bring the
	// bandwidth until a coefficient is good enough (100
	// iterations?) to convert L3 values to CAS values.
	
	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}

	#if 0
	// Reading the memory access channels of channels 0 and 1 of CCD0 and CCD1.
	// This is valid for CPUs with at least 2 CCDs.
	msr_read(bw->tp.cpus[0].id, &bw->cas_curr[0], sizeof(ulong), df_ctr0);
	msr_read(bw->tp.cpus[0].id, &bw->cas_curr[1], sizeof(ulong), df_ctr1);

	debug("CH0 counted %llu CAS", bw->cas_curr[0]);
	debug("CH1 counted %llu CAS", bw->cas_curr[1]);

	// Reading the L3 counters of CCX0, CCX1, CCX2, CCX3
	msr_read(bw->tp.cpus[0].id, &bw->l3_curr[0], sizeof(ulong), ctr_l3);
	msr_read(bw->tp.cpus[1].id, &bw->l3_curr[1], sizeof(ulong), ctr_l3);
	msr_read(bw->tp.cpus[2].id, &bw->l3_curr[2], sizeof(ulong), ctr_l3);
	msr_read(bw->tp.cpus[3].id, &bw->l3_curr[3], sizeof(ulong), ctr_l3);

	// Substracting data first (overflow safety).
	ulong aux_mem0 = bw->cas_curr[0] - bw->cas_prev[0];
	ulong aux_mem1 = bw->cas_curr[1] - bw->cas_prev[1];
	ulong aux_ccx0 = bw->l3_curr[0]  - bw->l3_prev[0];
	ulong aux_ccx1 = bw->l3_curr[1]  - bw->l3_prev[1];
	ulong aux_ccx2 = bw->l3_curr[2]  - bw->l3_prev[2];
	ulong aux_ccx3 = bw->l3_curr[3]  - bw->l3_prev[3];

	debug("cas0 diff %llu", aux_mem0);
	debug("cas1 diff %llu", aux_mem1);
	debug("l3-0 diff %llu", aux_ccx0);
	debug("l3-1 diff %llu", aux_ccx1);
	debug("l3-2 diff %llu", aux_ccx2);
	debug("l3-3 diff %llu", aux_ccx3);
	debug("cas sum %llu", aux_mem0+aux_mem1);
	debug("l3  sum %llu", aux_ccx0+aux_ccx1+aux_ccx2+aux_ccx3);
	
	// Detecting overflows
	int error = 
		(bw->cas_prev[0] >= bw->cas_curr[0]) ||
		(bw->cas_prev[1] >= bw->cas_curr[1]) ||
		(bw->l3_prev[0]  >= bw->l3_curr[0])  ||
		(bw->l3_prev[1]  >= bw->l3_curr[1])  ||
		(bw->l3_prev[2]  >= bw->l3_curr[2])  ||
		(bw->l3_prev[3]  >= bw->l3_curr[3]);

	// Saving the memory channel ticks for the next reading
	bw->cas_prev[0] = bw->cas_curr[0];
	bw->cas_prev[1] = bw->cas_curr[1];
	bw->l3_prev[0]  = bw->l3_curr[0];
	bw->l3_prev[1]  = bw->l3_curr[1];
	bw->l3_prev[2]  = bw->l3_curr[2];
	bw->l3_prev[3]  = bw->l3_curr[3];

	if (cas == NULL) {
		return EAR_SUCCESS;
	}

	// Comparing the percentage of L3 ticks respect the memory channel accesses
	double ccx_ticks = (double) (aux_ccx0 + aux_ccx1 + aux_ccx2 + aux_ccx3);
	double mem_ticks = (double) (aux_mem0 + aux_mem1);
	double aux_coeff = bw->coeff;
	double aux;
	
	if (!error) {
		// Improving coeff iteration per iteration
		bw->coeff       = bw->coeff + (mem_ticks / ccx_ticks); 
		bw->coeff_count = bw->coeff_count + 1.0;	
		aux_coeff       = bw->coeff / bw->coeff_count;
	}

	debug("coeff %lf of %lf", bw->coeff, bw->coeff_count);
	#endif

	for (i = 0; i < bw->fd_count; ++i)
	{
		// Reading each L3 miss counters
		s = msr_read(bw->tp.cpus[i].id, &cas[i], sizeof(ulong), ctr_l3);
		#if 0
		ulong readed = cas[i];

		if (!error) {
			aux = ((double) cas[i]) / ccx_ticks;
			aux = mem_ticks * aux;
			cas[i] = (ulong) aux;
		} else {
			cas[i] = 0;
		}
		
		debug("CCX%d, read L3 %llu, estimated %llu CAS", i, readed, cas[i]);
		#endif
	}
	
	// Remove warning
	(void) (s);

	return EAR_SUCCESS;
}
