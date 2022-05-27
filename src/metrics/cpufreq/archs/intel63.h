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

#ifndef METRICS_CPUFREQ_PERF_H
#define METRICS_CPUFREQ_PERF_H

#include <metrics/cpufreq/cpufreq.h>

state_t cpufreq_intel63_status(topology_t *tp, cpufreq_ops_t *ops);

state_t cpufreq_intel63_init(ctx_t *c);

state_t cpufreq_intel63_dispose(ctx_t *c);

state_t cpufreq_intel63_count_devices(ctx_t *c, uint *cpu_count);

state_t cpufreq_intel63_read(ctx_t *c, cpufreq_t *f);

state_t cpufreq_intel63_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *freqs, ulong *average);

#endif //METRICS_CPUFREQ_PERF_H
