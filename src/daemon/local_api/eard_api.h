/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EARD_LOCAL_API_FUNCTIONS_H
#define EARD_LOCAL_API_FUNCTIONS_H

#include <common/system/version.h>
#include <common/types/generic.h>
#include <common/types/log.h>
#include <daemon/local_api/eard_api_conf.h>
#include <daemon/local_api/eard_api_rpc.h>

// This class is intended to provide easy-to-read functions instead RPCs to
// communicate between binaries and EAR Daemon (EARD).

/** Sets the connection status as failed
 */
void eards_connection_failure();

state_t eard_new_application(application_t *app);
state_t eard_end_application(application_t *app);

typedef struct eard_state_s {
    version_t version;
    uint gpus_supported_count;
    uint cpus_supported_count;
    uint sock_supported_count;
    uint gpus_support_enabled;
    pid_t pid;
} eard_state_t;

state_t eards_get_state(eard_state_t *state);

// Legacy functions

// Request to write an application signature. Returns 0 on success, -1 on error.
ulong eards_write_app_signature(application_t *app_signature);
ulong eards_write_wf_app_signature(application_t *app_signature);

// Request write a loop signature into the DB. Returns 0 on success, -1 on error.
ulong eards_write_loop_signature(loop_t *loop_signature);

// Reports event. Returns 0 on success, -1 on error.
ulong eards_write_event(ear_event_t *event);

// Requests the DC energy of the node. Returns 0 on success, -1 on error.
int eards_node_dc_energy(void *energy, ulong datasize);

// Requests the DC energy node frequency. Returns -1 or 10000000 if not connected.
ulong eards_node_energy_frequency();

// Reports a loop signature to EARD to be reported in the DB.
ulong eards_write_loop_signature(loop_t *loop_signature);

// Returns the frequency at which the node energy frequency is refreshed.
ulong eards_node_energy_frequency();

// Returns if GPU is supported by the EARD.
int eards_gpu_support();

// Returns basic EARD compile configured limits & options for verification
state_t eards_get_state(eard_state_t *state);

/** \defgroup rapl_services RAPL services
 * \ingroup local_api
 * \todo Add a description.
 * @{ */
/** Sends a request to read the RAPL counters. Returns -1 if there's an error,
 * and on success returns 0 and fills the array \p values with the counters' values. */
int eards_read_rapl(unsigned long long *values);
/** Requests the rapl data size.
 * Returns -1 if there's an error, and the actual value otherwise. */
unsigned long eards_get_data_size_rapl();
/** @} */

// Legacy non-RPC services
#define CONNECT_EARD_NODE_SERVICES    5
#define DISCONNECT_EARD_NODE_SERVICES 1000
#define EAR_COM_OK                    0
#define EAR_COM_ERROR                 -1
#define READ_RAPL                     202
#define DATA_SIZE_RAPL                203
#define WRITE_APP_SIGNATURE           300
#define WRITE_EVENT                   302
#define WRITE_LOOP_SIGNATURE          303
#define READ_DC_ENERGY                400
#define ENERGY_FREQ                   403
#define GPU_SUPPORTED                 505
#define WRITE_WF_APP_SIGNATURE        301
#define NEW_API_SERVICES              1000

#endif // EARD_LOCAL_API_FUNCTIONS_H
