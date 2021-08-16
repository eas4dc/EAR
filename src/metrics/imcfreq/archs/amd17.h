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

#ifndef METRICS_IMCFREQ_AMD17_H
#define METRICS_IMCFREQ_AMD17_H

#include <metrics/imcfreq/imcfreq.h>

void imcfreq_amd17_load(topology_t *tp, imcfreq_ops_t *ops, int eard);

void imcfreq_amd17_get_api(uint *api, uint *api_intern);

state_t imcfreq_amd17_init(ctx_t *c);

state_t imcfreq_amd17_init_static(ctx_t *c);

state_t imcfreq_amd17_dispose(ctx_t *c);

state_t imcfreq_amd17_count_devices(ctx_t *c, uint *dev_count);
/** */
state_t imcfreq_amd17_read(ctx_t *c, imcfreq_t *i);

state_t imcfreq_amd17_data_diff(imcfreq_t *i2, imcfreq_t *i1, ulong *freq_list, ulong *average);

#endif //METRICS_IMCFREQ_DUMMY_H
