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


#include <library/metrics/dcgmi_lib/common.h>

#include <stdlib.h>
#include <errno.h>

#include <common/output/debug.h>
#include <metrics/gpu/gpuprof.h>


state_t dcgmi_lib_data_alloc(dcgmi_lib_t *dcgmi_lib_data, int set_count)
{
	debug("dcgmi_lib_data_alloc");
	if (dcgmi_lib_data)
	{
		// Set count
		dcgmi_lib_data->dcgmi_set_count = set_count;

		// clean first all pointers to then safetely call free func. in case of error.
		dcgmi_lib_data->dcgmi_set_sizes			= NULL;
		dcgmi_lib_data->dcgmi_event_ids			= NULL;
		dcgmi_lib_data->dcgmi_event_ids_csv	= NULL;
		dcgmi_lib_data->gpuprof_read1				= NULL;
		dcgmi_lib_data->gpuprof_read2 			= NULL;
		dcgmi_lib_data->gpuprof_diff				= NULL;
		dcgmi_lib_data->gpuprof_agg					= NULL;
		// dcgmi_lib_data->dcgm_event_mask			= NULL;

		// Set sizes array
		dcgmi_lib_data->dcgmi_set_sizes = (int *) malloc(set_count * sizeof(int));

		// Event sets array
		dcgmi_lib_data->dcgmi_event_ids = (uint **) calloc(set_count, sizeof(uint *));

		// Event sets string array
		dcgmi_lib_data->dcgmi_event_ids_csv = (char **) calloc(set_count, sizeof(char *));

		// Event sets metrics array
		dcgmi_lib_data->gpuprof_read1 = (gpuprof_t **) malloc(set_count * sizeof(gpuprof_t *));
		dcgmi_lib_data->gpuprof_read2 = (gpuprof_t **) malloc(set_count * sizeof(gpuprof_t *));
		dcgmi_lib_data->gpuprof_diff	= (gpuprof_t **) malloc(set_count * sizeof(gpuprof_t *));
		dcgmi_lib_data->gpuprof_agg		= (gpuprof_t **) malloc(set_count * sizeof(gpuprof_t *));

		// Event sets mask array
		// dcgmi_lib_data->dcgm_event_mask = (uint **) malloc(set_count * sizeof(uint *));

		if (dcgmi_lib_data->dcgmi_set_sizes &&
				dcgmi_lib_data->dcgmi_event_ids &&
				dcgmi_lib_data->dcgmi_event_ids_csv &&
				dcgmi_lib_data->gpuprof_read1 &&
				dcgmi_lib_data->gpuprof_read2 &&
				dcgmi_lib_data->gpuprof_diff &&
				dcgmi_lib_data->gpuprof_agg /* &&
				dcgmi_lib_data->dcgm_event_mask */
				)
		{
			return EAR_SUCCESS;
		} else
		{
			free(dcgmi_lib_data->dcgmi_set_sizes);

			free(dcgmi_lib_data->dcgmi_event_ids);
			free(dcgmi_lib_data->dcgmi_event_ids_csv);

			free(dcgmi_lib_data->gpuprof_read1);
			free(dcgmi_lib_data->gpuprof_read2);
			free(dcgmi_lib_data->gpuprof_diff);
			free(dcgmi_lib_data->gpuprof_agg);

			// free(dcgmi_lib_data->dcgm_event_mask);

			dcgmi_lib_data->dcgmi_set_sizes			= NULL;
			dcgmi_lib_data->dcgmi_event_ids			= NULL;
			dcgmi_lib_data->dcgmi_event_ids_csv	= NULL;
			dcgmi_lib_data->gpuprof_read1				= NULL;
			dcgmi_lib_data->gpuprof_read2 			= NULL;
			dcgmi_lib_data->gpuprof_diff				= NULL;
			dcgmi_lib_data->gpuprof_agg					= NULL;
			// dcgmi_lib_data->dcgm_event_mask			= NULL;

			return_msg(EAR_ERROR, Generr.alloc_error);
		}
	} else
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}
}
