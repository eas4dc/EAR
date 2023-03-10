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
