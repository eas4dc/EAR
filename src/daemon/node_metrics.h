/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/


/**
*    \file node_metrics.h
*
*/
#ifndef _NODE_METRICS_H_
#define _NODE_METRICS_H_

#include <common/types/generic.h>


typedef struct nm{
	uint ncpus;
    uint nsockets;
    ulong def_f;
	uint con;
}nm_t;

typedef struct node_metrics{
    ulong avg_cpu_freq;
    uint64_t *uncore_freq;
    unsigned long long *temp;  
}nm_data_t;

int init_node_metrics(nm_t *id,int sockets, int cpus_per_socket,int cores_model,ulong def_freq);
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
