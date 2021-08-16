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

#ifndef _EAR_DAEMON_CLIENT_H
#define _EAR_DAEMON_CLIENT_H

#include <common/types/generic.h>
#include <common/types/log.h>
#include <daemon/local_api/eard_conf_api.h>
#include <metrics/gpu/gpu.h>
#include <metrics/frequency/cpu.h>
#include <metrics/imcfreq/imcfreq.h>

/************************************/
/** Tries to connect with the daemon. Returns 0 on success and -1 otherwise. */
int eards_connect(application_t *my_app);
int eards_connected();
/** Closes the connection with the daemon. */
void eards_disconnect();

/************************************/
// Frequency services
/** Given a frequency value, sends a request to change the frequency to that
*   value. Returns -1 if there's an error, 0 if the change_freq service has not
*   been provided, and the final frequency value otherwise. */
/* sets the same CPU freq in all the CPUs */
unsigned long eards_change_freq(unsigned long newfreq);
/* Sets the given CPU freq to CPUS set to 1 in the mask */
unsigned long eards_change_freq_with_mask(unsigned long newfreq,cpu_set_t *mask);
/* Changes the CPU freq of all the CPUs. If CPU freq for a CPU is 0, the CPU freq is not modified */
unsigned long eards_change_freq_with_list(unsigned int num_cpus,unsigned long *newfreq);
/* Returns the CPU freq in the given cpu */
unsigned long eards_get_freq(unsigned int num_cpu);
/* Sets the list of CPU freqs in freqlist and returns the avg*/
unsigned long eards_get_freq_list(unsigned int num_cpus,unsigned long *freqlist); 
/** Tries to set the frequency to the turbo value */
void eards_set_turbo();


/**** This API is going to be deprecated ****/
/** Requests the frequency data size. Returns -1 if there's an error, and the
*   actual value otherwise. */
unsigned long eards_get_data_size_frequency();

/** Sends a request to read the CPU frequency. */
void eards_begin_compute_turbo_freq();
/** Requests to stop reading the CPU frequency. Returns the average frequency
*   between the begin call and the end call on success, -1 otherwise. */
unsigned long eards_end_compute_turbo_freq();

/** Sends a request to read the global CPU frequency. */
void eards_begin_app_compute_turbo_freq();
/** Requests to stop reading the CPU global frequency. Returns the average 
*   frequency between the begin call and the end call on success, -1 otherwise. */
unsigned long eards_end_app_compute_turbo_freq();

/**** New API for cpufreq management */
/* Load, init etc can be used directly from metrics */
/* These are privileged functions */
state_t eards_cpufreq_read(cpufreq_t *ef,size_t size);


/************************************/
// Uncore frequency management
state_t eards_imcfreq_data_count(uint *count);
state_t eards_imcfreq_read(imcfreq_t *ef,size_t size);

state_t eards_mgt_imcfreq_get(ulong service, char *data, size_t size);
state_t eards_mgt_imcfreq_set(ulong service, uint *index_list, uint index_count);

/************************************/
// Uncore services
/** Sends a request to read the uncores. Returns -1 if there's an error, on
*   success stores the uncores values' into *values and returns 0. */
int eards_read_uncore(unsigned long long *values);
/** Sends a request to stop&read the uncores. Returns -1 if there's an error, on
*   success stores the uncores values' into *values and returns 0. */
int eards_stop_uncore(unsigned long long *values);
/** Sends the request to start the uncores. Returns 0 on success, -1 on error */
int eards_start_uncore();
/** Sends a request to reset the uncores. Returns 0 on success, -1 on error */
int eards_reset_uncore();
/** Requests the uncore data size. Returns -1 if there's an error, and the
*   actual value otherwise. */
unsigned long eards_get_data_size_uncore();

/************************************/
// RAPL services
/** Sends a request to read the RAPL counters. Returns -1 if there's an error,
*   and on success returns 0 and fills the array given by parameter with the
*   counters' values. */
int eards_read_rapl(unsigned long long *values);
/** Sends the request to start the RAPL counters. Returns 0 on success or if
*   the application is not connected, -1 if there's an error. */
int eards_start_rapl();
/** Sends the request to reset the RAPL counters. Returns 0 on success or if
*    the application is not connected, -1 if there's an error. */
int eards_reset_rapl();
/** Requests the rapl data size. Returns -1 if there's an error, and the
*   actual value otherwise. */
unsigned long eards_get_data_size_rapl();

/************************************/
// System services
/** Sends a request to the deamon to write the whole application signature.
*   Returns 0 on success, -1 on error. */
ulong eards_write_app_signature(application_t *app_signature);
/** Sends a request to the deamon to write one loop  signature into the DB.
*   Returns 0 on success, -1 on error. */
ulong eards_write_loop_signature(loop_t *loop_signature);
/** Reports a new EAR event */
ulong eards_write_event(ear_event_t *event);

/************************************/
// Node energy services
/** Requests the IPMI data size. Returns -1 if there's an error, and the
*   actual value otherwise. */
unsigned long eards_node_energy_data_size();
/** Requests the DC energy to the node. Returns 0 on success, -1 if there's
*   an error. */
int eards_node_dc_energy(void *energy,ulong datasize);
/** Requests the dc energy node frequency. Returns -1 if there's an error,
*   and 10000000 otherwise. */
ulong eards_node_energy_frequency();

/** Sends the loop_signature to eard to be reported in the DB */
ulong eards_write_loop_signature(loop_t *loop_signature);

/** Returns the frequency at which the node energy frequency is refreshed */
ulong eards_node_energy_frequency();

/************************************/
// GPU services
int eards_gpu_model(uint *gpu_model);
int eards_gpu_dev_count(uint *gpu_dev_count);
int eards_gpu_data_read(gpu_t *gpu_info,uint num_dev);
int eards_gpu_set_freq(uint num_dev,ulong *freqs);
int eards_gpu_get_info(gpu_info_t *info, uint num_dev);

#else
#endif
