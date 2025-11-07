/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_H
#define METRICS_H

#include <common/hardware/topology.h>
#include <common/plugins.h>
#include <common/states.h>
#include <common/types/generic.h>
#include <common/utils/data_register.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/cache/cache.h>
#include <metrics/cpi/cpi.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/energy/energy_node.h>
#include <metrics/energy_cpu/energy_cpu.h>
#include <metrics/flops/flops.h>
#include <metrics/gpu/gpu.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/temperature/temperature.h>

typedef struct metrics_info_s {
    apinfo_t cpu; // cpufreq
    apinfo_t imc; // imcfreq
    apinfo_t ram; // bwidth
    apinfo_t cch; // cache
    apinfo_t flp; // flops
    apinfo_t tmp; // temp
    apinfo_t pow; // energy_cpu
    apinfo_t cpi; // cpi
    apinfo_t gpu; // gpu
    apinfo_t nod; // energy_node
} metrics_info_t;

typedef struct metrics_read_s {
    ullong samples;
    timestamp_t time;
    cpufreq_t *cpu;
    imcfreq_t *imc;
    bwidth_t *ram;
    cache_t cch;
    flops_t flp;
    llong *tmp;
    ullong *pow;
    cpi_t cpi;
    gpu_t *gpu;
    char *nod;
} metrics_read_t;

typedef struct metrics_diff_s {
    ullong samples;
    double time;      // Seconds
    ulong *cpu_diff;  // KHz (per device)
    ulong cpu_avrg;   // KHz
    ulong *imc_diff;  // KHz (per device)
    ulong imc_avrg;   // KHz
    ullong ram_diff;  // CAS
    double ram_avrg;  // GB/s
    cache_t cch_diff; // Misses
    double cch_avrg;  // GB/s
    flops_t flp_diff; // F. instructions
    double flp_avrg;  // Giga F. operations per second
    llong *tmp_diff;
    llong tmp_avrg;
    ullong *pow_diff;
    ullong pow_dram; // Average per DRAM
    ullong pow_pack; // Average per PACKAGE
    cpi_t cpi_diff;
    double cpi_avrg; // GPI
    gpu_t *gpu_diff;
    ulong nod_avrg; // Total node energy
} metrics_diff_t;

#define MET_CPUFREQ                0LLU  // 0x0000000001
#define MET_IMCFREQ                4LLU  // 0x0000000010
#define MET_BWIDTH                 8LLU  // 0x0000000100
#define MET_FLOPS                  12LLU // 0x0000001000
#define MET_CACHE                  16LLU // 0x0000010000
#define MET_TEMP                   20LLU // 0x0000100000
#define MET_CPI                    24LLU // 0x0001000000
#define MET_GPU                    28LLU // 0x0010000000
#define MET_CPUPOW                 32LLU // 0x0100000000
#define MET_NODEPOW                36LLU // 0x1000000000

#define MFLAG_UNZIP(mflag, metric) (int) ((mflag >> metric) & 0xF)

// This function contains load() and init() functions of the different metrics.
void metrics_load(metrics_info_t *m, topology_t *tp, char *nodepow_path, ullong mflags);

// This function is composed by get_info() functions of the different metrics.
// It is perfectly safe to call metrics_info_get() or single xxx_get_info()
// functions just after calling load(). It is not required to wait to call
// init() because the data returned by get_info() is set during load() calling.
void metrics_info_get(metrics_info_t *m);

char *metrics_info_tostr(metrics_info_t *m, char *buffer);

void metrics_info_print(metrics_info_t *m, int fd);

void metrics_read(metrics_read_t *mr);

void metrics_read_copy(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

void metrics_data_alloc(metrics_read_t *mr1, metrics_read_t *mr2, metrics_diff_t *mrD);

void metrics_data_diff(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD);

void metrics_data_copy(metrics_read_t *mrD, metrics_read_t *mrS);

void metrics_data_print(metrics_diff_t *mrD, int fd);

char *metrics_data_tostr(metrics_diff_t *mrD);

#endif // METRICS_H
