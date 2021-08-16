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

#ifndef METRICS_TEMPERATURE_ARCHS_INTEL63
#define METRICS_TEMPERATURE_ARCHS_INTEL63

#include <metrics/temperature/temperature.h>

state_t temp_intel63_status(topology_t *tp);

state_t temp_intel63_init(ctx_t *s);

state_t temp_intel63_dispose(ctx_t *s);

// Data
state_t temp_intel63_count_devices(ctx_t *c, uint *count);

// Getters
state_t temp_intel63_read(ctx_t *s, llong *temp, llong *average);

#endif //METRICS_TEMPERATURE_ARCHS_INTEL63
