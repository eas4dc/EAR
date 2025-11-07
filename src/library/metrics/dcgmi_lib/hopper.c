/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <library/metrics/dcgmi_lib/hopper.h>
#include <stdlib.h>

#include <library/metrics/dcgmi_lib/common.h>
#include <metrics/common/apis.h>

state_t dcgmi_lib_hopper_init(uint api, uint all_events, dcgmi_lib_t *dcgmi_data,
                              int (*event_info)[DCGMI_LIB_SUPPORTED_EVENTS][2])
{
    if (dcgmi_data && event_info) {
        debug("dcgmi_lib_hopper_init");

        if (!API_IS(API_DCGMI, api) && !API_IS(API_NVML, api)) {
            return_msg(EAR_ERROR, Generr.api_undefined);
        }
        debug("dcgmi_lib_data_alloc");
        if (state_fail(dcgmi_lib_data_alloc(dcgmi_data, 1))) {
            char err_msg[64];
            snprintf(err_msg, sizeof(err_msg), "Allocating dcgmi lib data (%s)", state_msg);
            return_msg(EAR_ERROR, err_msg);
        }

        if (all_events) {
            dcgmi_data->dcgmi_set_sizes[0] = DCGMI_LIB_SUPPORTED_EVENTS;
        } else {
            dcgmi_data->dcgmi_set_sizes[0] = 5;
        }

        // clean first all pointers to then safetely call free func. in case of error.
        dcgmi_data->dcgmi_event_ids[0]     = NULL;
        dcgmi_data->dcgmi_event_ids_csv[0] = NULL;

        debug("Allocating dcgmi_event_ids");

        // Allocate memory for event set array and event set string
        dcgmi_data->dcgmi_event_ids[0]     = (uint *) calloc(dcgmi_data->dcgmi_set_sizes[0], sizeof(uint));
        dcgmi_data->dcgmi_event_ids_csv[0] = (char *) calloc(dcgmi_data->dcgmi_set_sizes[0], ID_SIZE);

        if (!dcgmi_data->dcgmi_event_ids[0] || !dcgmi_data->dcgmi_event_ids_csv[0]) {
            debug("Allocating events, string and mask data");

            free(dcgmi_data->dcgmi_event_ids[0]);
            free(dcgmi_data->dcgmi_event_ids_csv[0]);

            dcgmi_data->dcgmi_event_ids[0]     = NULL;
            dcgmi_data->dcgmi_event_ids_csv[0] = NULL;

            return_msg(EAR_ERROR, Generr.alloc_error);
        }

        // Set 1
        if (all_events) {
            debug("Adding all events");
            dcgmi_data->dcgmi_event_ids[0][0] = (API_IS(API_NVML, api)) ? nvml_gr_engine_active : dcgm_gr_engine_active;
            dcgmi_data->dcgmi_event_ids[0][1] = (API_IS(API_NVML, api)) ? nvml_sm_active : dcgm_sm_active;
            dcgmi_data->dcgmi_event_ids[0][2] = (API_IS(API_NVML, api)) ? nvml_sm_occupancy : dcgm_sm_occupancy;
            dcgmi_data->dcgmi_event_ids[0][3] = (API_IS(API_NVML, api)) ? nvml_tensor_active : dcgm_tensor_active;
            dcgmi_data->dcgmi_event_ids[0][4] = (API_IS(API_NVML, api)) ? nvml_dram_active : dcgm_dram_active;
            dcgmi_data->dcgmi_event_ids[0][5] = (API_IS(API_NVML, api)) ? nvml_fp64_active : dcgm_fp64_active;
            dcgmi_data->dcgmi_event_ids[0][6] = (API_IS(API_NVML, api)) ? nvml_fp32_active : dcgm_fp32_active;
            dcgmi_data->dcgmi_event_ids[0][7] = (API_IS(API_NVML, api)) ? nvml_fp16_active : dcgm_fp16_active;
            dcgmi_data->dcgmi_event_ids[0][8] = (API_IS(API_NVML, api)) ? nvml_pcie_tx_bytes : dcgm_pcie_tx_bytes;
            dcgmi_data->dcgmi_event_ids[0][9] = (API_IS(API_NVML, api)) ? nvml_pcie_rx_bytes : dcgm_pcie_rx_bytes;
            dcgmi_data->dcgmi_event_ids[0][10] = (API_IS(API_NVML, api)) ? nvml_nvlink_tx_bytes : dcgm_nvlink_tx_bytes;
            dcgmi_data->dcgmi_event_ids[0][11] = (API_IS(API_NVML, api)) ? nvml_nvlink_rx_bytes : dcgm_nvlink_rx_bytes;
        } else {
            debug("Adding specific events");
            dcgmi_data->dcgmi_event_ids[0][0] = (API_IS(API_NVML, api)) ? nvml_tensor_active : dcgm_tensor_active;
            dcgmi_data->dcgmi_event_ids[0][1] = (API_IS(API_NVML, api)) ? nvml_dram_active : dcgm_dram_active;
            dcgmi_data->dcgmi_event_ids[0][2] = (API_IS(API_NVML, api)) ? nvml_fp64_active : dcgm_fp64_active;
            dcgmi_data->dcgmi_event_ids[0][3] = (API_IS(API_NVML, api)) ? nvml_fp32_active : dcgm_fp32_active;
            dcgmi_data->dcgmi_event_ids[0][4] = (API_IS(API_NVML, api)) ? nvml_fp16_active : dcgm_fp16_active;
        }

        /* Must follow the mapping set by the enum dcgm_event_id defined at dcgmi_lib.h */
        int hopper_event_info_all[DCGMI_LIB_SUPPORTED_EVENTS][2] = {
            {0, 0},  // gr_engine_active
            {0, 1},  // sm_active
            {0, 2},  // sm_occupancy
            {0, 3},  // tensor_active
            {0, 4},  // dram_active
            {0, 5},  // fp64_active
            {0, 6},  // fp32_active
            {0, 7},  // fp16_active
            {0, 8},  // pcie_tx_bytes
            {0, 9},  // pcie_rx_bytes
            {0, 10}, // nvlink_tx_bytes
            {0, 11}  //  nvlink_rx_bytes
        };

        int hopper_event_info_partial[DCGMI_LIB_SUPPORTED_EVENTS][2] = {
            {-1, -1}, // gr_engine_active
            {-1, -1}, // sm_active
            {-1, -1}, // sm_occupancy
            {0, 0},   // tensor_active
            {0, 1},   // dram_active
            {0, 2},   // fp64_active
            {0, 3},   // fp32_active
            {0, 4},   // fp16_active
            {-1, -1}, // pcie_tx_bytes
            {-1, -1}, // pcie_rx_bytes
            {-1, -1}, // nvlink_tx_bytes
            {-1, -1}  //  nvlink_rx_bytes
        };

        int(*hopper_event_info)[DCGMI_LIB_SUPPORTED_EVENTS][2] =
            (all_events) ? &hopper_event_info_all : &hopper_event_info_partial;

#if 0
		for (int i = 0; i < DCGMI_LIB_SUPPORTED_EVENTS; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				(*event_info)[i][j] = (*hopper_event_info)[i][j];
			}
		}
#endif

        memcpy((*event_info), (*hopper_event_info), DCGMI_LIB_SUPPORTED_EVENTS * 2 * sizeof(int));

        return EAR_SUCCESS;
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }
}

state_t dcgmi_lib_hopper_fp_coeffs(int **coeffs, size_t coeffs_length)
{

    int *hopper_coeffs;
    hopper_coeffs = (int *) malloc(coeffs_length * (sizeof(int)));

    hopper_coeffs[0] = 16384; // FP64 activities
    hopper_coeffs[1] = 32768; // FP32 activities
    hopper_coeffs[2] = 65536; // FP16 activities
    hopper_coeffs[3] = 32768; // Tensor activities

    *coeffs = hopper_coeffs;

    return EAR_SUCCESS;
}
