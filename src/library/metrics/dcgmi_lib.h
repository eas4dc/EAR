/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/


#ifndef _DCGMI_LIB_H_
#define _DCGMI_LIB_H_


#include <common/states.h>
#include <common/types.h>
#include <common/types/signature.h>

#include <metrics/gpu/gpuprof.h>

#include <library/metrics/dcgmi_lib/common.h>

// Index of event_info array that contains
// the set that a particular event belongs to.
#define event_info_set 0

// Index of event_info array that contains the position
// in the set that a particular event belongs to.
#define event_info_pos 1


/** A signature type which contains GPU gpuprof metrics. */
typedef struct dcgmi_sig
{
	uint			set_cnt;								/**< The total number of sets. */
	uint			gpu_cnt;								/**< The number of GPUs supported by DCGMI module. */
	uint			*set_instances_cnt;			/**< The number of instances for each set. */
	float			*gpu_gflops;						/**< The GFLOPS rate for each GPU. */
	gpuprof_t	**event_metrics_sets;		/**< For each set, and for each GPU, the event metrics list. */
} dcgmi_sig_t;

typedef enum dcgm_event_id
{
	gr_engine_active,
	sm_active,
	sm_occupancy,
	tensor_active,
	dram_active,
	fp64_active,
	fp32_active,
	fp16_active,
	pcie_tx_bytes,
	pcie_rx_bytes,
	nvlink_tx_bytes,
	nvlink_rx_bytes,
	event_id_max // error control
} dcgm_event_idx_t;


/** Loads this module. */
state_t dcgmi_lib_load();


/** Returns whether this module is enabled. */
uint dcgmi_lib_is_enabled();


// TODO: An analogous dispose should be implemented.
void dcgmi_lib_dcgmi_sig_init(dcgmi_sig_t *dcgmi_sig);


/** Fills \p buffer with a semi-colon separated header
 *	of the DCGMI data capable to report this module. */
state_t dcgmi_lib_dcgmi_sig_csv_header(char *buffer, size_t buff_size);


state_t dcgmi_lib_dcgmi_sig_to_csv(dcgmi_sig_t *dcgmi_sig, char *buffer, size_t buff_size);


state_t dcgmi_lib_get_current_metrics(dcgmi_sig_t *dcgmi_sig);


state_t dcgmi_lib_get_global_metrics(dcgmi_sig_t *dcgmi_sig);


void dcgmi_lib_reset_instances(dcgmi_sig_t *dcgmi_sig);


void dcgmi_lib_compute_gpu_gflops(dcgmi_sig_t *dcgmi_sig, signature_t *metrics);


/** Fills memory pointed by \ref event_info_dst with the
 * set and position indices that \ref event_id belongs to.
 *
 * You can use the above defined macros event_info_set and event_info_pos
 * to get the corresponding value in the output parameter.
 *
 * On error, this function sets the global
 * variable state_msg with the cause of that.
 *
 * \param[in] event_id The event ID you are requesting info.
 * \param[out] event_info_dst A pointer to an allocated
 *						 region of memory wich can store at least two integers.
 *
 * \return EAR_ERROR \ref event_id is in an invalid range.
 * \return EAR_ERROR \ref event_info_dst is a NULL pointer.
 * \return EAR_SUCCESS Otherwise.
 */
state_t dcgmi_lib_get_event_info(dcgm_event_idx_t event_id, int (*event_info_dst)[2]);


float gflops_cycle(dcgmi_sig_t *dcgmi_sig, uint gpu_idx);

#endif // _DCGMI_LIB_H_
