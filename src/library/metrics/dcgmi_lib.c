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


#include <library/metrics/dcgmi_lib.h>

#if !SHOW_DEBUGS
#define NDEBUG
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <common/config/config_env.h>
#include <common/config/config_install.h>
#include <common/environment_common.h>
#include <common/system/lock.h>
#include <common/system/monitor.h>
#include <common/output/debug.h>
#include <common/types/generic.h>
#include <common/types/signature.h>

#include <library/common/verbose_lib.h>
#include <library/common/externs.h>

#include <library/metrics/dcgmi_lib/common.h>
#include <library/metrics/dcgmi_lib/turing.h>
#include <library/metrics/dcgmi_lib/hopper.h>
#include <library/metrics/dcgmi_lib/ampere.h>

#include <metrics/gpu/gpu.h>
#include <metrics/gpu/gpuprof.h>
#include <metrics/common/nvml.h>
#include <metrics/common/apis.h>


#define DCGMI_MET_LVL			3

#define DCGMI_LIB_SAMPLING_PERIOD 2

#define DCGMI_LIB_NOT_SUPPORTED_MSG		"EARL not configured to support GPUs and DCGMI."
#define DCGMI_LIB_MODULE_DISABLED_MSG "DCGMI module not enabled."


typedef state_t (*dcgmi_lib_init_fn) (uint api, uint all_events, dcgmi_lib_t *dcgmi_data, int (*event_info)[DCGMI_LIB_SUPPORTED_EVENTS][2]);

typedef state_t (*dcgmi_lib_fp_coeffs) (int **coeffs, size_t coeffs_length);

static uint dcgmi_lib_enabled = (USE_GPUS && DCGMI) ? DCGM_DEFAULT : 0;

static dcgmi_lib_t dcgmi_data;

static apinfo_t gpuprof_api; // API info of the loaded GPUPROF API

static uint *set_instances_cnt; // Stores the instance counter for each set.

static int event_info[DCGMI_LIB_SUPPORTED_EVENTS][2]; // Event information filled by dcgmi_lib_init_fn

/* DCGMI monitor variables. */ 
static suscription_t *dcgmi_monitor;
static uint						dcgmi_curr_ev_set;
static uint						dcgmi_monitor_stop;

static pthread_mutex_t dcgmi_lock = PTHREAD_MUTEX_INITIALIZER;

static double elapsed_time;

/* DCGM event names. This array must match the same logic as dcgmi_event_idx_t */
static char *dcgmi_ev_names[event_id_max] =
{
  "gr_engine_active",
  "sm_active",
  "sm_occupancy",
  "tensor_active",
  "dram_active",
  "fp64_active",
  "fp32_active",
  "fp16_active",
  "pcie_tx_bytes",
  "pcie_rx_bytes",
  "nvlink_tx_bytes",
  "nvlink_rx_bytes"
};

static dcgmi_lib_fp_coeffs fp_coeffs_fn;

/** Inits gpuprof_api static member and checks whether GPUPROF API is not DUMMY.
 * This function must be used, at least, before using gpuprof_api member anywhere in the module. */
static uint is_gpuprof_api_valid();

/** Inits all static data structures of this module. */
static state_t static_init();

/** Creates a comma-separated list of event Ids. Used to set events through GPUPROF API.
 * \param[out] event_set_csv Must point to an allocated region of memory of at least ID_SIZE
 * (see common/types/generic.h) * event_set_size. */
static void event_set2csv(uint *event_set /*, uint *event_set_mask */, uint event_set_size, char *event_set_csv);

/** DCGMI monitor subscription init function. */
static state_t dcgmi_paction_init(void *p);

/** DCGMI monitor subscription periodic action. */
static state_t dcgmi_paction(void *p);

/** Aggregate GPUPROF data. */
static void aggregate_gpuprof_data(gpuprof_t *gpuprof_agg, gpuprof_t *gpuprof_src);

/** Verboses data of the dcgmi_curr_ev_set. */
static void verbose_current_event_set(int v_lvl);

/** Fills d_str with with a string showing data for all devices stored on d.
 *
 * \param[in] d gpuprof_t pointer containing GPUPROF data of a single set.
 * \param[in] set_idx The set index corresponding to \ref d. */
static void	dcgmi_data_tostr(gpuprof_t *d, char *d_str, size_t d_str_size, uint set_idx);

/** Selects the dcgmi_lib init (allocation) function based on GPU model. */
static state_t set_init_fn(dcgmi_lib_init_fn *dst_fn_ptr, dcgmi_lib_fp_coeffs *fp_coeffs_ptr);

/**   */
static void dcgm_ratios_as_percent(gpuprof_t *event_metrics_set, uint *event_ids_set, int set_size);


state_t dcgmi_lib_load()
{
	if (USE_GPUS && DCGMI)
	{
		verbose_info_master("GPU and DCGMI are supported.");

		/* First checkout whether DCGMI is explicitely requested
		 * to be enabled or disabled by the user. */
		char *use_dcgmi = ear_getenv(FLAG_GPU_DCGMI_ENABLED);
		if (use_dcgmi)
		{
			dcgmi_lib_enabled = atoi(use_dcgmi);
			verbose_info("DCGMI metrics %s by environment variable.",
									 dcgmi_lib_enabled ? "enabled" : "disabled");
		}

		if (!dcgmi_lib_enabled)
		{
			return_msg(EAR_WARNING, DCGMI_LIB_MODULE_DISABLED_MSG);
		}

		/* Init the module */

		/* Load GPUPROF API */
		gpuprof_load(API_FREE); // I don't give priority to any particular API
		if (!is_gpuprof_api_valid()) // Set gpuprof_api static member.
		{
			dcgmi_lib_enabled = 0;
			return_msg(EAR_ERROR, "GPUPROF DUMMY class loaded.");
		}

		/* Init GPUPROF API */
		state_t init_st = gpuprof_init();
		if (state_fail(init_st))
		{
			dcgmi_lib_enabled = 0;
			return_msg(EAR_ERROR, "GPUPROF init failed.");
		}

		/* Allocate and init static members. */
		if (state_fail(static_init()))
		{
			dcgmi_lib_enabled = 0;
			return_msg(EAR_ERROR, "Initiating the module.");
		}

		// TODO: Encapsulate below code inside a function.

		if (state_fail(monitor_init()))
		{
			dcgmi_lib_enabled = 0;

			verbose_warning("Monitor init failed (%s)", state_msg);

			return_msg(EAR_ERROR, "Monitor init failed.");
		}

		dcgmi_monitor							= suscription();
		dcgmi_monitor->call_init 	= dcgmi_paction_init;
		dcgmi_monitor->call_main 	= dcgmi_paction;

		/* Configure the sampling period time. */

		float sampling_period = DCGMI_LIB_SAMPLING_PERIOD;

		char *csampling_period = ear_getenv(FLAG_DCGMI_SAMPLING_PERIOD);

		if (csampling_period)
		{
			sampling_period = strtof(csampling_period, NULL);
			if (sampling_period <= 0)
			{
				// The user may want to know whether it set an incorrect value.
				verbose_error("Invalid sampling period: %f", sampling_period);

				// If invalid value, reset to the default one
				sampling_period = DCGMI_LIB_SAMPLING_PERIOD;
			}
		}

		verbose_info("DCGMI sampling period: %f s", sampling_period);

		// Time burst and relax are the same here.
		dcgmi_monitor->time_relax	= (int) (sampling_period * 1000);  // Miliseconds
		dcgmi_monitor->time_burst	= dcgmi_monitor->time_relax;

		if (state_ok(monitor_register(dcgmi_monitor)))
		{
			return EAR_SUCCESS;
		} else
		{
			dcgmi_lib_enabled = 0;
			return_msg(EAR_ERROR, "Subscription failed.");
		}
	} else
	{
		dcgmi_lib_enabled = 0;
		return_msg(EAR_WARNING, DCGMI_LIB_NOT_SUPPORTED_MSG);
	}
}


uint dcgmi_lib_is_enabled()
{
	return dcgmi_lib_enabled;
}


void dcgmi_lib_dcgmi_sig_init(dcgmi_sig_t *dcgmi_sig)
{
	if (dcgmi_lib_enabled)
	{
		dcgmi_sig->set_cnt						= dcgmi_data.dcgmi_set_count;
		dcgmi_sig->gpu_cnt						= gpuprof_api.devs_count;
		dcgmi_sig->set_instances_cnt	= calloc(dcgmi_sig->set_cnt, sizeof(uint));
		dcgmi_sig->event_metrics_sets	= calloc(dcgmi_sig->set_cnt, sizeof(gpuprof_t *));
		dcgmi_sig->gpu_gflops					= calloc(gpuprof_api.devs_count, sizeof(float));

		verbose_info2_master("EARL DCGM: %u GPUs detected.", dcgmi_sig->gpu_cnt);

		for (uint set = 0; set < dcgmi_sig->set_cnt; set++)
		{
			gpuprof_data_alloc(&dcgmi_sig->event_metrics_sets[set]);
		}
	}
}


state_t dcgmi_lib_dcgmi_sig_csv_header(char *buffer, size_t buff_size)
{
	if (buffer && buff_size)
	{
		strncpy(buffer, "DCGMI_EVENTS_COUNT", buff_size);
		for (int event_idx = 0; event_idx < event_id_max; event_idx++)
		{
			for (int gpu_idx = 0; gpu_idx < MAX_GPUS_SUPPORTED; gpu_idx++)
			{
				char gpu_event_field[ID_SIZE];
				snprintf(gpu_event_field, sizeof(gpu_event_field),
								 ";GPU%d_%s", gpu_idx, dcgmi_ev_names[event_idx]);

				strncat(buffer, gpu_event_field,
								buff_size - strlen(buffer) - 1);
			}
		}
		return EAR_SUCCESS;
	} else
	{
		return EAR_ERROR;
	}
}


state_t dcgmi_lib_dcgmi_sig_to_csv(dcgmi_sig_t *dcgmi_sig, char *buffer, size_t buff_size)
{
	if (dcgmi_sig && buffer && buff_size)
	{
		/* The total number of events */
		snprintf(buffer, buff_size, "%d", event_id_max);

		/* The event_idx order is marked by dcgm_event_idx_t */
		for (int event_idx = 0; event_idx < event_id_max; event_idx++)
		{
			int event_info[2];
			dcgmi_lib_get_event_info(event_idx, &event_info);

			int event_set = event_info[event_info_set];
			int event_pos = event_info[event_info_pos];

			/* Iterate across GPUs supported */
			for (int gpu_idx = 0; gpu_idx < MAX_GPUS_SUPPORTED; gpu_idx++)
			{
				char set_event_metric_buff[32];

				/* If the event is supported and the device exists */
				if (event_set >= 0 && dcgmi_sig->gpu_cnt > gpu_idx)
				{
					snprintf(set_event_metric_buff, sizeof(set_event_metric_buff),
									 ";%f", dcgmi_sig->event_metrics_sets[event_set][gpu_idx].values[event_pos]);
				} else
				{
					/* Else, we put a zero */
					snprintf(set_event_metric_buff, sizeof(set_event_metric_buff), ";0.0");
				}
				strncat(buffer, set_event_metric_buff, buff_size - strlen(buffer) - 1);
			}
		}
		return EAR_SUCCESS;
	} else
	{
		return EAR_ERROR;
	}
}


state_t dcgmi_lib_get_current_metrics(dcgmi_sig_t *dcgmi_sig)
{
	if (!dcgmi_lib_enabled)
	{
			return_msg(EAR_ERROR, DCGMI_LIB_MODULE_DISABLED_MSG);
	}

	if (!dcgmi_sig)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	while(ear_trylock(&dcgmi_lock) != EAR_SUCCESS);

	for (uint set = 0; set < dcgmi_sig->set_cnt; set++)
	{
		verbose_info2_master("DCGMI[%u] loop metrics computation...", set);
		
		gpuprof_data_copy(dcgmi_sig->event_metrics_sets[set], dcgmi_data.gpuprof_diff[set]);

		dcgmi_sig->set_instances_cnt[set] = set_instances_cnt[set];
	}

	ear_unlock(&dcgmi_lock);

	return EAR_SUCCESS;
}


state_t dcgmi_lib_get_global_metrics(dcgmi_sig_t *dcgmi_sig)
{
	if (!dcgmi_lib_enabled)
	{
			return_msg(EAR_ERROR, DCGMI_LIB_MODULE_DISABLED_MSG);
	}

	if (!dcgmi_sig)
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}

	while(ear_trylock(&dcgmi_lock) != EAR_SUCCESS);

	dcgmi_monitor_stop = 1;

	// As we are multiplexing profile metrics reading, the elapsed time must be averaged.
	// double elapsed = elapsed_time / dcgmi_data.dcgmi_set_count;

	for (uint set = 0; set < dcgmi_data.dcgmi_set_count; set++)
	{
		gpuprof_t *agg = dcgmi_data.gpuprof_agg[set];

		// Elapsed time for the set is stored at dev 0
		double elapsed = agg->time_s;
		for (uint gpu_idx = 0; gpu_idx < gpuprof_api.devs_count; gpu_idx++)
		{
			for (uint event_idx = 0; event_idx < dcgmi_data.dcgmi_set_sizes[set]; event_idx++)
			{
				dcgmi_sig->event_metrics_sets[set][gpu_idx].values[event_idx] = agg[gpu_idx].values[event_idx] / elapsed;
			}
		}

		if (VERB_ON(DCGMI_MET_LVL))
		{
			size_t str_size = dcgmi_sig->gpu_cnt * dcgmi_data.dcgmi_set_sizes[set] * ID_SIZE * sizeof(char);
			char *event_metrics_sets_str = malloc(str_size);
			if (event_metrics_sets_str)
			{
				memset(event_metrics_sets_str, 0, str_size);

				dcgmi_data_tostr(dcgmi_sig->event_metrics_sets[set], event_metrics_sets_str, str_size, set);

				verbose_master(DCGMI_MET_LVL, "DCGMI set %u app metrics %s elapsed %.3lf sec",
											 set, event_metrics_sets_str, elapsed);
			}
			free(event_metrics_sets_str);
		}
	}

	ear_unlock(&dcgmi_lock);

	return EAR_SUCCESS;
}


void dcgmi_lib_compute_gpu_gflops(dcgmi_sig_t *dcgmi_sig, signature_t *metrics)
{
	if (metrics == NULL) return;
  if (!dcgmi_lib_enabled || !dcgmi_sig) return;
  if (!fp_coeffs_fn) return;
	
  int fp16_info[2];
	dcgmi_lib_get_event_info(fp16_active, &fp16_info);
	gpuprof_t *dcgmis_data_16 = dcgmi_sig->event_metrics_sets[fp16_info[event_info_set]];

	int fp32_info[2];
	dcgmi_lib_get_event_info(fp32_active, &fp32_info);
	gpuprof_t *dcgmis_data_32 = dcgmi_sig->event_metrics_sets[fp32_info[event_info_set]];

  int fp64_info[2];
	dcgmi_lib_get_event_info(fp64_active, &fp64_info);
	gpuprof_t *dcgmis_data_64 = dcgmi_sig->event_metrics_sets[fp64_info[event_info_set]];

  int tensor_info[2];
	dcgmi_lib_get_event_info(tensor_active, &tensor_info);
	gpuprof_t *dcgmis_data_ten = dcgmi_sig->event_metrics_sets[tensor_info[event_info_set]];

  float fp16, fp32, fp64, tensor, gpu_freq, gflops_cycle;    
  int *coeffs;
  fp_coeffs_fn(&coeffs, 4);

	uint gpu_idx = 0;
	for (gpu_idx = 0; gpu_idx < dcgmi_sig->gpu_cnt; gpu_idx++) 
	{
		fp16 = (float) (dcgmis_data_16[gpu_idx].values[fp16_info[event_info_pos]]);
		fp32 = (float) (dcgmis_data_32[gpu_idx].values[fp32_info[event_info_pos]]);
		fp64 = (float) (dcgmis_data_64[gpu_idx].values[fp64_info[event_info_pos]]);
		tensor = (float) (dcgmis_data_ten[gpu_idx].values[tensor_info[event_info_pos]]);

		gflops_cycle = (coeffs[0]*fp64 + coeffs[1]*fp32 + coeffs[2]*fp16 + coeffs[3]*tensor)/100;
		gpu_freq = (float) (metrics->gpu_sig.gpu_data[gpu_idx].GPU_freq / 1000000.0);

		dcgmi_sig->gpu_gflops[gpu_idx] = gflops_cycle * gpu_freq;

		verbose(DCGMI_MET_LVL, "GPU%u GFLOPS: fp16 %f fp32 %f fp64 %f tensor %f gflops_cycle %f gpu_f %f: %f",
						gpu_idx, fp16, fp32, fp64, tensor, gflops_cycle, gpu_freq, dcgmi_sig->gpu_gflops[gpu_idx]);
#if WF_SUPPORT
		metrics->gpu_sig.gpu_data[gpu_idx].GPU_GFlops = gflops_cycle * gpu_freq;
		verbose(DCGMI_MET_LVL, "Storing GPU GFLOPS into GPU signature (%f)", metrics->gpu_sig.gpu_data[gpu_idx].GPU_GFlops);
#endif
	}

#if WF_SUPPORT
	// Fill positions left
	while (gpu_idx < MAX_GPUS_SUPPORTED)
	{
		metrics->gpu_sig.gpu_data[gpu_idx].GPU_GFlops = 0;
		gpu_idx++;
	}
#endif
	return;
}


void dcgmi_lib_reset_instances(dcgmi_sig_t *dcgmi_sig)
{
	if (!dcgmi_lib_enabled || !dcgmi_sig)
	{
		return;
	}

	if (dcgmi_sig->set_instances_cnt)
	{
		ear_lock(&dcgmi_lock);

		for (uint set = 0; set < dcgmi_data.dcgmi_set_count; set++)
		{
			dcgmi_sig->set_instances_cnt[set] = 0;
			set_instances_cnt[set] = 0;
		}

		ear_unlock(&dcgmi_lock);
	}
}


static uint is_gpuprof_api_valid()
{
	gpuprof_get_info(&gpuprof_api);
	return !(API_IS(gpuprof_api.api, API_DUMMY));
}


static state_t static_init()
{
	// Set the allocate function based on the GPU model.
	dcgmi_lib_init_fn init_fn;

	if (state_fail(set_init_fn(&init_fn, &fp_coeffs_fn)))
	{
		verbose_warning("Error getting the device model (%s)", state_msg);
		return_msg(EAR_ERROR, "Getting device model.");
	}

	uint all_events = DCGM_ALL_EVENTS_DEFAULT;
	char *all_events_c = ear_getenv(FLAG_DCGM_ALL_EVENTS);
	if (all_events_c)
	{
		all_events = strtol(all_events_c, NULL, 10);
		if (all_events)
		{
			verbose_info_master("All DCGM metrics requested.");
		}
	}

	// init_fn allocates and initialize all data structures based on GPU microarch.
	if (state_fail(init_fn(gpuprof_api.api, all_events, &dcgmi_data, &event_info)))
	{
		verbose_warning("Error allocating data (%s)", state_msg);
		return_msg(EAR_ERROR, "Allocating data");
	}

	// Allocate set sample count
	set_instances_cnt = calloc(dcgmi_data.dcgmi_set_count, sizeof(uint));

	if (!set_instances_cnt)
	{
		// dcgmi_data is allocated in init_fn
		dcgmi_lib_data_free(&dcgmi_data);
		return_msg(EAR_ERROR, Generr.alloc_error);
	}

	/* Setup each set's event IDs string readable by gpuprof API */
	for (uint set = 0; set < dcgmi_data.dcgmi_set_count; set++)
	{
		// Creates a comma-separated list of the set's event ID's
		event_set2csv(dcgmi_data.dcgmi_event_ids[set], dcgmi_data.dcgmi_set_sizes[set],
									dcgmi_data.dcgmi_event_ids_csv[set]);

		gpuprof_data_alloc(&dcgmi_data.gpuprof_read1[set]);
		gpuprof_data_alloc(&dcgmi_data.gpuprof_read2[set]);
		gpuprof_data_alloc(&dcgmi_data.gpuprof_diff[set]);
		gpuprof_data_alloc(&dcgmi_data.gpuprof_agg[set]);
	}

	return EAR_SUCCESS;
}


static void event_set2csv(uint *event_set/*, uint *event_set_mask*/, uint event_set_size, char *event_set_csv)
{
	event_set_csv[0] = '\0';

	for (int i = 0; i < event_set_size; i++)
	{
		char event_id_str[ID_SIZE]; // ID_SIZE is the size used for each event when 
																// allocating 'event_set_csv' argument.
		if (i)
		{
			// Append a comma after the first event
			snprintf(event_id_str, ID_SIZE, ",%u", event_set[i]);
		} else
		{
			snprintf(event_id_str, ID_SIZE, "%u", event_set[0]);
		}

		strcat(event_set_csv, event_id_str);
	}

	debug("Event set csv: %s", event_set_csv);
}


state_t dcgmi_lib_get_event_info(dcgm_event_idx_t event_id, int (*event_info_dst)[2])
{
	if (event_info_dst)
	{
		if (event_id >= 0 && event_id < event_id_max)
		{
			(*event_info_dst)[0] = event_info[event_id][0];
			(*event_info_dst)[1] = event_info[event_id][1];
			// debug("event_info %d: %d/%d", event_id, event_info[event_id][0], event_info[event_id][1]);
			return EAR_SUCCESS;
		} else
		{
			return_msg(EAR_ERROR, Generr.arg_outbounds);
		}
	} else
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}
}


static state_t dcgmi_paction_init(void *p)
{
	// all_devs mean all GPUs available to the job.
	// events_set internally starts the pool reading
	gpuprof_events_set(all_devs, dcgmi_data.dcgmi_event_ids_csv[dcgmi_curr_ev_set]);

	sleep(2); // Let the API to collect some metrics

	gpuprof_read(dcgmi_data.gpuprof_read1[dcgmi_curr_ev_set]);

	return EAR_SUCCESS;
}


static state_t dcgmi_paction(void *p)
{
	if (!dcgmi_monitor_stop)
	{
		double time_s = dcgmi_monitor->time_burst / 1000.0; // From ms to s Old value: 2.0;

		ear_lock(&dcgmi_lock);

		elapsed_time += time_s;

		/* Read current set of events. */
		gpuprof_t *read1 = dcgmi_data.gpuprof_read1[dcgmi_curr_ev_set];
		gpuprof_t *read2 = dcgmi_data.gpuprof_read2[dcgmi_curr_ev_set];
		gpuprof_t *diff = dcgmi_data.gpuprof_diff[dcgmi_curr_ev_set];

		gpuprof_read_diff(read2, read1, diff);
		if (API_IS(gpuprof_api.api, API_DCGMI))
		{
			dcgm_ratios_as_percent(diff, dcgmi_data.dcgmi_event_ids[dcgmi_curr_ev_set],
					dcgmi_data.dcgmi_set_sizes[dcgmi_curr_ev_set]);
		}

#if SHOW_DEBUGS
		size_t nmemb = gpuprof_api.devs_count * dcgmi_data.dcgmi_set_sizes[dcgmi_curr_ev_set] * ID_SIZE;

		char *event_metrics_sets_str = calloc(nmemb, sizeof(char));
		dcgmi_data_tostr(read1, event_metrics_sets_str, nmemb * sizeof(char), dcgmi_curr_ev_set);
		debug("SET %u read1: %s elapsed %.3ld s", dcgmi_curr_ev_set, event_metrics_sets_str, read1->time.tv_sec);

		memset(event_metrics_sets_str, 0, nmemb * sizeof(char));

		dcgmi_data_tostr(read2, event_metrics_sets_str, nmemb * sizeof(char), dcgmi_curr_ev_set);
		debug("SET %u read2: %s elapsed %.3ld s", dcgmi_curr_ev_set, event_metrics_sets_str, read2->time.tv_sec);
#endif

		/* Accumulate current set of events. */
		gpuprof_t *agg = dcgmi_data.gpuprof_agg[dcgmi_curr_ev_set];

		aggregate_gpuprof_data(agg, diff);

		verbose_current_event_set(DCGMI_MET_LVL+1);

		// Increment the instance count of the current set.
		set_instances_cnt[dcgmi_curr_ev_set]++;

		if (dcgmi_data.dcgmi_set_count > 1)
		{
			// Unsets the current event set.
			gpuprof_events_unset(all_devs);

			/* Change the active set of events */
			dcgmi_curr_ev_set = (dcgmi_curr_ev_set + 1) % dcgmi_data.dcgmi_set_count;
			gpuprof_events_set(all_devs, dcgmi_data.dcgmi_event_ids_csv[dcgmi_curr_ev_set]);
			sleep(1);

			/* Start reading the new event set. */
			gpuprof_read(dcgmi_data.gpuprof_read1[dcgmi_curr_ev_set]);
		} else
		{
			// Just one set, so no need to unset events, increment the set index, etc.
			gpuprof_data_copy(read1, read2); // Copy read2 to read1
		}

		ear_unlock(&dcgmi_lock);

	} else
	{
		// TODO: Could we unsubscribe here?
	}
	return EAR_SUCCESS;
}


static void aggregate_gpuprof_data(gpuprof_t *gpuprof_agg, gpuprof_t *gpuprof_src)
{
	if (gpuprof_src && gpuprof_agg)
	{
		assert(&gpuprof_agg->time_s == &gpuprof_agg[0].time_s);
		assert(&gpuprof_src->time_s == &gpuprof_src[0].time_s);

		// Based on what seen in gpuprof_dcgmi.c, I suppose I'm accessing the device 0 info
		gpuprof_agg->time_s += gpuprof_src->time_s;
		debug("Elapsed time + %f = %f", gpuprof_src->time_s, gpuprof_agg->time_s);

		for (int dev = 0; dev < gpuprof_api.devs_count; dev++)
		{
			gpuprof_agg[dev].hash = gpuprof_src[dev].hash;
			gpuprof_agg[dev].values_count = gpuprof_src[dev].values_count;

			gpuprof_agg[dev].samples_count += gpuprof_src[dev].samples_count;

			debug("Device %d hash: %s, #events: %u, #samples + %f = %f",
						dev, gpuprof_agg[dev].hash, gpuprof_agg[dev].values_count,
						gpuprof_src[dev].samples_count, gpuprof_agg[dev].samples_count);

			for (int met = 0; met < gpuprof_src[dev].values_count; met++)
			{
				gpuprof_agg[dev].values[met] += (gpuprof_src[dev].values[met] * gpuprof_src->time_s);
				debug("\tmetric %d agg: %f", met, gpuprof_agg[dev].values[met]);
			}
		}
	}
}


static void	dcgmi_data_tostr(gpuprof_t *d, char *d_str, size_t d_str_size, uint set_idx)
{
	if ((d == NULL) || (d_str == NULL)) return;

	d_str[0] = '\0';

	for (uint g = 0; g < gpuprof_api.devs_count; g++)
	{
		char ev_str[ID_SIZE];
		snprintf(ev_str, sizeof(ev_str), "\n--- GPU %u ---", g);

		strncat(d_str, ev_str, d_str_size - strlen(d_str));

		for (int e = 0; e < event_id_max; e++)
		{
			int event_info[2];
			dcgmi_lib_get_event_info(e, &event_info);

			debug("Event %d: set %d pos %d", e, event_info[event_info_set], event_info[event_info_pos]);

			if (event_info[event_info_set] == (int) set_idx)
			{
				/* The set of the event is the requested one */

				int event_pos = event_info[event_info_pos];

				snprintf(ev_str, sizeof(ev_str), "\n%s %lf", dcgmi_ev_names[e], d[g].values[event_pos]);
				strncat(d_str, ev_str, d_str_size - strlen(d_str));
			}
		}
	}
}


static void verbose_current_event_set(int v_lvl)
{
	if (VERB_ON(v_lvl))
	{
		// Most recent metrics of the current set.
		gpuprof_t *gpuprof = dcgmi_data.gpuprof_diff[dcgmi_curr_ev_set];
		uint event_cnt = dcgmi_data.dcgmi_set_sizes[dcgmi_curr_ev_set];

		size_t nmemb = gpuprof_api.devs_count * event_cnt * ID_SIZE;

		char *event_metrics_sets_str = calloc(nmemb, sizeof(char));
		if (!event_metrics_sets_str)
		{
			return;
		}

		dcgmi_data_tostr(gpuprof, event_metrics_sets_str, nmemb * sizeof(char), dcgmi_curr_ev_set);

		verbose_master(0, "DCGMI LIB set %u periodic metrics: %s elapsed %.3lf sec (total %.3lf sec)",
									 dcgmi_curr_ev_set, event_metrics_sets_str, gpuprof->time_s, elapsed_time);

		free(event_metrics_sets_str);
	}
}


static state_t set_init_fn(dcgmi_lib_init_fn *dst_fn_ptr, dcgmi_lib_fp_coeffs *fp_coeffs_ptr)
{
	if (dst_fn_ptr)
	{
		apinfo_t gpu_info;	
		gpu_get_info(&gpu_info);
		switch (gpu_info.dev_model)
		{
			case NVML_DEVICE_ARCH_TURING:
				debug("Turing microarch detected.");
				*dst_fn_ptr = dcgmi_lib_turing_init;
				*fp_coeffs_ptr = NULL; // dcgmi_lib_turing_fp_coeffs; TODO add 
				break;
			case NVML_DEVICE_ARCH_HOPPER:
				debug("Hopper microarch detected.");
				*dst_fn_ptr = dcgmi_lib_hopper_init;
				*fp_coeffs_ptr = dcgmi_lib_hopper_fp_coeffs;
				break;
			case NVML_DEVICE_ARCH_AMPERE:
				debug("Ampere microarch detected.");
				*dst_fn_ptr = dcgmi_lib_ampere_init;
				*fp_coeffs_ptr = dcgmi_lib_ampere_fp_coeffs;
				break;
			default:
				*dst_fn_ptr = NULL;
				return_msg(EAR_ERROR, "Device unknown or not supported.");
		}
		return EAR_SUCCESS;
	} else
	{
		return_msg(EAR_ERROR, Generr.input_null);
	}
}


static void dcgm_ratios_as_percent(gpuprof_t *event_metrics_set, uint *event_ids_set, int set_size)
{
	static uint dcgm_metrics_ratio[event_id_max] =
	{
		1, // dcgm_gr_engine_active
		1, // dcgm_sm_active
		1, // dcgm_sm_occupancy
		1, // dcgm_tensor_active
		1, // dcgm_dram_active
		1, // dcgm_fp64_active
		1, // dcgm_fp32_active
		1, // dcgm_fp16_active
		0, // dcgm_pcie_tx_bytes
		0, // dcgm_pcie_rx_bytes
		0, // dcgm_nvlink_tx_bytes
		0  // dcgm_nvlink_rx_bytes
	};

	for (int event_idx = 0; event_idx < set_size; event_idx++)
	{
		uint event_id = event_ids_set[event_idx];
		// We can use dcgm_first_event_id because we only call this function when API_DCGMI is used.
		// But BE CAREFUL
		uint event_is_ratio = dcgm_metrics_ratio[event_id - dcgm_first_event_id];

		for (int gpu_idx = 0; gpu_idx < gpuprof_api.devs_count; gpu_idx++)
		{
			event_metrics_set[gpu_idx].values[event_idx] *= (1 + 99 * event_is_ratio);
		}
	}
}
