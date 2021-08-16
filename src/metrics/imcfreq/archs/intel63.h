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

#ifndef METRICS_IMCFREQ_INTEL63_H
#define METRICS_IMCFREQ_INTEL63_H

#include <metrics/imcfreq/imcfreq.h>

void imcfreq_intel63_load(topology_t *tp, imcfreq_ops_t *ops);

void imcfreq_intel63_get_api(uint *api, uint *api_intern);

state_t imcfreq_intel63_init(ctx_t *c);

state_t imcfreq_intel63_dispose(ctx_t *c);

state_t imcfreq_intel63_count_devices(ctx_t *c, uint *dev_count);
/** */
state_t imcfreq_intel63_read(ctx_t *c, imcfreq_t *i);

#endif //METRICS_IMCFREQ_INTEL63_H
