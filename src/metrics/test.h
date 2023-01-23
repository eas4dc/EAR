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
#include <metrics/rapl/rapl.h>
#include <metrics/flops/flops.h>
#include <metrics/cache/cache.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/temperature/temperature.h>

typedef struct apis_info_s {
	ctx_t c;
	uint  api;
    char  api_str[32];
	uint  gran; // Granularity
	char  gran_str[32];
	uint  devs_count;
    void *list1;
    void *list2;
    void *list3;
    uint  list1_count;
    uint  list2_count;
    uint  list3_count;
} apis_info_t;

typedef struct metrics_apis_s {
	apis_info_t cpu; // cpufreq
	apis_info_t imc; // imcfreq
    apis_info_t ram; // bwidth
    apis_info_t cch; // cache
    apis_info_t flp; // flops
    apis_info_t tmp; // temp
    apis_info_t rap; // rapl
    apis_info_t cpi; // cpi
	apis_info_t gpu; // gpu
} metrics_apis_t;

typedef struct metrics_read_s {
	timestamp_t time;
	cpufreq_t  *cpu;
	imcfreq_t  *imc;
    bwidth_t   *ram;
    cache_t     cch;
    flops_t     flp;
    rapl_t     *rap;
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
    dreg_t  tmp_dreg;
    rapl_t *rap_diff;
    dreg_t  rap_dreg;
    cpi_t   cpi_diff;
    double  cpi_avrg; // GPI
	gpu_t  *gpu_diff;
} metrics_diff_t;

void metrics_init(topology_t *tp, int eard);

void metrics_read(metrics_read_t *mr);

void metrics_read_diff(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

void metrics_read_copy(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

void metrics_data_alloc(metrics_read_t *mr1, metrics_read_t *mr2, metrics_diff_t *mrD);

void metrics_data_print(metrics_diff_t *mrD);

#endif //TEST_H
