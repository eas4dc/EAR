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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef METRICS_H
#define METRICS_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/types/generic.h>
#include <common/hardware/topology.h>
#include <common/utils/data_register.h>
#include <metrics/gpu/gpu.h>
#include <metrics/cpi/cpi.h>
#include <metrics/flops/flops.h>
#include <metrics/cache/cache.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/temperature/temperature.h>

typedef struct metrics_info_s {
    apinfo_t cpu; // cpufreq
    apinfo_t imc; // imcfreq
    apinfo_t ram; // bwidth
    apinfo_t cch; // cache
    apinfo_t flp; // flops
    apinfo_t tmp; // temp
    apinfo_t cpi; // cpi
    apinfo_t gpu; // gpu
} metrics_info_t;

typedef struct metrics_read_s {
	timestamp_t time;
	cpufreq_t  *cpu;
	imcfreq_t  *imc;
    bwidth_t   *ram;
    cache_t     cch;
    flops_t     flp;
    cpi_t       cpi;
	gpu_t      *gpu;
} metrics_read_t;

typedef struct metrics_diff_s {
	ullong  time; // Seconds
	ulong  *cpu_diff; // KHz (per device)
	ulong   cpu_avrg; // KHz
	ulong  *imc_diff; // KHz (per device)
	ulong   imc_avrg; // KHz
    dreg_t  imc_dreg;
    ullong  ram_diff; // CAS
    double  ram_avrg; // GB/s
    cache_t cch_diff; // Misses
    double  cch_avrg; // GB/s
    flops_t flp_diff; // F. instructions
    double  flp_avrg; // Giga F. operations per second
    llong  *tmp_diff;
    llong   tmp_avrg;
    cpi_t   cpi_diff;
    double  cpi_avrg; // GPI
	gpu_t  *gpu_diff;
} metrics_diff_t;

void metrics_init(metrics_info_t *m, topology_t *tp, int eard);

void metrics_read(metrics_read_t *mr);

void metrics_read_copy(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

void metrics_data_alloc(metrics_read_t *mr1, metrics_read_t *mr2, metrics_diff_t *mrD);

#endif //METRICS_H
