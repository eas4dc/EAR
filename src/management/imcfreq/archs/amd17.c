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

#include <metrics/common/hsmp.h>
#include <management/imcfreq/archs/amd17.h>

static char *err1 = "Incorrect result when asking for frequency by HSMP";
static ullong freqs_khz[4];
static ullong freqs_mhz[4];
static uint sockets_count;
static uint pstate_count;

state_t mgt_imcfreq_amd17_load(topology_t *tp, mgt_imcfreq_ops_t *ops)
{
	struct timespec ten_ms = { 0, 1000 * 1000 * 10};
	state_t s;
	int i, j;

	// Already loaded
	if (state_fail(s = hsmp_scan(tp))) {
		return s;
	}
	// Getting the list of available frequencies
	uint args[2] = {  0, -1 };
	uint reps[3] = {  0,  0, -1 };

	for (i = j = 0; i < 4; ++i, ++j) {
		// Arg0 is the P_STATE for the APBDisable function (0x0d).
		args[0] = i;
		// Function APBDisable (0x0d).
		// Function ReadCurrentFclkMemclk (0x0f).
		if (state_fail(s = hsmp_send(0, 0x0d, &args[0], &reps[2]))) {
			debug("error while sending HSMP: %s", state_msg);
		}
		// Waiting 10 miliseconds to change the frequency
		nanosleep(&ten_ms, NULL);
		//
		if (state_fail(s = hsmp_send(0, 0x0f, &args[1], &reps[0]))) {
			debug("error while sending HSMP: %s", state_msg);
		}
		// Try again until the answer is different
		if (j >= 10000) {
			// Out of intents
			break;
		} else if (i > 0 && reps[0] == (uint) freqs_mhz[i-1]) {
			--i;
		} else {
			debug("PS%d: answered %u/%u (%u intents)", i, reps[0], reps[1], j);
			// If its a weird number...
			if ((reps[0] == 0) || (reps[0] == -1)) {
				return_msg(EAR_ERROR, err1);
			}
			freqs_mhz[i] = ((ullong) reps[0]);
			freqs_khz[i] = freqs_mhz[i] * 1000LLU;
			j = 0;
		}
	}
	pstate_count = i;
	// Setting auto
	hsmp_send(0, 0x0e, &args[1], &reps[2]);
	//
	replace_ops(ops->init,             mgt_imcfreq_amd17_init);
	replace_ops(ops->dispose,          mgt_imcfreq_amd17_dispose);
	replace_ops(ops->get_available_list, mgt_imcfreq_amd17_get_available_list);
	replace_ops(ops->get_current_list, mgt_imcfreq_amd17_get_current_list);
	replace_ops(ops->set_current_list, mgt_imcfreq_amd17_set_current_list);
	replace_ops(ops->set_current,      mgt_imcfreq_amd17_set_current);
	replace_ops(ops->set_auto,         mgt_imcfreq_amd17_set_auto);
	replace_ops(ops->get_current_ranged_list, mgt_imcfreq_amd17_get_current_ranged_list);
    replace_ops(ops->set_current_ranged_list, mgt_imcfreq_amd17_set_current_ranged_list);
	// Saving sockets
	sockets_count = tp->socket_count;

	return EAR_SUCCESS;
}

state_t mgt_imcfreq_amd17_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_amd17_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_amd17_get_available_list(ctx_t *c, const pstate_t **list, uint *count)
{
	static pstate_t ps[32];
	uint i;

	for (i = 0; i < pstate_count; ++i) {
		ps[i].khz = freqs_khz[i];
		ps[i].idx = i;
	}
	*list = ps;
	if (count != NULL) {
		*count = pstate_count;
	}
	return EAR_SUCCESS;
}

static uint find_index(ullong khz)
{
	int i;
	for (i = 0; i < 4; ++i) {
		if (freqs_khz[i] == khz) {
			return i;
		}
		if (freqs_khz[i] < khz) {
			return i;
		}
	}
	return 0;
}

state_t mgt_imcfreq_amd17_get_current_list(ctx_t *c, pstate_t *list)
{
	uint args[3] = {  0,  0, -1 };
	ullong freq;
	state_t s;
	uint i;

	for (i = 0; i < sockets_count; ++i) {
		// Function ReadCurrentFclkMemclk (0x0f). 0 arguments, 2 answers.
		//args[0] = 0;
		//args[1] = 0;
		if (state_fail(s = hsmp_send(i, 0x0f, &args[2], &args[0]))) {
			return s;
		}
		if (args[0] == 0 || args[0] == -1) {
			return_msg(EAR_ERROR, err1);
		}
		freq = ((ullong) args[0]) * 1000LLU;
		list[i].idx = find_index(freq);
		list[i].khz = freqs_khz[list[i].idx];
		debug("SOCKET%d: %llu KHz (id %u)", i, list[i].khz, list[i].idx);
	}
	return EAR_SUCCESS;
}

static state_t set(uint pstate_index, int socket)
{
	uint args[2] = {  0, -1 };
	uint function, i;
	state_t s;

	if (pstate_index == ps_auto) {
		// Function APBEnable (0x0e)
		function = 0x0e;
		i = 1;
	} else if (pstate_index >= pstate_count) {
		return_msg(EAR_ERROR, Generr.arg_outbounds);
	} else {
		// Function APBDisable (0x0d)
		function = 0x0d;
		i = 0;
	}
	args[0] = pstate_index;
	debug("Setting P%u (%u) to socket %d", pstate_index, args[0], socket);
	if (state_fail(s = hsmp_send(socket, function, &args[i], &args[1]))) {
		debug("Failed while setting IMC frequency set: %s (%d)", state_msg, s);
		return s;
	}
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_amd17_set_current_list(ctx_t *c, uint *index_list)
{
	state_t s;
	uint i;

	for (i = 0; i < sockets_count; ++i) {
		if (index_list[i] == ps_nothing) {
			continue;
		}
		if (state_fail(s = set(index_list[i], i))) {
			return s;
		}
	}
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_amd17_set_current(ctx_t *c, uint pstate_index, int socket)
{
	state_t s;
	uint i;
	
	if (pstate_index == ps_nothing) {
		return EAR_SUCCESS;
	}
	for (i = 0; i < sockets_count; ++i) {
		if (i != socket && socket != all_sockets) {
			continue;
		}
		if (state_fail(s = set(pstate_index, i))) {
			return s;
		}
	}
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_amd17_set_auto(ctx_t *c)
{
	return mgt_imcfreq_amd17_set_current(c, ps_auto, all_sockets);
}

state_t mgt_imcfreq_amd17_get_current_ranged_list(ctx_t *c, pstate_t *ps_min_list, pstate_t *ps_max_list)
{
    return mgt_imcfreq_amd17_get_current_list(c, ps_min_list);
}

state_t mgt_imcfreq_amd17_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list)
{
    return mgt_imcfreq_amd17_set_current_list(c, id_min_list);
}

