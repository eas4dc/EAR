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

/**
*    \file node_metrics.h
*
*/
#ifndef _NODE_METRICS_H_
#define _NODE_METRICS_H_

#include <common/types/generic.h>
#include <common/hardware/topology.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/frequency/cpu.h>

typedef struct nm{
	uint ncpus;
    uint nsockets;
    ulong def_f;
	uint con;
}nm_t;

typedef struct node_metrics{
	cpufreq_t freq_cpu;
	imcfreq_t *freq_imc;
	ulong avg_cpu_freq;
	ulong avg_imc_freq;
	llong avg_temp;
	llong *temp;
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


#endif
