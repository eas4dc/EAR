/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/


#include <library/metrics/dcgmi_lib/turing.h>

#include <stdlib.h>

#include <common/output/debug.h>


#define DCGMI_SET_COUNT 3
#define DCGMI_SET1_SIZE 10
#define DCGMI_SET2_SIZE 1
#define DCGMI_SET3_SIZE 1


state_t dcgmi_lib_turing_init(uint api, uint all_events, dcgmi_lib_t *dcgmi_data, int (*event_info)[DCGMI_LIB_SUPPORTED_EVENTS][2])
{
	if (dcgmi_data && event_info)
	{
		debug("dcgmi_lib_turing_init");

		if (state_fail(dcgmi_lib_data_alloc(dcgmi_data, DCGMI_SET_COUNT)))
		{
			debug("Error allocating dcgmi lib data");
			return_msg(EAR_ERROR, Generr.alloc_error);
		}

		dcgmi_data->dcgmi_set_sizes[0] = (all_events) ? DCGMI_SET1_SIZE : 3;
		dcgmi_data->dcgmi_set_sizes[1] = DCGMI_SET2_SIZE;
		dcgmi_data->dcgmi_set_sizes[2] = DCGMI_SET3_SIZE;

		// Allocate memory for each event set array and event set string
		int i = 0;
		while (i < dcgmi_data->dcgmi_set_count)
		{
			// Event set array
			dcgmi_data->dcgmi_event_ids[i] = (uint *) malloc(dcgmi_data->dcgmi_set_sizes[i] * sizeof(uint));

			if (!dcgmi_data->dcgmi_event_ids[i])
			{
				debug("Error allocating DCGMI event set %d", i);
				break;
			}

			// Event set string
			dcgmi_data->dcgmi_event_ids_csv[i] = (char *) calloc(dcgmi_data->dcgmi_set_sizes[i], ID_SIZE);

			if (!dcgmi_data->dcgmi_event_ids_csv[i])
			{
				debug("Error allocating DCGMI event set string %d", i);
				free(dcgmi_data->dcgmi_event_ids[i]);
				dcgmi_data->dcgmi_event_ids[i] = NULL;
				break;
			}

			i++;
		}

		// Check whether there was an error during allocation
		if (i < dcgmi_data->dcgmi_set_count)
		{
			for (int j = 0; j < i; j++)
			{
				free(dcgmi_data->dcgmi_event_ids[j]);
				free(dcgmi_data->dcgmi_event_ids_csv[j]);
			}

			return_msg(EAR_ERROR, Generr.alloc_error);
		}

		// Set 1
		if (all_events)
		{
			dcgmi_data->dcgmi_event_ids[0][0] = dcgm_sm_active;
			dcgmi_data->dcgmi_event_ids[0][1] = dcgm_sm_occupancy;
			dcgmi_data->dcgmi_event_ids[0][2] = dcgm_tensor_active;
			dcgmi_data->dcgmi_event_ids[0][3] = dcgm_fp32_active;
			dcgmi_data->dcgmi_event_ids[0][4] = dcgm_dram_active;
			dcgmi_data->dcgmi_event_ids[0][5] = dcgm_pcie_tx_bytes;
			dcgmi_data->dcgmi_event_ids[0][6] = dcgm_pcie_rx_bytes;
			dcgmi_data->dcgmi_event_ids[0][7] = dcgm_gr_engine_active;
			dcgmi_data->dcgmi_event_ids[0][8] = dcgm_nvlink_tx_bytes;
			dcgmi_data->dcgmi_event_ids[0][9] = dcgm_nvlink_rx_bytes;
		} else
		{
			dcgmi_data->dcgmi_event_ids[0][0] = dcgm_tensor_active;
			dcgmi_data->dcgmi_event_ids[0][1] = dcgm_fp32_active;
			dcgmi_data->dcgmi_event_ids[0][2] = dcgm_dram_active;
		}

		// Set 2
		dcgmi_data->dcgmi_event_ids[1][0] = dcgm_fp64_active;

		// Set 3
		dcgmi_data->dcgmi_event_ids[2][0] = dcgm_fp16_active;

		int turing_event_info_all[DCGMI_LIB_SUPPORTED_EVENTS][2] =
		{
			{0, 7}, // gr_engine_active
			{0, 0}, // sm_active
			{0, 1}, // sm_occupancy
			{0, 2}, // tensor_active
			{0, 4}, // dram_active
			{1, 0}, // fp64_active
			{0, 3}, // fp32_active
			{2, 0}, // fp16_active
			{0, 5}, // pcie_tx_bytes
			{0, 6}, // pcie_rx_bytes
			{0, 8}, // nvlink_tx_bytes
			{0, 9} // nvlink_rx_bytes
		};

		int turing_event_info_part[DCGMI_LIB_SUPPORTED_EVENTS][2] =
		{
			{-1, -1}, // gr_engine_active
			{-1, -1}, // sm_active
			{-1, -1}, // sm_occupancy
			{0, 0}, // tensor_active
			{0, 2}, // dram_active
			{1, 0}, // fp64_active
			{0, 1}, // fp32_active
			{2, 0}, // fp16_active
			{-1, -1}, // pcie_tx_bytes
			{-1, -1}, // pcie_rx_bytes
			{-1, -1}, // nvlink_tx_bytes
			{-1, -1} // nvlink_rx_bytes
		};

		int (*turing_event_info)[DCGMI_LIB_SUPPORTED_EVENTS][2] = (all_events) ? &turing_event_info_all : &turing_event_info_part;

		memcpy(*event_info, *turing_event_info, DCGMI_LIB_SUPPORTED_EVENTS * 2 * sizeof(int));

#if 0
		for (i = 0; i < DCGMI_LIB_SUPPORTED_EVENTS; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				(*event_info)[i][j] = turing_event_info[i][j];
			}
		}
#endif

		return EAR_SUCCESS;
	} else
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}
}
