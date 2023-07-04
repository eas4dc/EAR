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

#ifndef METRICS_GPU_DCGMI_H
#define METRICS_GPU_DCGMI_H

#include <metrics/gpu/gpu.h>

/*
 * dcgmi profile -l
 * +----------------+----------+------------------------------------------------------+
| Group.Subgroup | Field ID | Field Tag                                            |
+----------------+----------+------------------------------------------------------+
| A.1            | 1002     | sm_active                                            |
| A.1            | 1003     | sm_occupancy                                         |
| A.1            | 1004     | tensor_active                                        |
| A.1            | 1006     | fp64_active                                          |
| A.2            | 1008     | fp16_active                                          |
| A.2            | 1013     | tensor_imma_active                                   |
| A.2            | 1014     | tensor_hmma_active                                   |
| A.3            | 1007     | fp32_active                                          |
| B.0            | 1005     | dram_active                                          |
| C.0            | 1009     | pcie_tx_bytes                                        |
| C.0            | 1010     | pcie_rx_bytes                                        |
| D.0            | 1001     | gr_engine_active                                     |
| E.0            | 1011     | nvlink_tx_bytes                                      |
| E.0            | 1012     | nvlink_rx_bytes                                      |
+----------------+----------+------------------------------------------------------+
*/

/* for now, we don't need a more complex structure since event numbers
 * are consecutive
 */
#define dcgmi_first_event     1001
/* There are less events, but we wil do it simpler */
#define dcgmi_num_events      14

#define sm_active             1002
#define sm_occupancy          1003
#define tensor_active         1004
#define fp64_active           1006
#define fp16_active           1008
#define tensor_imma_active    1013
#define tensor_hmma_active    1014
#define fp32_active           1007
#define dram_active           1005
#define pcie_tx_bytes         1009
#define pcie_rx_bytes         1010
#define gr_engine_active      1001
#define nvlink_tx_bytes       1011    
#define nvlink_rx_bytes       1012

#define RATIO 1
#define ACCUM 0

typedef struct dcgmi_s {
    double values[15];
} dcgmi_t;

state_t gpu_dcgmi_init(char *events, uint events_count);

state_t gpu_dcgmi_read(dcgmi_t *d);

void gpu_dcgmi_data_alloc(dcgmi_t **d);
void gpu_dcgmi_events_to_str(uint *dcgmi_events, uint num_ev,char * dcgmi_events_str);
void gpu_dcgmi_data_str(dcgmi_t *d, char *d_str, uint *ev_list, uint num_ev);
void gpu_dcgmi_data_copy(dcgmi_t *dest, dcgmi_t *src);

/* This function changes the command to be executed
 * warning: it is user responsabiity to allocate data with the right number of events
 * since the API is not taking that into account.
 */
void gpu_dcgmi_set_events(char *events, uint events_count_in);

/* Accumulates d in accum for elapsed period */
void gpu_dcgmi_accum_data(dcgmi_t *accum, dcgmi_t *d, double elapsed, uint *ev_list, uint num_ev);
/* Given an accumulated elpased time, computes the average for each event value and GPU */
void gpu_dcgmi_data_avg(dcgmi_t *accum, double elapsed, uint *ev_list, uint num_ev);
/* Returns RATIO or ACUM depending on the event. RATIO means is a ratio of activity (percentage?);
 * ACCUM means is an absolute  value 
 */
uint gpu_dcgmi_event_type(uint event);
/* DCGMI detected GPUs can be different than NVML */
uint gpu_dcgmi_gpus_detected();
/* Returns the name of the event */
char *gpu_dcgmi_event_to_name(uint ev);
#endif //METRICS_GPU_DCGMI_H
