/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _PC_STATUS_CONF_H_
#define _PC_STATUS_CONF_H_


#include <stdint.h>
#include <common/types/powercap.h>


#define DOMAIN_NODE     0
#define DOMAIN_CPU      1
#define DOMAIN_DRAM     2
#define DOMAIN_GPU      3
#define DOMAIN_GPUS     DOMAIN_GPU
#define NUM_DOMAINS     4

#if USE_GPUS
#define GPU_PERC_UTIL 0.45
#else
#define GPU_PERC_UTIL 0
#endif

#define PC_DVFS     50
#define PC_POWER    51



#define PC_MODE_LIMIT   200
#define PC_MODE_TARGET  201				

#define PC_STATUS_ERROR 100

#define AUTO_CONFIG 0
#define EARL_CONFIG 1

#define TEMP_NUM_NODES 10

typedef struct node_powercap_opt {
    pthread_mutex_t lock;
    uint def_powercap;              /* powercap default value defined in ear.conf */
    uint powercap_idle;             /* powercap to be applied at idle periods */
    uint current_pc;                /* Current limit */
    uint last_t1_allocated;         /* Power allocated at last T1,guaranteed */
    uint released;                  /* Power released since last T1 */
    uint max_node_power;            /* Maximum node power */
    uint th_inc;                    /* Percentage to mark powercap_status as greedy */
    uint th_red;                    /* Percentage to mark powercap_status as released */
    uint th_release;                /* Percentage of power to be released */
    uint powercap_status;           /* Current status */
    uint cluster_perc_power;        /* Max extra power for new jobs */
    uint requested;                 /* Extra power requested, used when node is greedy or powercap < def_powercap */
    uint pper_domain[NUM_DOMAINS];  /* Power allocated to each domain */
} node_powercap_opt_t;

#define POWERCAP_STATUS_ACUM_ELEMS 8
typedef struct greedy_bytes {
    uint8_t     requested;      /* power requested */
    uint8_t     stress;         /* node stress */
    uint16_t    extra_power;    /* extra power already allocated */
} greedy_bytes_t;

typedef struct powercap_status{
    /* Static part */
    uint total_nodes;
    uint idle_nodes;            /* Total number of idle nodes */
    uint released;          	/* Accumulated released power in last T1 */
    uint requested;         	/*accumulated new_req */
    uint total_idle_power;      /* Total power allocated to idle nodes */
    uint current_power;         /* Accumulated power */
    uint total_powercap;        /* Accumulated current powercap limits */
    uint num_greedy;            /* Number of greedy nodes */
    /* Dynamic part */
    int *greedy_nodes;              /* List of greedy nodes */
    greedy_bytes_t *greedy_data;    /* Data from the greedy nodes */
} powercap_status_t;

#define TEMP_NUM_NODES 10

typedef struct powercap_opt{
    uint num_greedy;      /* Number of greedy nodes */
    int *greedy_nodes;     /* List of greedy nodes */
    int *extra_power;/* Extra power received by each greedy node */
    uint8_t cluster_perc_power; /* Percentage of total cluster power allocated */
} powercap_opt_t;

typedef struct dom_power{
    uint platform;
    uint cpu;
    uint dram;
    uint gpu;
} dom_power_t;

typedef struct pc_release_data{
    uint released;
} pc_release_data_t;


/*****
#define DOMAIN_NODE 0
#define DOMAIN_CPU 1
#define DOMAIN_DRAM     2
#define DOMAIN_GPU      3
 ****/

typedef struct domain_status{
    uint ok;
    uint exceed;
    uint current_pc;
    uint8_t stress;
    uint8_t requested;
} domain_status_t;

typedef struct pmgt_status {
    uint     status;
    uint     tbr;
    uint16_t extra;
    uint8_t  requested;
    uint8_t  stress;
} pmgt_status_t;

typedef struct domain_settings {
	float node_ratio;
	float security_range;
} domain_settings_t;


#endif
