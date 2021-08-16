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

#ifndef METRICS_CACHE_H
#define METRICS_CACHE_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>

state_t cache_load(topology_t *tp);

state_t cache_init();

state_t cache_dispose();

state_t cache_reset();

state_t cache_start();

state_t cache_stop(llong *L1_misses, llong *LL_misses);

state_t cache_read(llong *L1_misses, llong *LL_misses);

state_t cache_data_print(llong L1_misses, llong LL_misses);

/* This is an obsolete function to make metrics.c compatible. */
void get_cache_metrics(llong *L1_misses, llong *LL_misses);

#endif //METRICS_CACHE_H
