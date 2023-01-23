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

#ifndef METRICS_GPU_EARD_H
#define METRICS_GPU_EARD_H

#include <metrics/gpu/gpu.h>

void gpu_eard_load(gpu_ops_t *ops, int eard);

void gpu_eard_get_api(uint *api);

state_t gpu_eard_init(ctx_t *c);

state_t gpu_eard_dispose(ctx_t *c);

state_t gpu_eard_count_devices(ctx_t *c, uint *dev_count);

state_t gpu_eard_read(ctx_t *c, gpu_t *data);

state_t gpu_eard_read_raw(ctx_t *c, gpu_t *data);

int gpu_eard_is_supported();

#endif
