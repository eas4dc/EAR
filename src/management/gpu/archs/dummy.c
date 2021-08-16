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

#include <stdlib.h>
#include <management/gpu/archs/dummy.h>

static ulong  clock_khz;
static ulong *clock_list[1]; // MHz
static uint	  clock_lens[1];

state_t mgt_dummy_status()
{
	return 1;
}

state_t mgt_dummy_init(ctx_t *c)
{
	clock_list[0] = &clock_khz;
	clock_lens[0] = 1;
	clock_khz     = 0;
	return EAR_SUCCESS;
}

state_t mgt_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_dummy_count(ctx_t *c, uint *_dev_count)
{
	if (_dev_count != NULL) {
		*_dev_count = 1;
	}
	return EAR_SUCCESS;
}

state_t dummy_freq_limit_get_current(ctx_t *c, ulong *khz)
{
	if (khz != NULL) {
		khz[0] = 0;
	}
	return EAR_SUCCESS;
}

state_t dummy_freq_limit_get_default(ctx_t *c, ulong *khz)
{
	if (khz != NULL) {
		khz[0] = 0;
	}
	return EAR_SUCCESS;
}

state_t dummy_freq_limit_get_max(ctx_t *c, ulong *khz)
{
	if (khz != NULL) {
		khz[0] = 0;
	}
	return EAR_SUCCESS;
}

state_t dummy_freq_limit_reset(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t dummy_freq_limit_set(ctx_t *c, ulong *khz)
{
	if (khz != NULL) {
		khz[0] = 0;
	}
	return EAR_SUCCESS;
}

state_t dummy_freq_get_valid(ctx_t *c, uint d, ulong freq_ref, ulong *freq_near)
{
	if (freq_near != NULL) {
		*freq_near = 0;
	}
	return EAR_SUCCESS;
}

state_t dummy_freq_get_next(ctx_t *c, uint d, ulong freq_ref, uint *freq_idx, uint flag)
{
	if (freq_idx != NULL) {
		*freq_idx = 0;
	}
	return EAR_SUCCESS;
}

state_t dummy_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
	if (list_khz != NULL) {
		*list_khz = (const ulong **) clock_list;
	}
	if (list_len != NULL) {
		*list_len = (const uint *) clock_lens;
	}
	return EAR_SUCCESS;
}

state_t dummy_power_cap_get_current(ctx_t *c, ulong *watts)
{
	if (watts != NULL) {
		watts[0] = 0;
	}
	return EAR_SUCCESS;
}

state_t dummy_power_cap_get_default(ctx_t *c, ulong *watts)
{
	if (watts != NULL) {
		watts[0] = 0;
	}
	return EAR_SUCCESS;
}

state_t dummy_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
	if (watts_max != NULL) {
		watts_max[0] = 0;
	}
	if (watts_min != NULL) {
		watts_min[0] = 0;
	}
	return EAR_SUCCESS;
}

state_t dummy_power_cap_reset(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t dummy_power_cap_set(ctx_t *c, ulong *watts)
{
	if (watts != NULL) {
		watts[0] = 0;
	}
	return EAR_SUCCESS;
}
