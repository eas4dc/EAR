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

#ifndef METRICS_H
#define METRICS_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/types/generic.h>
#include <common/hardware/topology.h>
#include <metrics/gpu/gpu.h>
#include <metrics/cpi/cpi.h>
#include <metrics/flops/flops.h>
#include <metrics/cache/cache.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/bandwidth/bandwidth.h>

typedef struct apis_info_s {
	ctx_t c;
	uint api;
	uint working;
	uint granularity;
	uint devs_count;
} apis_info_t;

typedef struct metrics_apis_s {
	apis_info_t cpu; // cpufreq
	apis_info_t imc; // imcfreq
    apis_info_t ram; // bwidth
    apis_info_t cch; // cache
    apis_info_t flp; // flops
    apis_info_t cpi; // cpi
#if NOT_READY
	apis_info_t gpu; // gpu
#endif
} metrics_apis_t;

typedef struct metrics_read_s {
	timestamp_t time;
	cpufreq_t  *cpu;
	imcfreq_t  *imc;
    bwidth_t   *ram;
    cache_t     cch;
    flops_t     flp;
    cpi_t       cpi;
#if NOT_READY
	gpu_t      *gpu;
#endif
} metrics_read_t;

typedef struct metrics_diff_s {
	ullong  time; // Seconds
	ulong  *cpu_diff; // KHz (per device)
	ulong   cpu_avrg; // KHz
	ulong  *imc_diff; // KHz (per device)
	ulong   imc_avrg; // KHz
    ullong  ram_diff; // CAS
    double  ram_avrg; // GB/s
    cache_t cch_diff; // Misses
    double  cch_avrg; // GB/s
    flops_t flp_diff; // F. instructions
    double  flp_avrg; // Giga F. operations per second
    cpi_t   cpi_diff;
    double  cpi_avrg; // GPI
#if NOT_READY
	gpu_t  *gpu_diff;
#endif
} metrics_diff_t;

state_t metrics_apis_init(topology_t *tp, uint eard, metrics_apis_t **m);

state_t metrics_apis_dispose();

state_t metrics_apis_print();

state_t metrics_read_alloc(metrics_read_t *mr);

state_t metrics_read_free(metrics_read_t *mr);

state_t metrics_read(metrics_read_t *mr);

state_t metrics_read_diff(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

state_t metrics_read_copy(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

state_t metrics_diff_alloc(metrics_diff_t *md);

state_t metrics_diff_free(metrics_read_t *md);

state_t metrics_diff(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

state_t metrics_diff_print(metrics_diff_t *mrD);

#endif //METRICS_H