/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/


#ifndef _DCGMI_LIB_COMMON_H_
#define _DCGMI_LIB_COMMON_H_


#include <common/types.h>
#include <common/states.h>

// #include <metrics/gpu/archs/dcgmi.h> // TODO: To be removed
#include <metrics/gpu/gpuprof.h>


#define DCGMI_LIB_SUPPORTED_EVENTS 12

#define dcgm_gr_engine_active      1001
#define dcgm_sm_active             1002
#define dcgm_sm_occupancy          1003
#define dcgm_tensor_active         1004
#define dcgm_dram_active           1005
#define dcgm_fp64_active           1006
#define dcgm_fp32_active           1007
#define dcgm_fp16_active           1008
#define dcgm_pcie_tx_bytes         1009
#define dcgm_pcie_rx_bytes         1010
#define dcgm_nvlink_tx_bytes       1011    
#define dcgm_nvlink_rx_bytes       1012
#define dcgm_tensor_imma_active    1013 // Currently not supported by EARL
#define dcgm_tensor_hmma_active    1014 // Currently not supported by EARL

#define dcgm_first_event_id				 dcgm_gr_engine_active

#define nvml_gr_engine_active      1
#define nvml_sm_active             2
#define nvml_sm_occupancy          3
#define nvml_tensor_active         5
#define nvml_dram_active           10
#define nvml_fp64_active           11
#define nvml_fp32_active           12
#define nvml_fp16_active           13
#define nvml_pcie_tx_bytes         20
#define nvml_pcie_rx_bytes         21
#define nvml_nvlink_tx_bytes       61    
#define nvml_nvlink_rx_bytes       60
#define nvml_tensor_imma_active    9 // Currently not supported by EARL
#define nvml_tensor_hmma_active    7 // Currently not supported by EARL


typedef struct dcgmi_lib_s
{
	int *dcgmi_set_sizes;				/**< Event set size for each set. */
	int	dcgmi_set_count;				/**< Event set count. */

	uint **dcgmi_event_ids;			/**< Event IDs for each set. */
	char **dcgmi_event_ids_csv;	/**< CSV event strings for each set. */

	gpuprof_t **gpuprof_read1;	/**< Event metrics for each set and device. */
	gpuprof_t **gpuprof_read2;	/**< Event metrics for each set and device. */
	gpuprof_t **gpuprof_diff;		/**< Event metrics for each set and device. */
	gpuprof_t **gpuprof_agg;		/**< Aggregated metrics for each set and device. */
} dcgmi_lib_t;


/** Allocates \ref dcgmi_data attributes based on \ref set_count.
 * You can use \ref state_msg to read the error message. */
state_t dcgmi_lib_data_alloc(dcgmi_lib_t *dcgmi_data, int set_count);


/** TODO: Implement this function. */
#if 0
state_t dcgmi_lib_data_free(dcgmi_lib_t *dcgmi_data);
#else
#define dcgmi_lib_data_free(...)
#endif


#endif // _DCGMI_LIB_COMMON_H_
