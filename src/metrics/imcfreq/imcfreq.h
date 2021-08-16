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

#ifndef METRICS_IMCFREQ_H
#define METRICS_IMCFREQ_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <common/hardware/topology.h>
#include <metrics/common/apis.h>
#include <metrics/common/pstate.h>

typedef struct imcfreq_s {
	timestamp_t time;
	ulong freq; // KHz
	uint error;
} imcfreq_t;

typedef struct imcfreq_ops_s
{
	void    (*get_api)        (uint *api, uint *api_intern);
	state_t (*init)           (ctx_t *c);
	state_t (*init_static[4]) (ctx_t *c);
	state_t (*dispose)        (ctx_t *c);
	state_t (*count_devices)  (ctx_t *c, uint *dev_count);
	state_t (*read)           (ctx_t *c, imcfreq_t *reg_list);
	state_t (*data_alloc)     (imcfreq_t **reg_list, ulong **freq_list);
	state_t (*data_free)      (imcfreq_t **reg_list, ulong **freq_list);
	state_t (*data_copy)      (imcfreq_t *reg_list2, imcfreq_t *reg_list1);
	state_t (*data_diff)      (imcfreq_t *reg_list2, imcfreq_t *reg_list, ulong *freq_list, ulong *freq_avg);
	void    (*data_print)     (ulong *freq_list, ulong *freq_avg, int fd);
	void    (*data_tostr)     (ulong *freq_list, ulong *freq_avg, char *buffer, size_t length);
} imcfreq_ops_t;

// Frequency is KHz
void imcfreq_load(topology_t *tp, int eard);

void imcfreq_get_api(uint *api);

state_t imcfreq_init(ctx_t *c);

state_t imcfreq_dispose(ctx_t *c);

state_t imcfreq_count_devices(ctx_t *c, uint *dev_count);

state_t imcfreq_read(ctx_t *c, imcfreq_t *reg_list);

state_t imcfreq_read_diff(ctx_t *c, imcfreq_t *reg_list2, imcfreq_t *reg_list1, ulong *freq_list, ulong *freq_avg);

state_t imcfreq_read_copy(ctx_t *c, imcfreq_t *reg_list2, imcfreq_t *reg_list1, ulong *freq_list, ulong *freq_avg);

// Helpers
state_t imcfreq_data_alloc(imcfreq_t **reg_list, ulong **freq_list);

state_t imcfreq_data_free(imcfreq_t **reg_list, ulong **freq_list);

state_t imcfreq_data_copy(imcfreq_t *reg_list2, imcfreq_t *reg_list1);

state_t imcfreq_data_diff(imcfreq_t *reg_list2, imcfreq_t *reg_list1, ulong *freq_list, ulong *freq_avg);

void imcfreq_data_print(ulong *freq_list, ulong *freq_avg, int fd);

void imcfreq_data_tostr(ulong *freq_list, ulong *freq_avg, char *buffer, size_t length);

#endif //METRICS_IMCFREQ_H
