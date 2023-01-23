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

#ifndef METRICS_RAPL_H
#define METRICS_RAPL_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>

typedef struct rapl_s {
	ullong pack;
	ullong dram;
	ullong core;
} rapl_t;

state_t rapl_load(topology_t *tp);

void rapl_get_api(uint *api);

void rapl_get_granularity(uint *granularity);

state_t rapl_init(ctx_t *c);

state_t rapl_dispose(ctx_t *c);

state_t rapl_count_devices(ctx_t *c, uint *devs_count);

state_t rapl_read(ctx_t *c, rapl_t *b);

state_t rapl_read_diff(ctx_t *c, rapl_t *r1, rapl_t *r2, rapl_t *rD);

state_t rapl_read_copy(ctx_t *c, rapl_t *r1, rapl_t *r2, rapl_t *rD);

state_t rapl_data_diff(rapl_t *r2, rapl_t *r1, rapl_t *rD);

state_t rapl_data_alloc(rapl_t **r);

state_t rapl_data_copy(rapl_t *rd, rapl_t *rs);

void rapl_data_print(rapl_t *rD, int fd);

#endif //METRICS_RAPL_H
