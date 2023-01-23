/*

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


#ifndef METRICS_ENERGY_CPU_DUMMY_H
#define METRICS_ENERGY_CPU_DUMMY_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <metrics/common/pci.h>
#include <metrics/common/msr.h>
#include <metrics/common/apis.h>
#include <metrics/energy_cpu/energy_cpu.h>
#include <common/hardware/topology.h>

state_t energy_cpu_dummy_load(topology_t *tp_in);

state_t energy_cpu_dummy_get_granularity(ctx_t *c, uint *granularity);

state_t energy_cpu_dummy_init(ctx_t *c);

state_t energy_cpu_dummy_dispose(ctx_t *c);

state_t energy_cpu_dummy_count_devices(ctx_t *c, uint *devs_count_in);

state_t energy_cpu_dummy_read(ctx_t *c, ullong *values);

#endif
