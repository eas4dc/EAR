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

/** \defgroup local_api EARD Local API
 * \todo Add a description.
 */

#include <common/types/generic.h>
#include <common/types/log.h>
#include <metrics/gpu/gpu.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>

#include <daemon/local_api/eard_api_rpc.h>
#include <daemon/local_api/eard_api_conf.h>

/** @addtogroup connection Connection 
 * \ingroup local_api
 * \todo Add a description.
 * @{ */
/** This function is to be used for external commands or applications not using the EARL. */
int eards_connection();
/** EARL connection: Tries to connect with the daemon.
 * Returns 0 on success and -1 otherwise. */
int eards_connect(application_t *my_app, ulong lid);
/** \todo Add a description for this function. */
int eards_connected();
/** Closes the connection with the daemon. */
void eards_disconnect();
/** @} */

/** \defgroup rapl_services RAPL services
 * \ingroup local_api
 * \todo Add a description.
 * @{ */
/** Sends a request to read the RAPL counters. Returns -1 if there's an error,
 * and on success returns 0 and fills the array \p values with the counters' values. */
int eards_read_rapl(unsigned long long *values);
/** Sends the request to start the RAPL counters. Returns 0 on success or if
 * the application is not connected, -1 if there's an error. */
int eards_start_rapl();
/** Sends the request to reset the RAPL counters. Returns 0 on success or if
 * the application is not connected, -1 if there's an error. */
int eards_reset_rapl();
/** Requests the rapl data size.
 * Returns -1 if there's an error, and the actual value otherwise. */
unsigned long eards_get_data_size_rapl();
/** @} */

/** \defgroup system_services System services
 * \ingroup local_api
 * \todo Add a description.
 * @{ */
/** Sends a request to the deamon to write the whole application signature.
 * Returns 0 on success, -1 on error. */
ulong eards_write_app_signature(application_t *app_signature);
/** Sends a request to the deamon to write one loop  signature into the DB.
 * Returns 0 on success, -1 on error. */
ulong eards_write_loop_signature(loop_t *loop_signature);
/** Reports a new EAR event. */
ulong eards_write_event(ear_event_t *event);
/** @}*/

/** \defgroup node_energy_services Node energy services
 * \ingroup local_api
 * \todo Add a description.
 * @{ */
/** Requests the IPMI data size.
 * Returns -1 if there's an error, and the actual value otherwise. */
unsigned long eards_node_energy_data_size();
/** Requests the DC energy to the node.
 * Returns 0 on success, -1 if there's an error. */
int eards_node_dc_energy(void *energy,ulong datasize);
/** Requests the dc energy node frequency.
 * Returns -1 if there's an error, and 10000000 otherwise. */
ulong eards_node_energy_frequency();
/** Sends the loop_signature to eard to be reported in the DB. */
ulong eards_write_loop_signature(loop_t *loop_signature);
/** Returns the frequency at which the node energy frequency is refreshed. */
ulong eards_node_energy_frequency();
/** @}  */

/** \defgroup gpu_services GPU services
 * \ingroup local_api
 * \todo Add a description.
 * @{ */
/** \todo  */
int eards_gpu_model(uint *gpu_model);
/** \todo  */
int eards_gpu_dev_count(uint *gpu_dev_count);
/** \todo  */
int eards_gpu_data_read(gpu_t *gpu_info,uint num_dev);
/** \todo  */
int eards_gpu_set_freq(uint num_dev,ulong *freqs);
/** \todo  */
int eards_gpu_get_info(gpu_info_t *info, uint num_dev);
/** @}  */

#endif
