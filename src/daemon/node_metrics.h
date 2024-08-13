/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/**
*    \file node_metrics.h
*
*/
#ifndef _NODE_METRICS_H_
#define _NODE_METRICS_H_

#include <common/config.h>
#include <common/types/generic.h>
#include <common/hardware/topology.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/cpufreq/cpufreq.h>

typedef struct nm{
	uint ncpus;
    uint nsockets;
    ulong def_f;
	uint con;
}nm_t;

typedef struct node_metrics{
    uint       freq_cpu_count;
	cpufreq_t *freq_cpu;
    ulong     *freq_cpu_diff; //avg_per_cpu
    uint       freq_imc_count;
	imcfreq_t *freq_imc;
	ulong      avg_cpu_freq;
	ulong      avg_imc_freq;
	llong      avg_temp;
	llong     *temp;
} nm_data_t;

int init_node_metrics(nm_t *id, topology_t *topo, ulong def_freq);
int init_node_metrics_data(nm_t *id,nm_data_t *mm);
int start_compute_node_metrics(nm_t *id,nm_data_t *nm);
int end_compute_node_metrics(nm_t *id,nm_data_t *nm);
int dispose_node_metrics(nm_t *id);


int diff_node_metrics(nm_t *id,nm_data_t *init,nm_data_t *end,nm_data_t *diff_nm);
int copy_node_metrics(nm_t *id,nm_data_t* dest, nm_data_t * src);
int print_node_metrics(nm_t *id,nm_data_t *nm);
int verbose_node_metrics(nm_t *id,nm_data_t *nm);
unsigned long long get_nm_temp(nm_t *id,nm_data_t *nm);
ulong get_nm_cpufreq(nm_t *id,nm_data_t *nm);
ulong get_nm_cpufreq_with_mask(nm_t *id,nm_data_t *nm, cpu_set_t m);



#endif
