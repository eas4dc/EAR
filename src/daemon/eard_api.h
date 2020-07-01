/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/



#ifndef _EAR_DAEMON_CLIENT_H
#define _EAR_DAEMON_CLIENT_H

#include <common/types/generic.h>
#include <common/types/log.h>
#include <daemon/eard_conf_api.h>

/** Tries to connect with the daemon. Returns 0 on success and -1 otherwise. */
int eards_connect(application_t *my_app);
int eards_connected();
/** Closes the connection with the daemon. */
void eards_disconnect();

// Frequency services
/** Given a frequency value, sends a request to change the frequency to that
*   value. Returns -1 if there's an error, 0 if the change_freq service has not
*   been provided, and the final frequency value otherwise. */
unsigned long eards_change_freq(unsigned long newfreq);
/** Tries to set the frequency to the turbo value */
void eards_set_turbo();

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

// System services
/** Sends a request to the deamon to write the whole application signature.
*   Returns 0 on success, -1 on error. */
ulong eards_write_app_signature(application_t *app_signature);
/** Sends a request to the deamon to write one loop  signature into the DB.
*   Returns 0 on success, -1 on error. */
ulong eards_write_loop_signature(loop_t *loop_signature);
/** Reports a new EAR event */
ulong eards_write_event(ear_event_t *event);

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

#else
#endif
